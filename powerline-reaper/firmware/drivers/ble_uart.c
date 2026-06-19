/*
 * ble_uart.c — UART3 framing protocol to the nRF52840 BLE module
 *
 * Frame format (over UART3 @ 1 Mbps, HW flow control):
 *   [0xA5][LEN_LO][LEN_HI][TYPE][PAYLOAD...][CRC16_LO][CRC16_HI][0x5A]
 * The record layer above is ChaCha20-Poly1305 (see chacha20poly1305.c).
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "ble_uart.h"
#include "chacha20poly1305.h"
#include "../board.h"
#include "../registers.h"

#define BLE_UART_REG ((usart_t *)USART3_BASE)

static uint8_t rx_buf[256];
static volatile uint16_t rx_len = 0;
static volatile uint8_t  rx_state = 0; /* 0=idle 1=got_len_lo 2=got_len_hi 3=payload 4=crc 5=trailer */
static volatile uint16_t rx_expect = 0;
static volatile uint8_t  rx_type = 0;
static volatile uint16_t rx_crc = 0;
static volatile uint8_t  rx_idx = 0;
static uint8_t tx_buf[300];

static ble_frame_cb_t g_cb = NULL;
static uint8_t g_tunnel_key[32];
static uint8_t g_tunnel_nonce[12];
static uint32_t g_tunnel_counter = 0;
static int g_tunnel_up = 0;
static int g_paired = 0;

static uint16_t crc16(const uint8_t *d, uint32_t n) {
    uint16_t c = 0xFFFF;
    for (uint32_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int j = 0; j < 8; j++) {
            c = (c & 1) ? (c >> 1) ^ 0xA001 : (c >> 1);
        }
    }
    return c;
}

static void uart_write_byte(uint8_t b) {
    while (!(BLE_UART_REG->ISR & USART_ISR_TXE)) { }
    BLE_UART_REG->TDR = b;
}

void ble_uart_init(void) {
    /* USART3 on PC10(TX)/PC11(RX)/PD2(RTS)/PC12(CTS) AF7, HW flow control */
    /* Enable clocks already done in clock_init */
    GPIOC->MODER = (GPIOC->MODER & ~(3U<<(10*2) | 3U<<(11*2)))
                 | (2U<<(10*2)) | (2U<<(11*2));
    GPIOC->AFRH = (GPIOC->AFRH & ~(0xFFU << ((10-8)*4)))
                | (7U << ((10-8)*4)) | (7U << ((11-8)*4));
    /* RTS/CTS: PD2 alt 7, PC12 alt 7 */
    GPIOD->MODER = (GPIOD->MODER & ~(3U<<(2*2))) | (2U<<(2*2));
    GPIOD->AFRL  = (GPIOD->AFRL  & ~(0xFU << (2*4))) | (7U << (2*4));
    GPIOC->MODER = (GPIOC->MODER & ~(3U<<(12*2))) | (2U<<(12*2));
    GPIOC->AFRH  = (GPIOC->AFRH  & ~(0xFU << ((12-8)*4))) | (7U << ((12-8)*4));

    /* 1 Mbps: APB1=120 MHz / BRR. Use OVER8=0: BRR = 120e6 / 1e6 = 120 */
    BLE_UART_REG->BRR = 120;
    BLE_UART_REG->CR2 = 0;
    BLE_UART_REG->CR3 = USART_CR3_CTSE | USART_CR3_RTSE;
    BLE_UART_REG->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
    rx_state = 0;
}

void ble_uart_set_cb(ble_frame_cb_t cb) { g_cb = cb; }

void ble_uart_send(uint8_t type, const uint8_t *payload, uint16_t len) {
    if (len > 250) return;
    tx_buf[0] = 0xA5;
    tx_buf[1] = len & 0xFF;
    tx_buf[2] = (len >> 8) & 0xFF;
    tx_buf[3] = type;
    memcpy(&tx_buf[4], payload, len);
    uint16_t c = crc16(&tx_buf[3], len + 1);
    tx_buf[4 + len] = c & 0xFF;
    tx_buf[5 + len] = (c >> 8) & 0xFF;
    tx_buf[6 + len] = 0x5A;
    for (uint32_t i = 0; i < 7 + len; i++) uart_write_byte(tx_buf[i]);
}

/* RX state machine driven by USART3 IRQ */
void USART3_IRQHandler(void) {
    if (!(BLE_UART_REG->ISR & USART_ISR_RXNE)) return;
    uint8_t b = BLE_UART_REG->RDR;
    switch (rx_state) {
    case 0:
        if (b == 0xA5) { rx_state = 1; rx_idx = 0; }
        break;
    case 1:
        rx_expect = b; rx_state = 2; break;
    case 2:
        rx_expect |= (b << 8); rx_state = 3; break;
    case 3:
        if (rx_idx == 0) rx_type = b;
        else rx_buf[rx_idx - 1] = b;
        rx_idx++;
        if (rx_idx > rx_expect + 1) rx_state = 4;
        break;
    case 4:
        rx_crc = b; rx_state = 5; break;
    case 5:
        rx_crc |= (b << 8); rx_state = 6; break;
    case 6:
        if (b == 0x5A) {
            uint16_t c = crc16(&rx_buf[0], rx_expect + 1);
            if (c == rx_crc && g_cb) {
                g_cb(rx_type, rx_buf, rx_expect);
            }
        }
        rx_state = 0;
        break;
    }
}

void ble_uart_irq_handler(void) {
    /* Routed from EXTI15_10 — but actual UART IRQ is USART3_IRQHandler.
     * We use this for the BLE module's separate IRQ line (paired event),
     * handled at the application layer. Here we just wake the scheduler.
     */
    g_paired = 1;
}

void ble_uart_tick(void) {
    /* In a full impl: send periodic heartbeat, manage pairing state,
     * flush any pending tunnel frames. */
    if (g_tunnel_up && g_paired) {
        /* heartbeat */
    }
}

/* ---- Air-gap tunnel: encrypt + send PLC frame over BLE ---- */
void ble_uart_tunnel_tx(const qca7420_frame_t *f) {
    if (!g_tunnel_up) return;
    /* Encrypt frame data with ChaCha20-Poly1305, prepend counter */
    uint8_t buf[PLC_MAX_FRAME + 32];
    uint16_t len = f->len;
    if (len > PLC_MAX_FRAME) return;
    memcpy(buf, f->data, len);
    uint8_t tag[16];
    chacha20poly1305_encrypt(g_tunnel_key, g_tunnel_counter, g_tunnel_nonce,
                              NULL, 0, buf, len, tag);
    /* Send type=0x10 (tunnel frame) with counter+tag prefix */
    uint8_t record[32 + PLC_MAX_FRAME];
    record[0] = g_tunnel_counter & 0xFF;
    record[1] = (g_tunnel_counter >> 8) & 0xFF;
    record[2] = (g_tunnel_counter >> 16) & 0xFF;
    record[3] = (g_tunnel_counter >> 24) & 0xFF;
    memcpy(&record[4], tag, 16);
    memcpy(&record[20], buf, len);
    ble_uart_send(0x10, record, 20 + len);
    g_tunnel_counter++;
}

int ble_uart_tunnel_set_key(const uint8_t key[32], const uint8_t nonce[12]) {
    memcpy(g_tunnel_key, key, 32);
    memcpy(g_tunnel_nonce, nonce, 12);
    g_tunnel_counter = 0;
    g_tunnel_up = 1;
    return 0;
}

int ble_uart_tunnel_up(void) { return g_tunnel_up; }
int ble_uart_paired(void)   { return g_paired; }