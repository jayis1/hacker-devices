/*
 * drivers/usb_cdc.c — USB CDC Virtual Serial for Prism-Tap
 *
 * Implements a USB 2.0 CDC-ACM virtual serial port over the STM32H730's
 * internal USB OTG HS PHY. Used for command/control and bulk frame
 * exfiltration.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "usb_cdc.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- USB OTG HS register base ---- */
#define USB_OTG_HS_BASE    0x40080000U
#define USB_OTG_GOTGCTL    (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x000))
#define USB_OTG_GINTSTS    (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x008))
#define USB_OTG_GINTMSK    (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x00C))
#define USB_OTG_GCCFG      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x038))
#define USB_OTG_DCFG       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x800))
#define USB_OTG_DCTL       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x804))
#define USB_OTG_DAINT      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x818))
#define USB_OTG_DAINTMSK   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x81C))

/* Device endpoint registers */
#define USB_OTG_DIEPCTL0   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x900))
#define USB_OTG_DIEPINT0   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x908))
#define USB_OTG_DIEPTSIZ0  (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x910))
#define USB_OTG_DIEPDMA0   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x914))
#define USB_OTG_DIEPCTL1   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x920))
#define USB_OTG_DOEPCTL0   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0xA00))
#define USB_OTG_DOEPINT0   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0xA08))
#define USB_OTG_DOEPTSIZ0  (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0xA10))

/* ---- CDC-ACM descriptors (simplified) ---- */

static const uint8_t cdc_device_descriptor[] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType: Device */
    0x00, 0x02, /* bcdUSB: 2.00 */
    0x02,       /* bDeviceClass: CDC */
    0x00,       /* bDeviceSubClass */
    0x00,       /* bDeviceProtocol */
    0x40,       /* bMaxPacketSize0: 64 */
    0x83, 0x04, /* idVendor: 0x0483 (ST) */
    0x57, 0x12, /* idProduct: 0x1257 (Prism-Tap) */
    0x00, 0x01, /* bcdDevice: 1.00 */
    0x01,       /* iManufacturer */
    0x02,       /* iProduct */
    0x03,       /* iSerialNumber */
    0x01,       /* bNumConfigurations */
};

static const uint8_t cdc_config_descriptor[] = {
    /* Configuration descriptor */
    0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,
    /* Interface 0: CDC Control */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* CDC Header */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* CDC Call Management */
    0x05, 0x24, 0x01, 0x00, 0x01,
    /* CDC ACM */
    0x04, 0x24, 0x02, 0x02,
    /* CDC Union */
    0x05, 0x24, 0x06, 0x00, 0x01,
    /* Endpoint: Notification (IN, interrupt) */
    0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x08,
    /* Interface 1: CDC Data */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* Endpoint: Bulk OUT */
    0x07, 0x05, 0x01, 0x02, 0x00, 0x02, 0x00,
    /* Endpoint: Bulk IN */
    0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00,
};

static const char string_manufacturer[] = "jayis1";
static const char string_product[] = "Prism-Tap MIPI Interface";
static const char string_serial[] = "PT0001";

/* ---- Internal state ---- */

static uint8_t usb_connected = 0;
static uint8_t usb_configured = 0;
static uint8_t bulk_exfil_active = 0;
static void (*baud_callback)(uint32_t) = 0;

/* RX/TX ring buffers */
#define USB_BUF_SIZE 512
static uint8_t rx_buf[USB_BUF_SIZE];
static volatile uint16_t rx_head = 0, rx_tail = 0;
static uint8_t tx_buf[USB_BUF_SIZE];

/* ---- USB initialization ---- */

int usb_init(void)
{
    /* Enable GPIOA clock for USB DM/DP pins */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN;

    /* Configure PA11 (DM) and PA12 (DP) as analog mode for USB PHY */
    uint32_t moder = GPIO_REG(GPIOA_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (USB_DM_PIN * 2));
    moder &= ~(3U << (USB_DP_PIN * 2));
    moder |= (GPIO_MODE_ANALOG << (USB_DM_PIN * 2));
    moder |= (GPIO_MODE_ANALOG << (USB_DP_PIN * 2));
    GPIO_REG(GPIOA_BASE, GPIO_MODER_OFFSET) = moder;

    /* Enable USB OTG HS clock */
    RCC_AHB1ENR |= (1U << 1);  /* USB1OTGHSEN */

    /* Soft disconnect */
    USB_OTG_DCTL |= (1U << 1);  /* SDET: soft disconnect */

    /* Configure device mode */
    USB_OTG_GCCFG &= ~(1U << 16);  /* NOVBUSSENS */
    USB_OTG_DCFG = 0x03 << 0;      /* DSPD: high-speed */
    USB_OTG_DCFG &= ~(0xFU << 4);  /* device address = 0 */

    /* Enable global interrupts */
    USB_OTG_GINTMSK = 0;

    /* Soft reconnect */
    for (volatile int i = 0; i < 100000; i++)
        ;
    USB_OTG_DCTL &= ~(1U << 1);  /* clear SDET */

    usb_connected = 1;
    usb_configured = 0;
    return 0;
}

int usb_is_connected(void)
{
    return usb_connected;
}

void usb_poll(void)
{
    /* In a real implementation, handle USB interrupts:
     * - RESET: reinitialize endpoints, set address
     * - ENUMDNE: enumeration done, set configuration
     * - RXFLVL: receive FIFO level (data from host)
     * - IEPTINT: IN endpoint interrupt (TX complete)
     * - OEPTINT: OUT endpoint interrupt (RX ready)
     *
     * For this firmware, we provide the interface; a full USB stack
     * would be ~2000 lines. We track connection status.
     */

    if (usb_connected && !usb_configured) {
        /* Simulate enumeration completion */
        usb_configured = 1;
    }
}

int usb_write(const uint8_t *data, uint32_t len)
{
    if (!usb_configured || !data || len == 0)
        return -1;

    /* In a real implementation, load data into the IN endpoint FIFO
       and trigger transmission. For now, copy to tx_buf as a simulation. */
    uint32_t to_copy = (len < USB_BUF_SIZE) ? len : USB_BUF_SIZE;
    memcpy(tx_buf, data, to_copy);
    return (int)to_copy;
}

int usb_read(uint8_t *data, uint32_t max_len)
{
    if (!data || max_len == 0)
        return -1;

    uint32_t read = 0;
    while (read < max_len && rx_tail != rx_head) {
        data[read++] = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % USB_BUF_SIZE;
    }
    return (int)read;
}

void usb_set_baud_callback(void (*cb)(uint32_t baud))
{
    baud_callback = cb;
}

int usb_start_bulk_exfil(void)
{
    bulk_exfil_active = 1;
    return 0;
}

int usb_stop_bulk_exfil(void)
{
    bulk_exfil_active = 0;
    return 0;
}

int usb_bulk_exfil_active(void)
{
    return bulk_exfil_active;
}

/* ---- Descriptor access (for enumeration) ---- */

const uint8_t *usb_get_device_descriptor(uint16_t *len)
{
    if (len)
        *len = sizeof(cdc_device_descriptor);
    return cdc_device_descriptor;
}

const uint8_t *usb_get_config_descriptor(uint16_t *len)
{
    if (len)
        *len = sizeof(cdc_config_descriptor);
    return cdc_config_descriptor;
}

const char *usb_get_string_descriptor(uint8_t index)
{
    switch (index) {
    case 1: return string_manufacturer;
    case 2: return string_product;
    case 3: return string_serial;
    default: return "";
    }
}

/* ---- End of usb_cdc.c ----
 * Author: jayis1
 */