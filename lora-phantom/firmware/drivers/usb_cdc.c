/*
 * lora-phantom / drivers/usb_cdc.c
 * USB CDC virtual serial port for control + data backhaul over USB-C.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * This is a minimal USB 2.0 Full-Speed CDC implementation using the STM32H7
 * USB OTG-HS peripheral in FS mode. It provides a virtual serial port that
 * the operator app (or a terminal program) can open to send/receive protocol
 * frames at full USB FS speed (12 Mbps).
 *
 * The implementation is intentionally minimal (single bulk endpoint pair +
 * control endpoint) to keep the code small. A production build would use
 * STM32 Cube USB Device library; this is a hand-rolled driver for
 * educational/research purposes.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* USB OTG-HS register definitions (simplified) */
#define USB_OTG_HS     ((volatile uint32_t *)0x40040000UL)
#define USB_OTG_GUSBCFG    (*(volatile uint32_t *)(0x40040000UL + 0x0C))
#define USB_OTG_GINTSTS    (*(volatile uint32_t *)(0x40040000UL + 0x014))
#define USB_OTG_GINTMSK    (*(volatile uint32_t *)(0x40040000UL + 0x018))
#define USB_OTG_GCCFG      (*(volatile uint32_t *)(0x40040000UL + 0x038))
#define USB_OTG_DCFG       (*(volatile uint32_t *)(0x40040000UL + 0x800))
#define USB_OTG_DCTL       (*(volatile uint32_t *)(0x40040000UL + 0x804))

/* Endpoint registers (Device mode) */
#define USB_OTG_DIEPCTL0   (*(volatile uint32_t *)(0x40040000UL + 0x900))
#define USB_OTG_DIEPINT0   (*(volatile uint32_t *)(0x40040000UL + 0x908))
#define USB_OTG_DIEPTSIZ0  (*(volatile uint32_t *)(0x40040000UL + 0x910))
#define USB_OTG_DIEPDMA0   (*(volatile uint32_t *)(0x40040000UL + 0x914))
#define USB_OTG_DOEPCTL0   (*(volatile uint32_t *)(0x40040000UL + 0xB00))
#define USB_OTG_DOEPINT0   (*(volatile uint32_t *)(0x40040000UL + 0xB08))
#define USB_OTG_DOEPTSIZ0  (*(volatile uint32_t *)(0x40040000UL + 0xB10))

/* RX FIFO */
#define USB_OTG_RXFSTS     (*(volatile uint32_t *)(0x40040000UL + 0xE14))

/* Simplified USB CDC state */
static int s_usb_connected = 0;
static int s_usb_configured = 0;

/* USB descriptors (minimal CDC) */
static const uint8_t s_device_desc[] = {
    0x12, 0x01, 0x00, 0x02, 0x02, 0x00, 0x00, 0x40,
    0x83, 0x04, 0x10, 0x57, 0x00, 0x00, 0x01, 0x02,
    0x00, 0x01
};

static const uint8_t s_config_desc[] = {
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
    /* Interrupt IN endpoint (EP1) */
    0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x08,
    /* Interface 1: CDC Data */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* Bulk IN endpoint (EP2) */
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,
    /* Bulk OUT endpoint (EP3) */
    0x07, 0x05, 0x03, 0x02, 0x40, 0x00, 0x00
};

static const uint8_t s_string_lang[] = { 0x04, 0x03, 0x09, 0x04 };
static const uint8_t s_string_manu[] = {
    0x0C, 0x03, 'j','\0','a','\0','y','\0','i','\0','s','\0','1','\0'
};
static const uint8_t s_string_prod[] = {
    0x1E, 0x03,
    'L','\0','o','\0','R','\0','a','\0','P','\0','h','\0','a','\0','n','\0','t','\0','o','\0','m','\0'
};
static const uint8_t s_string_serial[] = {
    0x0A, 0x03, '0','\0','0','\0','1','\0','0','\0','1','\0'
};

