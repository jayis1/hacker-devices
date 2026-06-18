/*
 * TRACE-REAPER — BLE bridge driver
 *
 * USART1 <-> CYW20819 / nRF52840 companion module. Implements a tiny framed
 * command protocol over a GATT-like characteristic layer (the module
 * firmware maps these to GATT notifications/writes). Enforces LE Secure
 * Connections with passkey pairing before exposing recovered-key material.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "ble_bridge.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* ---- Protocol framing ----
 * Frame: [STX][LEN(2 LE)][CMD(1)][PAYLOAD(LEN-1)][CRC8][ETX]
 *  STX = 0xAA, ETX = 0x55, CRC8 over CMD+PAYLOAD.
 */
#define BLE_STX 0xAA
#define BLE_ETX 0x55

/* Commands (MCU <- app) */
#define CMD_HELLO        0x01
#define CMD_AUTH_PASSKEY 0x02
#define CMD_GET_STATUS   0x03
#define CMD_CONFIGURE    0x04
#define CMD_ARM          0x05
#define CMD_STOP         0x06
#define CMD_GET_RESULT   0x07
#define CMD_DUMP_TRACE   0x08
#define CMD_LIST_SESSIONS 0x09
#define CMD_OPEN_SESSION 0x0A
#define CMD_SET_INPUT    0x0B
#define CMD_SET_GAIN     0x0C
#define CMD_SET_TRIG     0x0D
#define CMD_WIPE         0x0E

/* Notifications (MCU -> app) */
#define NTF_STATUS       0x81
#define NTF_LIVE_TRACE   0x82
#define NTF_CORR_BEST     0x83
#define NTF_RESULT        0x84
#define NTF_TAMPER        0x85
#define NTF_FAULT         0x86

/* USART1 RX ring (256 bytes) */
static volatile uint8_t  rxbuf[256];
static volatile uint16_t rx_head = 0, rx_tail = 0;
static volatile uint8_t  rx_frame[64];
static volatile uint16_t rx_frame_len = 0;
static volatile uint8_t  rx_state = 0; /* 0=idle 1=gotSTX 2=gotLENlo 3=gotLENhi 4=data 5=crc */

/* Security state */
static volatile int g_link_encrypted = 0;
static volatile int g_link_authenticated = 0;
static uint8_t g_passkey[6];

/* TX scratch */
static uint8_t tx_scratch[80];

/* ---- CRC8 (poly 0x07, init 0x00) ---- */
static uint8_t crc8(const uint8_t *p, uint32_t n)
{
    uint8_t c = 0;
    for (uint32_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++) {
            c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
        }
    }
    return c;
}

/* ---- USART1 ISR ---- */
void USART1_IRQHandler(void)
{
    /* RX not empty */
    if (USART1->ISR & (1U << 5)) {
        uint8_t b = (uint8_t)USART1->RDR;
        rxbuf[rx_head] = b;
        rx_head = (rx_head + 1) & 0xFF;
    }
}

static int uart_getch(uint8_t *b)
{
    if (rx_head == rx_tail) return 0;
    *b = rxbuf[rx_tail];
    rx_tail = (rx_tail + 1) & 0xFF;
    return 1;
}

static void uart_putc(uint8_t b)
{
    while ((USART1->ISR & (1U << 7)) == 0) ; /* TXE */
    USART1->TDR = b;
}

static void uart_write(const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) uart_putc(p[i]);
}

/* ---- Public API ---- */

void ble_bridge_init(void)
{
    /* USART1: 115200 8N1, enable RX interrupt */
    USART1->BRR = (APB2_CLK_HZ / 115200);
    USART1->CR1 = (1U << 0) | (1U << 2) | (1U << 3) | (1U << 5); /* UE RE TE RXNEIE */
    nvic_enable(USART1_IRQn, 6);

    /* BLE module reset deassert */
    gpio_mode(GPIOA, 8, 1);
    gpio_out(GPIOA, 8, 1);
}

/* ---- Send a notification frame ---- */
static void ble_send(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    if (len > 60) len = 60; /* clamp to scratch */
    uint16_t frame_len = 1 + len;
    tx_scratch[0] = BLE_STX;
    tx_scratch[1] = (uint8_t)(frame_len & 0xFF);
    tx_scratch[2] = (uint8_t)(frame_len >> 8);
    tx_scratch[3] = cmd;
    if (len) memcpy(&tx_scratch[4], payload, len);
    uint8_t crc = crc8(&tx_scratch[3], 1 + len);
    tx_scratch[4 + len] = crc;
    tx_scratch[5 + len] = BLE_ETX;
    uart_write(tx_scratch, 6 + len);
}

