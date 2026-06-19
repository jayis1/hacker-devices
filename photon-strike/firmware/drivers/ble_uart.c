/**
 * drivers/ble_uart.c — framed binary protocol over UART3 to the ESP32-C3
 * Author: jayis1
 * License: GPL-2.0
 *
 * Frame format:  [0xA5][0x5A][op][len_hi][len_lo][payload...][crc8][0x0A]
 *
 * The ESP32-C3 bridges this UART to BLE GATT characteristics. The M7
 * uses a receive ring buffer filled by the UART RXNE interrupt, and a
 * blocking transmit path (frames are short and infrequent enough that
 * DMA is not warranted).
 */

#include "ble_uart.h"
#include "gpio.h"
#include "../registers.h"
#include <string.h>

#define UART3   ((volatile uint32_t *)USART3_BASE)
#define BLE_BAUD  2000000u

/* CRC8 (poly 0x07, init 0x00). */
static uint8_t crc8(const uint8_t *p, uint32_t n)
{
    uint8_t c = 0;
    for (uint32_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++)
            c = (c & 0x80u) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
    }
    return c;
}

/* ─── UART3 TX (blocking) ─────────────────────────────────────────────── */
static void uart3_putc(uint8_t b)
{
    while (!(UART3[USART_ISR / 4u] & USART_ISR_TXE)) ;
    *(volatile uint8_t *)&UART3[USART_TDR / 4u] = b;
}

static void uart3_send(const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) uart3_putc(p[i]);
}

/* ─── RX ring buffer (filled by IRQ) ──────────────────────────────────── */
#define RX_RING_SIZE  512u
static volatile uint8_t  rx_ring[RX_RING_SIZE];
static volatile uint16_t rx_head, rx_tail;

void USART3_IRQHandler(void)
{
    if (UART3[USART_ISR / 4u] & USART_ISR_RXNE) {
        uint8_t b = *(volatile uint8_t *)&UART3[USART_RDR / 4u];
        uint16_t next = (uint16_t)((rx_head + 1u) % RX_RING_SIZE);
        if (next != rx_tail) {
            rx_ring[rx_head] = b;
            rx_head = next;
        }
    }
}

static int rx_pop(void)
{
    if (rx_head == rx_tail) return -1;
    uint8_t b = rx_ring[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1u) % RX_RING_SIZE);
    return b;
}

/* ─── Output mailbox (operator-supplied target output) ──────────────────
 * The app sends target output blocks framed with op=BLE_MSG_LOG (reused
 * as "data" messages). We stash the latest one here for fire_one_shot()
 * to pick up. */
static uint8_t  s_out_buf[64];
static uint16_t s_out_len;
static volatile bool s_out_ready;

/* ─── Frame parser ────────────────────────────────────────────────────── */
/* We pull bytes from the ring until we either have a full frame or run
 * out of bytes. Returns the op and copies payload into buf; returns 0
 * length if no complete frame. */
static uint16_t parse_frame(uint8_t *op, uint8_t *buf, uint16_t maxlen)
{
    /* Scan for the sync pair 0xA5 0x5A. */
    int b;
    while ((b = rx_pop()) >= 0) {
        if (b != 0xA5) continue;
        b = rx_pop();
        if (b < 0)  return 0;
        if (b != 0x5A) continue;
        /* Sync found. Read op + len. */
        b = rx_pop(); if (b < 0) return 0; *op = (uint8_t)b;
        b = rx_pop(); if (b < 0) return 0; uint8_t len_hi = (uint8_t)b;
        b = rx_pop(); if (b < 0) return 0; uint8_t len_lo = (uint8_t)b;
        uint16_t len = (uint16_t)((len_hi << 8) | len_lo);
        if (len > maxlen) {
            /* drain the payload + crc + trailer to resync */
            for (uint16_t i = 0; i < len + 2u; i++) { if (rx_pop() < 0) return 0; }
            return 0;
        }
        for (uint16_t i = 0; i < len; i++) {
            int x = rx_pop();
            if (x < 0) return 0;
            buf[i] = (uint8_t)x;
        }
        int c = rx_pop(); if (c < 0) return 0;
        /* verify crc over op+len+payload */
        uint8_t hdr[3] = { *op, len_hi, len_lo };
        uint8_t crc = crc8(hdr, 3) ^ crc8(buf, len);  /* chained */
        if ((uint8_t)c != crc) return 0;
        rx_pop();   /* trailer 0x0A */
        return len;
    }
    return 0;
}