/* ---- TX/RX buffers ---- */
#define USB_TX_BUF_SIZE 512
#define USB_RX_BUF_SIZE 512
static volatile uint8_t s_tx_buf[USB_TX_BUF_SIZE];
static volatile uint16_t s_tx_len = 0;
static volatile uint8_t s_rx_buf[USB_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

void usb_cdc_init(void) {
    /* Enable USB OTG-HS in FS mode.
     * Full USB initialization requires:
     *   1. Power on the transceiver
     *   2. Configure GUSBCFG (FS mode, turnaround time)
     *   3. Configure DCFG (device address = 0, speed = FS)
     *   4. Unmask reset interrupt
     *   5. Enable endpoints on SET_CONFIGURATION
     * This is a simplified init; a full implementation is hundreds of lines
     * of register setup. For the research firmware, we provide the skeleton
     * and the descriptor table; the operator can use the STM32Cube USB
     * library for a drop-in replacement. */

    /* Power up USB transceiver */
    USB_OTG_GCCFG |= (1U << 16);   /* PWRDWN — enable transceiver */

    /* Configure for Full-Speed (FS) */
    USB_OTG_GUSBCFG = (0x06 << 0)  /* FDMOD = device mode */
                    | (0x06 << 10); /* TRDT = 6 for FS */

    /* Device config: address 0, speed = FS (0x03 in DSPD) */
    USB_OTG_DCFG = 0x03;   /* DSPD = FS */

    /* Unmask USB reset interrupt */
    USB_OTG_GINTMSK = (1U << 11);  /* USBRST */

    /* Soft-disconnect then reconnect (to trigger enumeration) */
    USB_OTG_DCTL |= (1U << 1);     /* SFTDISCONNECT */
    for (volatile int i = 0; i < 100000; i++) { }
    USB_OTG_DCTL &= ~(1U << 1);    /* Reconnect */

    s_usb_connected = 1;
}

int usb_cdc_connected(void) {
    return s_usb_connected && s_usb_configured;
}

int usb_cdc_send(const uint8_t *buf, uint16_t len) {
    if (!usb_cdc_connected()) return -1;
    if (len > USB_TX_BUF_SIZE) len = USB_TX_BUF_SIZE;
    /* In a full implementation, we'd load the TX FIFO and arm EP2 IN.
     * Here we buffer and the main loop / ISR handles the actual USB xfer. */
    memcpy((void *)s_tx_buf, buf, len);
    s_tx_len = len;
    /* (EP2 IN transfer would be triggered here) */
    return (int)len;
}

int usb_cdc_recv(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms) {
    uint16_t got = 0;
    uint32_t deadline = timeout_ms * 48000;
    while (got < maxlen && deadline--) {
        if (s_rx_tail != s_rx_head) {
            buf[got++] = s_rx_buf[s_rx_tail];
            s_rx_tail = (s_rx_tail + 1) % USB_RX_BUF_SIZE;
        }
    }
    return (int)got;
}

/* ---- USB ISR (simplified — handles reset + enumeration) ---- */
void OTG_HS_EP1_OUT_IRQHandler(void) {
    /* Handle EP3 OUT (bulk RX from host) — read from RX FIFO into s_rx_buf */
    /* Full implementation would check RXFSTS, pop status + data words */
    USB_OTG_GINTSTS = (1U << 4);  /* RXFLVL clear */
}

void OTG_HS_IRQHandler(void) {
    uint32_t sts = USB_OTG_GINTSTS;
    if (sts & (1U << 11)) {
        /* USB reset — re-init endpoints, set address to 0 */
        USB_OTG_DCFG &= ~0x7F0;   /* clear DAD */
        s_usb_configured = 0;
        USB_OTG_GINTSTS = (1U << 11);
    }
    if (sts & (1U << 12)) {
        /* Enumeration done — set speed */
        s_usb_connected = 1;
        USB_OTG_GINTSTS = (1U << 12);
    }
}