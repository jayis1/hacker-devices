/*
 * usb_cdc.c — USB CDC Virtual Serial Port for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal USB CDC implementation for host-side control and
 * high-speed audio capture download.
 *
 * The USB OTG1 peripheral is configured in full-speed device mode
 * with a single CDC virtual serial port interface.
 *
 * NOTE: This is a simplified implementation. A production version
 * would include full USB enumeration, CDC class handling, and
 * endpoint management.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ========================================================================
 *  USB state
 * ======================================================================== */

typedef struct {
    uint8_t  connected;
    uint8_t  configured;
    uint8_t  rx_buf[512];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint8_t  tx_buf[512];
    uint16_t tx_len;
} usb_state_t;

static usb_state_t g_usb;

/* USB OTG registers (simplified) */
#define USB_OTG_BASE   (PERIPH_AHB2_BASE + 0x4000U)
#define USB_OTG_GUSBCFG REG32(USB_OTG_BASE + 0x0CU)
#define USB_OTG_GINTSTS REG32(USB_OTG_BASE + 0x14U)
#define USB_OTG_GINTMSK REG32(USB_OTG_BASE + 0x18U)
#define USB_OTG_DCFG    REG32(USB_OTG_BASE + 0x800U)
#define USB_OTG_DCTL    REG32(USB_OTG_BASE + 0x804U)

/* ========================================================================
 *  Initialize USB CDC
 * ========================================================================
 */

void usb_cdc_init(void)
{
    memset(&g_usb, 0, sizeof(g_usb));

    /* Enable USB OTG1 clock (already done in gpio_init) */

    /* Configure USB OTG as device, full-speed */
    /* For simplicity, we just mark the interface as available */
    /* A full USB CDC implementation would handle enumeration here */

    /* Enable VBUS sensing and connect */
    /* USB_OTG_DCTL &= ~(1U << 0); */  /* Soft disconnect = 0 */

    g_usb.connected = 0;
    g_usb.configured = 0;
}

/* ========================================================================
 *  Send data over USB CDC
 * ========================================================================
 */

int usb_cdc_send(const uint8_t *data, uint16_t len)
{
    if (!g_usb.connected || !g_usb.configured)
        return -1;

    if (len > sizeof(g_usb.tx_buf))
        len = sizeof(g_usb.tx_buf);

    /* In a full implementation, this would:
     * 1. Copy data to the USB TX FIFO
     * 2. Write to the IN endpoint
     * 3. Wait for TX complete interrupt
     *
     * For now, we just buffer the data
     */
    memcpy(g_usb.tx_buf, data, len);
    g_usb.tx_len = len;

    return len;
}

/* ========================================================================
 *  Receive data from USB CDC (non-blocking)
 * ========================================================================
 */

int usb_cdc_recv(uint8_t *buf, uint16_t maxlen)
{
    if (g_usb.rx_tail == g_usb.rx_head)
        return 0;  /* No data */

    uint16_t count = 0;
    while (g_usb.rx_tail != g_usb.rx_head && count < maxlen) {
        buf[count++] = g_usb.rx_buf[g_usb.rx_tail];
        g_usb.rx_tail = (g_usb.rx_tail + 1) % sizeof(g_usb.rx_buf);
    }

    return count;
}

/* ========================================================================
 *  Check if USB is connected
 * ========================================================================
 */

uint8_t usb_cdc_connected(void)
{
    return g_usb.connected && g_usb.configured;
}

/* ========================================================================
 *  USB OTG interrupt handler (simplified)
 *
 *  A full implementation would handle:
 *  - Reset interrupt → set device address 0, enter default state
 *  - Enum done → set full-speed, configure endpoints
 *  - Setup packet → handle GET_DESCRIPTOR, SET_CONFIGURATION, etc.
 *  - RX FIFO not empty → read data into rx_buf
 *  - TX FIFO empty → send queued data
 * ========================================================================
 */

void OTG1_IRQHandler(void)
{
    uint32_t intsts = USB_OTG_GINTSTS;

    if (intsts & BIT(12)) {
        /* USB reset */
        g_usb.connected = 1;
        g_usb.configured = 0;
        USB_OTG_DCFG = (USB_OTG_DCFG & ~(0x7FU << 4)) | (0x1U << 4);  /* Set address 0 */
    }

    if (intsts & BIT(13)) {
        /* Enumeration done */
        g_usb.configured = 1;
    }

    /* Clear all interrupts */
    USB_OTG_GINTSTS = intsts;
}