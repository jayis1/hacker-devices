/*
 * aperture-phantom / firmware / drivers / ble_uart.c
 *
 * Framed binary command protocol carried over the BLE module's UART
 * console (EFR32BG22 NCP mode). The module transparently bridges a GATT
 * characteristic to the UART, so the MCU sees a serial byte stream that
 * we delimit with STX/ETX and protect with CRC8. Outgoing notifications
 * (status, log lines, frame chunks) use the same framing.
 *
 * Format: [STX=0xAA][LEN(2)][OP][PAYLOAD..][CRC8][ETX=0x55]
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"

extern volatile uint32_t g_ticks_ms;

static uint8_t  g_rx_buf[256];
static uint16_t g_rx_len;
static uint8_t  g_rx_state;   /* 0=idle,1=gotSTX,2=gotLEN,3=payload,4=crc */
static uint16_t g_rx_expect;
static uint8_t  g_rx_op;
static uint8_t  g_rx_crc;

void ble_init(void) {
    uart_init(115200);
    g_rx_state = 0; g_rx_len = 0;
    /* Send a module "boot" line so the operator knows the link is up. */
    uart_puts("BLE: aperture-phantom ready (jayis1)\r\n");
}

static void ble_tx_frame(uint8_t op, const uint8_t *payload, uint16_t len) {
    uint8_t hdr[5];
    hdr[0] = CMD_STX;
    hdr[1] = (uint8_t)(len >> 8);
    hdr[2] = (uint8_t)(len & 0xFF);
    hdr[3] = op;
    /* CRC over [LEN(2) || OP || payload] */
    uint8_t crc = crc8(&hdr[1], 3);
    if (len) crc = crc8(payload, len) ^ crc;
    hdr[4] = crc;
    uart_write(hdr, 5);
    if (len) uart_write(payload, len);
    uint8_t etx = CMD_ETX;
    uart_write(&etx, 1);
}

void ble_send_cmd(uint8_t op, const uint8_t *payload, uint16_t len) {
    ble_tx_frame(op, payload, len);
}

void ble_send_frame(const uint8_t *data, uint16_t len, uint8_t dt) {
    /* Frame chunk: [dt][seq16][len16][data] as a CMD_FRAME payload. */
    static uint16_t seq;
    uint8_t hdr[5];
    hdr[0] = dt;
    hdr[1] = (uint8_t)(seq >> 8); hdr[2] = (uint8_t)(seq & 0xFF);
    hdr[3] = (uint8_t)(len >> 8); hdr[4] = (uint8_t)(len & 0xFF);
    seq++;
    /* send as CMD_FRAME with combined payload */
    uint8_t op = CMD_FRAME;
    uint8_t hdr2[5]; memcpy(hdr2, hdr, 5);
    uint8_t crc = crc8(hdr2, 5) ^ (data ? crc8(data, len) : 0);
    uint8_t stx = CMD_STX;
    uart_write(&stx, 1);
    uint16_t total = 5u + len;
    uint8_t lh[2] = { (uint8_t)(total >> 8), (uint8_t)(total & 0xFF) };
    uart_write(lh, 2);
    uart_write(&op, 1);
    uart_write(hdr2, 5);
    if (len) uart_write(data, len);
    uart_write(&crc, 1);
    uint8_t etx = CMD_ETX;
    uart_write(&etx, 1);
}

void ble_notify_log(const char *s) {
    uint16_t n = 0; while (s[n] && n < 200) n++;
    ble_tx_frame(CMD_LOG, (const uint8_t *)s, n);
}

void ble_notify_status(uint32_t status, uint32_t frame_count,
                       uint32_t crc_errs, uint8_t mode, uint8_t armed) {
    uint8_t p[14];
    p[0] = (uint8_t)(status >> 24); p[1] = (uint8_t)(status >> 16);
    p[2] = (uint8_t)(status >> 8);  p[3] = (uint8_t)(status);
    p[4] = (uint8_t)(frame_count >> 24); p[5] = (uint8_t)(frame_count >> 16);
    p[6] = (uint8_t)(frame_count >> 8);  p[7] = (uint8_t)(frame_count);
    p[8] = (uint8_t)(crc_errs >> 24); p[9] = (uint8_t)(crc_errs >> 16);
    p[10]= (uint8_t)(crc_errs >> 8); p[11]= (uint8_t)(crc_errs);
    p[12]= mode; p[13]= armed;
    ble_tx_frame(CMD_GET_STATUS, p, 14);
}

/* ---- RX: byte-by-byte state machine fed from uart_getc ---- */
int ble_poll_cmd(uint8_t *op, uint8_t *payload, uint16_t *len) {
    char c;
    while (uart_getc(&c, 0) == 0) {
        uint8_t b = (uint8_t)c;
        switch (g_rx_state) {
        case 0:
            if (b == CMD_STX) { g_rx_state = 1; g_rx_len = 0; }
            break;
        case 1:
            g_rx_expect = (uint16_t)b << 8; g_rx_state = 2; break;
        case 2:
            g_rx_expect |= b; g_rx_state = 3; g_rx_op = 0; g_rx_crc = 0; break;
        case 3:
            g_rx_op = b; g_rx_state = 4; g_rx_len = 0; break;
        case 4:
            if (g_rx_len < g_rx_expect) {
                g_rx_buf[g_rx_len++] = b;
                if (g_rx_len == g_rx_expect) g_rx_state = 5;
            } else {
                g_rx_state = 5; g_rx_crc = b;
            }
            break;
        case 5:
            /* got CRC byte above; expect ETX */
            if (b == CMD_ETX) {
                *op = g_rx_op;
                *len = g_rx_len;
                if (g_rx_len) memcpy(payload, g_rx_buf, g_rx_len);
                g_rx_state = 0;
                return 1;
            }
            g_rx_state = 0;
            break;
        default:
            g_rx_state = 0;
            break;
        }
    }
    return 0;
}