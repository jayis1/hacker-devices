/*
 * drivers/usb_cdc.c — USB-C CDC command interface (USART2 + CP2102N)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Mirrors the BLE frame protocol over USB-C CDC for wired control,
 * scenario upload, and log download. Shares the same opcode space so
 * the same companion app can connect over either transport.
 */
#include "../board.h"
#include "../registers.h"

#define USB_STX  0xA5
#define USB_ETX  0x5A

static uint8_t usb_rxbuf[USB_BUF_SZ];
static uint16_t usb_rxlen;
static uint8_t  usb_state;
static uint8_t  usb_plen;

extern uint8_t ble_crc8(const uint8_t *p, uint8_t n);
extern void ble_handle_frame(uint8_t op, const uint8_t *payload, uint8_t plen);

/* ---- TX ------------------------------------------------------------ */
static void usb_tx_raw(uint8_t c)
{
    uart_putc(USART2_BASE, c);
}

void usb_send_frame(uint8_t op, const uint8_t *payload, uint8_t plen)
{
    if (plen > 60) plen = 60;
    uint8_t crc = ble_crc8(&op, 1);
    crc = ble_crc8_update(crc, payload, plen);
    usb_tx_raw(USB_STX);
    usb_tx_raw(plen + 1);
    usb_tx_raw(op);
    for (uint8_t i = 0; i < plen; i++) usb_tx_raw(payload[i]);
    usb_tx_raw(crc);
    usb_tx_raw(USB_ETX);
}

/* A small helper to add to a CRC without re-scanning */
uint8_t ble_crc8_update(uint8_t crc, const uint8_t *p, uint8_t n)
{
    while (n--) {
        crc ^= *p++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
    return crc;
}

/* ---- RX poll ------------------------------------------------------- */
void usb_poll_rx(void)
{
    uint8_t c;
    while (uart_getc_nb(USART2_BASE, &c)) {
        switch (usb_state) {
        case 0:
            if (c == USB_STX) usb_state = 1;
            break;
        case 1:
            if (c == 0 || c > 63) { usb_state = 0; break; }
            usb_plen = c;
            usb_rxlen = 0;
            usb_state = 2;
            break;
        case 2:
            usb_rxbuf[usb_rxlen++] = c;
            if (usb_rxlen >= usb_plen) usb_state = 3;
            break;
        case 3:
            usb_rxbuf[usb_rxlen] = c;
            usb_state = 4;
            break;
        case 4:
            if (c == USB_ETX) {
                uint8_t crc = ble_crc8(usb_rxbuf, usb_plen);
                if (crc == usb_rxbuf[usb_plen]) {
                    ble_handle_frame(usb_rxbuf[0], &usb_rxbuf[1], usb_plen - 1);
                }
            }
            usb_state = 0;
            break;
        }
    }
}

/* ---- ASCII log dump ------------------------------------------------ */
void usb_log_line(const char *s)
{
    while (*s) usb_tx_raw((uint8_t)*s++);
    usb_tx_raw('\r');
    usb_tx_raw('\n');
}

/* ---- Binary scenario upload (extended opcode 0x0C) ----------------- */
/* Receives a scenario bytecode blob (up to RP_SCN_MAX_OPS bytes) over
 * multiple frames and stages it into SRAM for scenario_load_custom().
 */
static uint8_t scenario_staging[RP_SCN_MAX_OPS];
static uint16_t scenario_staging_len;

uint8_t usb_stage_scenario(const uint8_t *data, uint8_t n)
{
    if (scenario_staging_len + n > RP_SCN_MAX_OPS) return 0;
    for (uint8_t i = 0; i < n; i++)
        scenario_staging[scenario_staging_len++] = data[i];
    return 1;
}

void usb_commit_scenario(void)
{
    if (scenario_staging_len > 0) {
        scenario_load_custom(scenario_staging);
    }
}

void usb_reset_staging(void)
{
    scenario_staging_len = 0;
}

void usb_init(void)
{
    usb_state = 0;
    usb_rxlen = 0;
    usb_plen = 0;
    scenario_staging_len = 0;
    uart_init(USART2_BASE, USB_BAUD);
}