/* ─── Public API ──────────────────────────────────────────────────────── */
void ble_init(void)
{
    rx_head = rx_tail = 0;
    s_out_ready = false;
    s_out_len = 0;

    /* UART3: 2 Mbaud, 8N1, enable RX interrupt. */
    UART3[USART_CR1 / 4u] = 0;
    UART3[USART_BRR / 4u] = 480000000u / BLE_BAUD;
    UART3[USART_CR1 / 4u] = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE |
                            USART_CR1_RXNEIE;

    /* Enable the USART3 IRQ in the NVIC. */
    REG32(NVIC_ISER0) = (1u << 39u);  /* USART3 IRQ #39 on STM32H7 */
}

static void send_frame(uint8_t op, const uint8_t *payload, uint16_t len)
{
    uint8_t hdr[5] = { 0xA5, 0x5A, op, (uint8_t)(len >> 8), (uint8_t)(len & 0xFF) };
    uart3_send(hdr, 5);
    if (len) uart3_send(payload, len);
    uint8_t crc_buf[3] = { op, (uint8_t)(len >> 8), (uint8_t)(len & 0xFF) };
    uint8_t crc = crc8(crc_buf, 3);
    crc = (uint8_t)(crc ^ (len ? crc8(payload, len) : 0));
    uart3_putc(crc);
    uart3_putc(0x0A);
}

void ble_send_hello(uint32_t version)
{
    uint8_t p[4] = { (uint8_t)(version), (uint8_t)(version >> 8),
                    (uint8_t)(version >> 16), (uint8_t)(version >> 24) };
    send_frame(BLE_MSG_HELLO, p, 4);
}

void ble_send_ack(uint8_t op, uint32_t arg)
{
    uint8_t p[5] = { op, (uint8_t)arg, (uint8_t)(arg >> 8),
                    (uint8_t)(arg >> 16), (uint8_t)(arg >> 24) };
    send_frame(BLE_MSG_ACK, p, 5);
}

void ble_send_nack(uint8_t op, uint32_t arg)
{
    uint8_t p[5] = { op, (uint8_t)arg, (uint8_t)(arg >> 8),
                    (uint8_t)(arg >> 16), (uint8_t)(arg >> 24) };
    send_frame(BLE_MSG_NACK, p, 5);
}

void ble_send_status(uint8_t op, uint32_t a, uint32_t b, uint32_t c)
{
    uint8_t p[12];
    for (int i = 0; i < 4; i++) p[i]      = (uint8_t)(a >> (i * 8));
    for (int i = 0; i < 4; i++) p[4 + i]  = (uint8_t)(b >> (i * 8));
    for (int i = 0; i < 4; i++) p[8 + i]  = (uint8_t)(c >> (i * 8));
    send_frame(op, p, 12);
}

void ble_send_shot(const ps_shot_t *shot)
{
    send_frame(BLE_MSG_STATUS, (const uint8_t *)shot, sizeof(*shot));
}

void ble_send_dfa(uint8_t faults, uint8_t unique, const uint8_t key[16])
{
    uint8_t p[18];
    p[0] = faults;
    p[1] = unique;
    memcpy(&p[2], key, 16);
    send_frame(BLE_MSG_DFA, p, 18);
}

uint16_t ble_poll(uint8_t *op, uint8_t *buf, uint16_t maxlen)
{
    return parse_frame(op, buf, maxlen);
}

bool ble_take_output(uint8_t *buf, uint16_t *len, uint16_t maxlen, uint32_t timeout_ms)
{
    /* Drain the ring for a short timeout looking for a data frame. */
    for (uint32_t i = 0; i < timeout_ms * 2u; i++) {
        uint8_t op;
        uint16_t n = parse_frame(&op, buf, maxlen);
        if (n > 0) {
            if (op == BLE_MSG_LOG) {
                *len = n;
                return true;
            } else {
                /* Not an output frame; put it back is not possible with a
                 * ring, so we just drop it (the main loop will re-fetch
                 * control messages separately). In a production design
                 * we'd queue these; here we accept the small loss. */
                continue;
            }
        }
        ps_delay_us(500);
    }
    return false;
}