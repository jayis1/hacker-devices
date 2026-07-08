/*
 * tpm-phantom — drivers/usb_cdc.c
 * Minimal USB CDC (Communication Device Class) driver over OTG-HS in
 * full-speed physical mode.
 *
 * Implements a virtual serial port so the host PC can configure the
 * phantom, stream captured transactions, and receive status updates.
 * The driver implements the minimum USB descriptors and endpoint
 * handling for a single bulk IN/OUT interface.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "usb_cdc.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================
 * USB descriptors
 * =================================================================== */
static const uint8_t g_device_desc[] = {
    0x12,        /* bLength */
    0x01,        /* bDescriptorType = Device */
    0x00, 0x02,  /* bcdUSB 2.0 */
    0x02,        /* bDeviceClass = CDC */
    0x00,        /* bDeviceSubClass */
    0x00,        /* bDeviceProtocol */
    0x40,        /* bMaxPacketSize0 = 64 */
    0x83, 0x12,  /* idVendor = 0x1283 (jayis1 assigned) */
    0x50, 0x00,  /* idProduct = 0x0050 (tpm-phantom) */
    0x01, 0x01,  /* bcdDevice 1.01 */
    0x01,        /* iManufacturer */
    0x02,        /* iProduct */
    0x03,        /* iSerialNumber */
    0x01         /* bNumConfigurations */
};

static const uint8_t g_config_desc[] = {
    /* Configuration descriptor */
    0x09, 0x02,
    0x43, 0x00,  /* wTotalLength = 67 */
    0x02,        /* bNumInterfaces = 2 (control + data) */
    0x01,        /* bConfigurationValue */
    0x00,        /* iConfiguration */
    0x80,        /* bmAttributes = bus powered */
    0x32,        /* bMaxPower = 100mA */

    /* Interface 0 — CDC control */
    0x09, 0x04,
    0x00, 0x00, 0x01,  /* iface 0, alt 0, 1 endpoint */
    0x02, 0x02, 0x01,  /* class CDC, subclass ACM, protocol */
    0x00,
    /* Header functional */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* Call management */
    0x05, 0x24, 0x01, 0x00, 0x01,
    /* ACM functional */
    0x04, 0x24, 0x02, 0x02,
    /* Union functional */
    0x05, 0x24, 0x06, 0x00, 0x01,
    /* Interrupt IN endpoint (EP1) */
    0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x0A,

    /* Interface 1 — CDC data */
    0x09, 0x04,
    0x01, 0x00, 0x02,  /* iface 1, alt 0, 2 endpoints */
    0x0A, 0x00, 0x00,  /* class CDC-data */
    0x00,
    /* Bulk IN endpoint (EP2) */
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,
    /* Bulk OUT endpoint (EP3) */
    0x07, 0x05, 0x03, 0x02, 0x40, 0x00, 0x00
};

static const uint8_t g_string0[] = { 0x04, 0x03, 0x09, 0x04 };  /* English */
static const char g_str_mfr[]  = "jayis1";
static const char g_str_prod[] = "tpm-phantom TPM Bus Analyzer";
static const char g_str_ser[]  = "TPM-PHANTOM-001";

/* ===================================================================
 * State
 * =================================================================== */
static volatile uint8_t g_usb_configured = 0;
static volatile uint8_t g_usb_address = 0;
static volatile uint8_t g_rx_buf[64];
static volatile uint8_t g_rx_len = 0;
static volatile uint8_t g_rx_ready = 0;

static uint8_t g_tx_buf[256];
static volatile uint16_t g_tx_len = 0;

/* ===================================================================
 * Helper: encode string descriptor
 * =================================================================== */
static uint8_t make_string_desc(const char *ascii, uint8_t *out, uint8_t max)
{
    uint8_t len = (uint8_t)strlen(ascii);
    uint8_t total = 2 + len * 2;
    if (total > max) total = max;
    out[0] = total;
    out[1] = 0x03;
    for (uint8_t i = 0; i < len && (2 + i * 2) < max; i++) {
        out[2 + i * 2] = (uint8_t)ascii[i];
        out[3 + i * 2] = 0;
    }
    return total;
}

/* ===================================================================
 * Initialize USB OTG-HS in device mode (FS physical)
 * =================================================================== */
