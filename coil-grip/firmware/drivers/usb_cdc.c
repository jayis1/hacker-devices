/*
 * usb_cdc.c — USB-C CDC console for CoilGrip
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * The STM32H743 USB OTG block is configured as a Communications Device
 * Class Abstract Control Model (CDC-ACM). This provides a command-line
 * console over USB-C for firmware updates, log streaming, and direct
 * control without BLE. The full USB stack is large; for the reference
 * build we model the CDC as a ring-buffered serial endpoint exposed via
 * the OTG-FS peripheral with a minimal device descriptor and bulk
 * endpoint pair. The CLI parser lives in main.c.
 */

#include "board.h"
#include "registers.h"

/* USB OTG-FS base (STM32H743) */
#define OTG_FS_BASE         0x50040000U

/* OTG register offsets (subset) */
#define OTG_GUSBCFG         0x00C
#define OTG_GINTSTS          0x014
#define OTG_GINTMSK          0x018
#define OTG_DCFG            0x800
#define OTG_DIEPCTL0         0x900
#define OTG_DIEPINT0         0x908
#define OTG_DIEPTSIZ0        0x910
#define OTG_DIEPDFIFO0       0x1000
#define OTG_DOEPCTL0         0xB00
#define OTG_DOEPINT0         0xB08
#define OTG_DOEPTSIZ0        0xB10
#define OTG_DOEPDFIFO0       0x1000

/* CDC endpoints: EP0 (control), EP1 IN (bulk TX to host), EP2 OUT (bulk RX) */
#define CDC_EP0_IN          0x00
#define CDC_EP1_IN          0x01
#define CDC_EP2_OUT         0x02

#define CDC_BUF_SZ          512U

static uint8_t g_rx_buf[CDC_BUF_SZ];
static volatile uint16_t g_rx_head = 0, g_rx_tail = 0;
static uint8_t g_tx_buf[CDC_BUF_SZ];
static volatile uint16_t g_tx_head = 0, g_tx_tail = 0;
static volatile bool g_usb_configured = false;

/* ---- Minimal CDC device descriptor (raw bytes) ------------------------- */
static const uint8_t cdc_device_desc[18] = {
    0x12, 0x01, 0x00, 0x02, 0xEF, 0x02, 0x01, 0x40,
    0x83, 0x04, 0x10, 0x57, 0x00, 0x01, 0x00, 0x02,
    0x00, 0x01
};
static const uint8_t cdc_cfg_desc[67] = {
    /* configuration descriptor (9) */
    0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,
    /* interface 0: CDC control (9) */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* functional descriptors (5+5+4+5 = 19) */
    0x05, 0x24, 0x00, 0x10, 0x01,
    0x05, 0x24, 0x01, 0x00, 0x01,
    0x04, 0x24, 0x02, 0x02,
    0x05, 0x24, 0x06, 0x00, 0x01,
    /* endpoint notification (7) */
    0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x0A,
    /* interface 1: CDC data (9) */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* endpoint 1 IN bulk (7) */
    0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,
    /* endpoint 2 OUT bulk (7) */
    0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00
};

/* ---- Ring buffer helpers ----------------------------------------------- */
static void rx_push(uint8_t c)
{
    uint16_t next = (g_rx_head + 1) % CDC_BUF_SZ;
    if (next != g_rx_tail) {
        g_rx_buf[g_rx_head] = c;
        g_rx_head = next;
    }
}

static uint8_t tx_pop(void)
{
    if (g_tx_head == g_tx_tail) return 0;
    uint8_t c = g_tx_buf[g_tx_tail];
    g_tx_tail = (g_tx_tail + 1) % CDC_BUF_SZ;
    return c;
}

/* ---- Minimal OTG init (skeleton) -------------------------------------- */
/* A full USB stack is out of scope for the reference build; this models
 * the bring-up sequence and the descriptor response path so the CLI can
 * be exercised. On real hardware you'd wire the OTG interrupts; here we
 * poll the RxFIFO in usb_cdc_poll(). */

static void otg_init(void)
{
    /* Enable OTG-FS clocks (USB1EN in AHB2ENR bit 31) */
    RCC->AHB2ENR |= BIT(31);
    /* Soft disconnect, configure as device, full-speed */
    REG32(OTG_FS_BASE + OTG_GUSBCFG) = 0x00001400U;  /* TRDT=4 */
    REG32(OTG_FS_BASE + OTG_DCFG) = 0x00002200U;      /* DSPD=full speed */
    /* Soft connect */
    REG32(OTG_FS_BASE + OTG_DCFG) &= ~BIT(31);
}

int usb_cdc_init(void)
{
    g_rx_head = g_rx_tail = 0;
    g_tx_head = g_tx_tail = 0;
    g_usb_configured = false;
    otg_init();
    return 0;
}

/* Called from the main loop; polls the OTG RxFIFO for host bytes and
 * drains our TX buffer. */
void usb_cdc_poll(void)
{
    /* In a full implementation we'd service SETUP packets here and feed
     * descriptors when the host asks for them. For the reference build
     * we treat the CDC as already configured and poll the OUT endpoint. */
    if (!g_usb_configured) {
        /* Stub: assume configured after a grace period so the CLI works
         * on boards without a real USB stack. */
        static uint32_t boot_loops = 0;
        if (++boot_loops > 1000000) g_usb_configured = true;
        return;
    }
    /* Poll OUT endpoint (EP2) for data */
    uint32_t doeptsiz = REG32(OTG_FS_BASE + OTG_DOEPTSIZ0);
    if (doeptsiz & 0x7F) {
        uint32_t fif = REG32(OTG_FS_BASE + OTG_DOEPDFIFO0);
        uint8_t b = (uint8_t)(fif & 0xFF);
        rx_push(b);
    }
    /* Drain TX buffer if any */
    while (g_tx_head != g_tx_tail) {
        uint8_t c = tx_pop();
        REG32(OTG_FS_BASE + OTG_DIEPDFIFO0) = c;
    }
}

int usb_cdc_write(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (g_tx_head + 1) % CDC_BUF_SZ;
        if (next == g_tx_tail) return -1;  /* full */
        g_tx_buf[g_tx_head] = data[i];
        g_tx_head = next;
    }
    return len;
}

int usb_cdc_read(uint8_t *buf, uint16_t buf_sz)
{
    uint16_t n = 0;
    while (n < buf_sz && g_rx_head != g_rx_tail) {
        buf[n++] = g_rx_buf[g_rx_tail];
        g_rx_tail = (g_rx_tail + 1) % CDC_BUF_SZ;
    }
    return (int)n;
}

bool usb_cdc_configured(void) { return g_usb_configured; }
const uint8_t *usb_cdc_device_desc(void) { return cdc_device_desc; }
const uint8_t *usb_cdc_cfg_desc(void) { return cdc_cfg_desc; }