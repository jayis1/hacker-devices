/*
 * aperture-phantom / firmware / drivers / usb_cdc.c
 *
 * Minimal USB OTG-HS CDC backhaul stub. On the real board this is the
 * ST USB middleware stack; here we provide the polling hook and a
 * pass-through so the same command protocol can run over USB CDC from a
 * laptop. The CDC endpoints are initialised by the middleware in
 * usb_cdc_init(); this file mainly wires the framing to the same
 * dispatcher the BLE path uses.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "board.h"

static uint8_t  g_usb_rx_buf[256];
static uint16_t g_usb_rx_len;
static uint8_t  g_usb_state;

void usb_cdc_init(void) {
    /* The real implementation calls USBD_Init() and registers the CDC
     * class. For the design reference we just zero state. */
    g_usb_rx_len = 0;
    g_usb_state = 0;
    (void)g_usb_rx_buf;
}

void usb_cdc_poll(void) {
    /* In the real build, the CDC RX callback appends bytes to g_usb_rx_buf
     * and sets g_usb_rx_len when a complete frame is present. Here we just
     * no-op to avoid blocking the main loop. */
}

int usb_cdc_tx(const uint8_t *buf, uint16_t len) {
    /* Real build: CDC_Transmit_FS(buf, len). */
    (void)buf; (void)len;
    return 0;
}

int usb_cdc_rx(uint8_t *buf, uint16_t len) {
    if (g_usb_rx_len == 0) return -1;
    uint16_t n = (g_usb_rx_len < len) ? g_usb_rx_len : len;
    for (uint16_t i = 0; i < n; i++) buf[i] = g_usb_rx_buf[i];
    g_usb_rx_len = 0;
    return (int)n;
}