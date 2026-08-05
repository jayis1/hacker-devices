/*
 * usb_cdc.c — USB CDC communication driver
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements a minimal USB CDC (Communications Device Class) virtual
 * serial port over the STM32H723 USB OTG peripheral. This provides
 * the primary communication channel between the device and the host
 * (Android phone or PC).
 *
 * The driver uses a simple ring buffer for RX and a straightforward
 * TX path. The USB enumeration and endpoint handling is simplified
 * for readability — a real implementation would use the full STM32
 * USB device library or a lightweight alternative like TinyUSB.
 */

#include "usb_cdc.h"
#include "../registers.h"

/* ---- Private state ------------------------------------------------- */

#define USB_RX_BUF_SIZE 1024
#define USB_TX_BUF_SIZE 1024

static uint8_t  g_rx_buf[USB_RX_BUF_SIZE];
static volatile uint16_t g_rx_head;
static volatile uint16_t g_rx_tail;

static uint8_t  g_tx_buf[USB_TX_BUF_SIZE];
static volatile uint16_t g_tx_head;

static volatile bool g_connected;

/* ---- Initialization ------------------------------------------------ */

void usb_cdc_init(void)
{
    /*
     * Enable USB OTG peripheral.
     * A full USB CDC implementation requires:
     *  1. Configure USB GPIO pins (PA11 = D-, PA12 = D+)
     *  2. Enable USB power transceiver
     *  3. Configure endpoints (EP0 for control, EP1 for CDC bulk)
     *  4. Set device descriptors (VID, PID, strings)
     *  5. Handle enumeration interrupts
     *
     * This is a simplified skeleton — the actual USB stack would
     * be provided by the STM32 HAL USB device library or TinyUSB.
     */

    /* Configure PA11 (D-) and PA12 (D+) as AF10 (USB OTG) */
    gpio_config(GPIOA, 11, GPIO_MODE_AF, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 10);
    gpio_config(GPIOA, 12, GPIO_MODE_AF, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 10);

    /* Enable USB OTG */
    USB_OTG_GAHBCFG |= BIT(0); /* Global interrupt enable */

    /* Soft reset */
    USB_OTG_GRSTCTL |= BIT(0); /* AHB Master reset */
    while (USB_OTG_GRSTCTL & BIT(0))
        ;

    /* Enable USB */
    USB_OTG_GUSBCFG |= BIT(0); /* Force device mode */

    /* Enable global interrupts */
    USB_OTG_GINTMSK |= BIT(12); /* USB reset interrupt */
    USB_OTG_GINTMSK |= BIT(11); /* Enumeration done */

    g_rx_head = 0;
    g_rx_tail = 0;
    g_tx_head = 0;
    g_connected = false;

    /*
     * NOTE: A complete USB CDC stack requires handling the SETUP
     * token for enumeration (GET_DESCRIPTOR, SET_ADDRESS, SET_CONFIG)
     * and managing the bulk endpoints for data transfer. This skeleton
     * outlines the initialization; the protocol layer (protocol.c)
     * would call usb_cdc_rx/tx to send and receive command frames.
     */
}

/* ---- Poll (called from main loop) ---------------------------------- */

void usb_cdc_poll(void)
{
    /*
     * In a real implementation, this would:
     *  1. Check for USB reset / enumeration events
     *  2. Handle OUT endpoint (RX) data
     *  3. Handle IN endpoint (TX) completion
     *
     * For now, we simulate connected state.
     */
    /* Check if USB is configured (simplified) */
    if (USB_OTG_GINTSTS & BIT(12)) {
        /* USB reset */
        USB_OTG_GINTSTS = BIT(12); /* clear */
        g_connected = false;
    }
    if (USB_OTG_GINTSTS & BIT(11)) {
        /* Enumeration done */
        USB_OTG_GINTSTS = BIT(11); /* clear */
        g_connected = true;
    }
}

/* ---- RX / TX ------------------------------------------------------- */

uint32_t usb_cdc_rx(uint8_t *buf, uint32_t max_len)
{
    uint32_t count = 0;
    while (count < max_len && g_rx_tail != g_rx_head) {
        buf[count] = g_rx_buf[g_rx_tail];
        g_rx_tail = (g_rx_tail + 1) % USB_RX_BUF_SIZE;
        count++;
    }
    return count;
}

void usb_cdc_tx(const uint8_t *buf, uint32_t len)
{
    /*
     * Copy data to the TX buffer. In a real implementation, this
     * would fill the USB IN endpoint FIFO and trigger transmission.
     */
    for (uint32_t i = 0; i < len && g_tx_head < USB_TX_BUF_SIZE; i++) {
        g_tx_buf[g_tx_head] = buf[i];
        g_tx_head++;
    }

    /* Trigger IN endpoint transmission (simplified) */
    /* In a real stack: USB_OTG_DIEPTSIZ1 = len; USB_OTG_DIEPCTL1 |= EPENA; */
}

bool usb_cdc_connected(void)
{
    return g_connected;
}

/* ---- USB descriptors (reference) ----------------------------------- */

/*
 * In a real build, these descriptors would be defined here:
 *
 * Device descriptor: VID=0x1209 (pid.codes), PID=0xPLASMA
 *   (PlasmaReaper USB CDC PID would be allocated from pid.codes)
 * Configuration descriptor: CDC class, 1 bulk IN + 1 bulk OUT endpoint
 * String descriptors: Manufacturer="jayis1", Product="PlasmaReaper"
 *   Serial = unique device ID from MCU UID register
 */

/* ---- End of file --------------------------------------------------- */