void usb_cdc_init(void)
{
    /* Enable GPIOA and USB OTG HS clock */
    SET_BIT(RCC_AHB4ENR, BIT(0));   /* GPIOA */
    SET_BIT(RCC_AHB1ENR, BIT(7));  /* USB OTG HS */

    /* PA11 (DM), PA12 (DP) — AF10 (OTG_FS) */
    gpio_set_mode(USB_DM_PORT, USB_DM_PIN, GPIO_MODE_AF);
    gpio_set_af(USB_DM_PORT, USB_DM_PIN, 10);
    gpio_set_speed(USB_DM_PORT, USB_DM_PIN, GPIO_SPEED_VHIGH);
    gpio_set_mode(USB_DP_PORT, USB_DP_PIN, GPIO_MODE_AF);
    gpio_set_af(USB_DP_PORT, USB_DP_PIN, 10);
    gpio_set_speed(USB_DP_PORT, USB_DP_PIN, GPIO_SPEED_VHIGH);

    /* Reset OTG */
    USB_OTG_GRSTCTL = BIT(0);  /* CSRST */
    while (USB_OTG_GRSTCTL & BIT(0)) ;

    /* Configure for device mode, FS */
    USB_OTG_GUSBCFG = 0x00004000;  /* FDMOD (force device) */
    /* Power down PHY, use FS */
    USB_OTG_GUSBCFG |= BIT(30);    /* PHYSEL */
    /* Soft disconnect */
    USB_OTG_DCTL = BIT(1);         /* SDIS */
    for (volatile int i = 0; i < 100000; i++) __asm__("nop");
    USB_OTG_DCTL = 0;              /* connect */

    /* Enable interrupts: reset, suspend, resume, SOF, EP interrupt */
    USB_OTG_GINTMSK = BIT(0) | BIT(3) | BIT(10) | BIT(11) | BIT(12) | BIT(4);

    g_usb_configured = 0;
    g_rx_ready = 0;
    g_rx_len = 0;
}

/* ===================================================================
 * Handle GET_DESCRIPTOR
 * =================================================================== */
static void usb_handle_descriptor(uint16_t wValue, uint16_t wLength)
{
    uint8_t type = (wValue >> 8) & 0xFF;
    uint8_t idx = wValue & 0xFF;
    const uint8_t *p = 0;
    uint16_t len = 0;

    switch (type) {
    case 0x01:  /* Device */
        p = g_device_desc;
        len = sizeof(g_device_desc);
        break;
    case 0x02:  /* Configuration */
        p = g_config_desc;
        len = sizeof(g_config_desc);
        break;
    case 0x03:  /* String */
        if (idx == 0) { p = g_string0; len = 4; }
        else {
            static uint8_t str_buf[64];
            const char *s = 0;
            if (idx == 1) s = g_str_mfr;
            else if (idx == 2) s = g_str_prod;
            else if (idx == 3) s = g_str_ser;
            if (s) { len = make_string_desc(s, str_buf, sizeof(str_buf)); p = str_buf; }
        }
        break;
    default:
        break;
    }
    if (p && len > wLength) len = wLength;
    /* TX descriptor on EP0 — simplified: write to FIFO */
    if (p && len) {
        memcpy(g_tx_buf, p, len);
        g_tx_len = len;
    }
}

/* ===================================================================
 * USB interrupt handler (OTG-HS global IRQ #77)
 * =================================================================== */
void OTG_HS_IRQHandler(void)
{
    uint32_t sts = USB_OTG_GINTSTS;
    if (sts & BIT(0)) {
        /* USB reset */
        USB_OTG_GINTSTS = BIT(0);
        g_usb_configured = 0;
        g_usb_address = 0;
    }
    if (sts & BIT(11)) {
        /* Suspend */
        USB_OTG_GINTSTS = BIT(11);
    }
    if (sts & BIT(12)) {
        /* Resume */
        USB_OTG_GINTSTS = BIT(12);
    }
}

/* ===================================================================
 * Send data on CDC bulk IN (EP2)
 * Simplified: copies to TX buffer; real impl writes OTG FIFO.
 * =================================================================== */
uint16_t usb_cdc_send(const uint8_t *data, uint16_t len)
{
    if (!g_usb_configured)
        return 0;
    if (len > sizeof(g_tx_buf))
        len = sizeof(g_tx_buf);
    memcpy(g_tx_buf, data, len);
    g_tx_len = len;
    /* Real implementation: write to OTG FIFO EP2 IN, set DTOG */
    return len;
}

/* ===================================================================
 * Receive data from CDC bulk OUT (EP3)
 * Non-blocking; returns 0 if no data, else copies into buf.
 * =================================================================== */
uint16_t usb_cdc_recv(uint8_t *buf, uint16_t max)
{
    if (!g_rx_ready || !g_usb_configured)
        return 0;
    uint16_t n = g_rx_len > max ? max : g_rx_len;
    memcpy(buf, (const void *)g_rx_buf, n);
    g_rx_ready = 0;
    return n;
}

uint8_t usb_cdc_configured(void) { return g_usb_configured; }

void usb_cdc_set_configured(uint8_t v) { g_usb_configured = v; }

/* ===================================================================
 * Poll USB (call from main loop to handle EP0 SETUP)
 * In a minimal implementation we process any pending RX.
 * =================================================================== */
void usb_cdc_poll(void)
{
    /* Check if a SETUP packet arrived (simplified) */
    /* Real OTG: read DOEPCTL0, parse 8-byte SETUP, dispatch */
    if (g_rx_ready) {
        /* Already handled by interrupt */
    }
}