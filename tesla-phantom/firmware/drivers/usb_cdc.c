/*
 * tesla-phantom — drivers/usb_cdc.c
 * USB CDC virtual serial port for wired command/data interface.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* STM32H743 USB OTG FS (full-speed, 12 Mbps).
 * We implement a minimal CDC (Communications Device Class) device
 * that appears as a virtual serial port on the host.
 *
 * The USB peripheral is complex; this driver provides a minimal
 * interface with interrupt-driven RX and polling TX.
 */

/* USB OTG FS base address */
#define USB_OTG_FS_BASE   0x50080000UL

/* USB OTG registers (simplified — full implementation needs many more) */
typedef struct {
    volatile uint32_t GOTGCTL;   /* 0x000 */
    volatile uint32_t GOTGINT;   /* 0x004 */
    volatile uint32_t GAHBCFG;   /* 0x008 */
    volatile uint32_t GUSBCFG;   /* 0x00C */
    volatile uint32_t GRSTCTL;   /* 0x010 */
    volatile uint32_t GINTSTS;   /* 0x014 */
    volatile uint32_t GINTMSK;   /* 0x018 */
    volatile uint32_t GRXSTSR;   /* 0x01C */
    volatile uint32_t GRXSTSP;   /* 0x020 */
    volatile uint32_t GRXFSIZ;   /* 0x024 */
    volatile uint32_t DIEPTXF0;  /* 0x028 */
    volatile uint32_t HNPTXFSIZ; /* 0x028 (same offset) */
} usb_otg_regs_t;

#define USB_OTG_FS  ((usb_otg_regs_t *)USB_OTG_FS_BASE)

/* USB endpoint state */
#define USB_EP0_RX_BUF_SIZE  64
static uint8_t usb_rx_buf[USB_EP0_RX_BUF_SIZE];
static volatile uint16_t usb_rx_len = 0;
static volatile uint8_t  usb_configured = 0;

/* USB CDC device descriptor (minimal) */
static const uint8_t dev_descriptor[] = {
    18,                           /* bLength */
    0x01,                         /* bDescriptorType (Device) */
    0x10, 0x01,                   /* bcdUSB 1.10 */
    0x02,                         /* bDeviceClass (CDC) */
    0x00,                         /* bDeviceSubClass */
    0x00,                         /* bDeviceProtocol */
    64,                           /* bMaxPacketSize0 */
    0x83, 0x04,                   /* idVendor (0x0483 = ST) */
    0x40, 0x57,                   /* idProduct (0x5740 = CDC) */
    0x00, 0x02,                   /* bcdDevice 2.00 */
    1,                            /* iManufacturer */
    2,                            /* iProduct */
    3,                            /* iSerialNumber */
    1                             /* bNumConfigurations */
};

/* ── Public API ───────────────────────────────────────────────────── */

int usb_init(void) {
    /* Enable USB OTG FS clock */
    volatile uint32_t *rcc_ahb1enr = (volatile uint32_t *)(RCC_BASE + 0x48 + 0x0C);
    *rcc_ahb1enr |= (1U << 25);  /* USB OTG FS */

    /* Enable USB power */
    volatile uint32_t *pwr_cr3 = (volatile uint32_t *)(PWR_BASE + 0x0C);
    *pwr_cr3 |= (1U << 25);  /* USB33DEN */

    delay_ms(10);

    /* Reset USB OTG FS core */
    USB_OTG_FS->GRSTCTL = (1U << 0);  /* CSRST */
    while (USB_OTG_FS->GRSTCTL & (1U << 0)) { }

    /* Configure USB OTG as device mode */
    USB_OTG_FS->GUSBCFG = (1U << 30)   /* FDMOD = device mode */
                        | (6U << 10);  /* TRDT = 6 for full-speed */

    /* Enable VBUS sensing */
    USB_OTG_FS->GOTGCTL = (1U << 16);  /* VBVALOEN */

    /* Unmask USB interrupt (global) */
    USB_OTG_FS->GAHBCFG = (1U << 0);   /* GINTMSK */

    /* Minimal setup — a full CDC implementation requires significant
     * additional code for endpoint configuration, enumeration handling,
     * and data transfer. This stub provides the basic interface. */
    usb_configured = 0;
    usb_rx_len = 0;

    return 0;
}

int usb_send(const void *data, uint16_t len) {
    if (!usb_configured || len == 0) return 0;

    /* Transmit via IN endpoint (simplified — would use USB OTG FIFO) */
    const uint8_t *p = (const uint8_t *)data;
    for (uint16_t i = 0; i < len; i++) {
        /* In a real implementation, write to USB FIFO */
        (void)p[i];
    }

    return len;
}

int usb_recv(void *buf, uint16_t maxlen) {
    if (usb_rx_len == 0) return 0;

    uint16_t copy = (usb_rx_len < maxlen) ? usb_rx_len : maxlen;
    uint8_t *p = (uint8_t *)buf;
    for (uint16_t i = 0; i < copy; i++)
        p[i] = usb_rx_buf[i];

    usb_rx_len = 0;
    return (int)copy;
}

void usb_process(void) {
    /* Process USB events — in a full implementation this would
     * handle enumeration, endpoint setup, and data transfer. */
    /* Check for USB reset / enumeration events */
    if (USB_OTG_FS->GINTSTS & (1U << 12)) {
        /* USB reset detected */
        USB_OTG_FS->GINTSTS = (1U << 12);  /* clear */
        usb_configured = 0;
    }

    /* Check for enumeration done */
    if (USB_OTG_FS->GINTSTS & (1U << 13)) {
        USB_OTG_FS->GINTSTS = (1U << 13);  /* clear */
        usb_configured = 1;
    }

    /* Check for RX data available */
    if (USB_OTG_FS->GINTSTS & (1U << 4)) {
        /* RX FIFO non-empty — read status */
        uint32_t status = USB_OTG_FS->GRXSTSP;
        uint8_t ep = (status >> 0) & 0x0F;
        uint16_t bytes = (status >> 4) & 0x7FF;
        (void)ep;

        if (bytes > 0 && bytes <= USB_EP0_RX_BUF_SIZE) {
            /* Read data from RX FIFO (simplified) */
            for (uint16_t i = 0; i < bytes; i++) {
                if (i < USB_EP0_RX_BUF_SIZE) {
                    /* Read from USB FIFO — actual register access omitted */
                    usb_rx_buf[usb_rx_len++] = 0;  /* placeholder */
                }
            }
        }
    }
}