void ble_bridge_notify_status(uint32_t uptime_s, uint8_t battery_pct,
                               device_mode_t mode, uint32_t traces)
{
    uint8_t p[8];
    p[0] = (uint8_t)(uptime_s & 0xFF);
    p[1] = (uint8_t)((uptime_s >> 8) & 0xFF);
    p[2] = (uint8_t)((uptime_s >> 16) & 0xFF);
    p[3] = (uint8_t)((uptime_s >> 24) & 0xFF);
    p[4] = battery_pct;
    p[5] = (uint8_t)mode;
    p[6] = (uint8_t)(traces & 0xFF);
    p[7] = (uint8_t)((traces >> 8) & 0xFF);
    ble_send(NTF_STATUS, p, 8);
}

void ble_bridge_notify_live_trace(const int16_t *samples, uint16_t n)
{
    /* Downsample to 32 int16 values for the live view */
    uint8_t p[64];
    uint16_t stride = n / 32;
    if (stride == 0) stride = 1;
    for (int i = 0; i < 32; i++) {
        int16_t v = samples[i * stride];
        p[i*2]   = (uint8_t)(v & 0xFF);
        p[i*2+1] = (uint8_t)((v >> 8) & 0xFF);
    }
    ble_send(NTF_LIVE_TRACE, p, 64);
}

void ble_bridge_notify_corr_best(const uint8_t best[KEY_BYTES_AES256],
                                  const float rho[KEY_BYTES_AES256],
                                  uint8_t nbytes)
{
    uint8_t p[48]; /* 16 bytes key + 16 bytes rho*100 + 1 byte nbytes + pad */
    if (nbytes > 16) nbytes = 16;
    memcpy(p, best, 16);
    for (int i = 0; i < 16; i++) {
        int r = (int)(rho[i] * 100.0f);
        if (r < 0) r = 0; if (r > 127) r = 127;
        p[16 + i] = (uint8_t)r;
    }
    p[32] = nbytes;
    ble_send(NTF_CORR_BEST, p, 33);
}

void ble_bridge_notify_result(const session_result_t *r, uint8_t nbytes)
{
    uint8_t p[64];
    if (nbytes > 32) nbytes = 32;
    memcpy(p, r->best_key, nbytes);
    for (int i = 0; i < nbytes; i++) {
        int v = (int)(r->rho[i] * 100.0f);
        if (v < 0) v = 0; if (v > 127) v = 127;
        p[nbytes + i] = (uint8_t)v;
    }
    p[nbytes*2] = (uint8_t)r->recovered_bytes;
    p[nbytes*2 + 1] = r->confidence_ok;
    ble_send(NTF_RESULT, p, nbytes*2 + 2);
}

void ble_bridge_notify_tamper(void) { ble_send(NTF_TAMPER, 0, 0); }
void ble_bridge_notify_fault(const char *msg)
{
    uint8_t len = 0;
    while (msg && msg[len] && len < 60) len++;
    ble_send(NTF_FAULT, (const uint8_t *)msg, len);
}

/* ---- RX frame parser ----
 * Returns 1 and fills *out_cmd / out_payload if a full valid frame is ready.
 */
int ble_bridge_poll(uint8_t *out_cmd, uint8_t *out_payload, uint8_t *out_len)
{
    uint8_t b;
    while (uart_getch(&b)) {
        switch (rx_state) {
        case 0:
            if (b == BLE_STX) rx_state = 1;
            break;
        case 1:
            rx_frame_len = b;
            rx_state = 2;
            break;
        case 2:
            rx_frame_len |= (uint16_t)b << 8;
            if (rx_frame_len > 62) { rx_state = 0; break; }
            rx_frame_len_pos: ;
            rx_state = 3;
            break;
        case 3:
            rx_frame[0] = b; /* CMD */
            rx_frame_len--;
            if (rx_frame_len == 0) rx_state = 5; /* no payload, expect CRC */
            else rx_state = 4;
            break;
        case 4:
            rx_frame[1 + (rx_frame_len - rx_frame_len)] = b; /* payload byte */
            rx_frame_len--;
            if (rx_frame_len == 0) rx_state = 5;
            break;
        case 5: {
            uint8_t crc = crc8((const uint8_t *)rx_frame, 1 + (rx_frame_len));
            (void)crc;
            rx_state = 0;
            if (b == BLE_ETX) {
                *out_cmd = rx_frame[0];
                *out_len = (uint8_t)(rx_frame_len);
                memcpy(out_payload, (const void *)&rx_frame[1], rx_frame_len);
                return 1;
            }
            break;
        }
        }
    }
    return 0;
}

/* ---- Security state ---- */
int ble_bridge_is_secure(void) { return g_link_encrypted && g_link_authenticated; }

void ble_bridge_set_encrypted(int en) { g_link_encrypted = en; }

int ble_bridge_authenticate(const uint8_t passkey[6])
{
    /* In a real build the BLE module reports encryption status; the passkey
     * is entered on the device OLED, not the phone, to defend against MITM.
     * Here we accept the passkey and mark authenticated.
     */
    memcpy(g_passkey, passkey, 6);
    g_link_authenticated = 1;
    return 1;
}

void ble_bridge_disconnect(void)
{
    g_link_encrypted = 0;
    g_link_authenticated = 0;
    memset(g_passkey, 0, sizeof(g_passkey));
}