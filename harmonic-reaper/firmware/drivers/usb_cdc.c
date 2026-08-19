/*
 * usb_cdc.c — USB CDC log streaming + DFU trigger
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The nRF52840 has a built-in USB device peripheral. We use it as a
 * CDC ACM virtual serial port for log streaming and as a DFU trigger
 * (writing "DFU\n" to the CDC port reboots into the bootloader).
 *
 * This file implements a minimal CDC interface. The full USB descriptor
 * and endpoint handling would use Nordic's USBD driver; we provide the
 * application-layer ring buffer and the CDC write primitive that the
 * rest of the firmware uses.
 */
#include "../registers.h"
#include "../board.h"
#include "usb_cdc.h"

#define CDC_BUF_LEN 512
static uint8_t  cdc_tx_buf[CDC_BUF_LEN];
static volatile uint16_t cdc_tx_head, cdc_tx_tail;
static uint8_t  cdc_rx_buf[64];
static volatile uint8_t  cdc_rx_head, cdc_rx_tail;
static uint8_t  usb_ready = 0;

void usb_cdc_init(void)
{
    /* Enable USB power detection */
    if (POWER_USBREGSTATUS & POWER_USBREG_USB_POWER_OK) {
        usb_ready = 1;
    }
    cdc_tx_head = cdc_tx_tail = 0;
    cdc_rx_head = cdc_rx_tail = 0;
}

void usb_cdc_write(const uint8_t *data, uint16_t len)
{
    if (!usb_ready) return;
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (cdc_tx_head + 1) & (CDC_BUF_LEN - 1);
        if (next == cdc_tx_tail) break;  /* buffer full — drop */
        cdc_tx_buf[cdc_tx_head] = data[i];
        cdc_tx_head = next;
    }
}

void usb_cdc_pump(void)
{
    if (!usb_ready) return;

    /* In a full implementation, this would:
     *   1. Check if the USB CDC bulk IN endpoint is ready.
     *   2. Copy bytes from cdc_tx_buf to the endpoint FIFO.
     *   3. Trigger the endpoint send.
     *   4. Check the bulk OUT endpoint for received data.
     *   5. Scan received data for "DFU\n" to trigger bootloader.
     *
     * We stub the endpoint access here — the Nordic USBD driver
     * provides the actual FIFO access. This pump runs every 5 ms.
     */
    if (cdc_tx_head != cdc_tx_tail) {
        /* Drain a chunk to the USB IN endpoint (max 64 bytes per packet) */
        uint16_t count = 0;
        while (cdc_tx_head != cdc_tx_tail && count < 64) {
            /* Would write cdc_tx_buf[cdc_tx_tail] to USBD EP IN FIFO */
            cdc_tx_tail = (cdc_tx_tail + 1) & (CDC_BUF_LEN - 1);
            count++;
        }
        /* Trigger USBD EP IN send — Nordic driver call */
    }

    /* Check for received data (DFU command) */
    uint8_t rx[4];
    uint8_t rx_len = 0;
    /* Would read from USBD EP OUT FIFO into rx[] */
    if (rx_len >= 4) {
        if (rx[0] == 'D' && rx[1] == 'F' && rx[2] == 'U' && rx[3] == '\n') {
            /* Trigger DFU reboot — write to GPREGRET and reset */
            REG32(0x4000551Cu) = 0x42u;  /* GPREGRET = bootloader magic */
            /* System reset */
            REG32(0x40005504u) = 1u;    /* reset via AIRCR.SYSRESETREQ */
        }
    }
}

/* EOF — usb_cdc.c — jayis1 */