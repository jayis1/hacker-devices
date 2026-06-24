/*
 * lora-phantom / drivers/ble_uart.c
 * UART protocol to nRF52840 BLE module (framed, AES-CTR encrypted).
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The nRF52840 runs a transparent BLE GATT bridge: data written to its UART
 * appears on the GATT characteristic 0xDEADF00D-... and vice versa. The link
 * is encrypted with AES-CTR using a shared 128-bit key provisioned at pairing
 * (the operator app and the device derive this key via ECDH during BLE
 * pairing; here we assume the key is set via CMD_SET_LINK_KEY).
 *
 * This driver handles the UART transport layer: framing, encryption/decryption,
 * and buffering. The actual BLE GATT service runs on the nRF52840 firmware
 * (separate build; not included here).
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* Forward declarations */
void aes128_ecb_pub(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
#define aes128_ecb aes128_ecb_pub

static uint8_t s_link_key[16] = {0};
static int s_key_set = 0;

/* ---- RX ring buffer ---- */
#define BLE_RX_BUF_SIZE 1024
static volatile uint8_t s_rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/* ---- USART3 ISR (receive) ---- */
/* On STM32H7, USART3 IRQ number = 82 (vector 82 + 16 = 98 in vector table). */
void USART3_IRQHandler(void) {
    if (USART3->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART3->RDR;
        uint16_t next = (s_rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next != s_rx_tail) {
            s_rx_buf[s_rx_head] = b;
            s_rx_head = next;
        }
    }
    /* Clear errors */
    if (USART3->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        USART3->ICR = 0xFF;
    }
}

void ble_uart_init(void) {
    /* UART is already configured in board_init.c (usart3_init).
     * Here we enable the RX interrupt. */
    USART3->CR1 |= USART_CR1_RXNEIE;

    /* Enable USART3 IRQ in NVIC (IRQ #82 on H7) */
    nvic_enable(82);
    nvic_set_prio(82, 0x80);

    /* Reset nRF52840 */
    gpio_write(BLE_RESET_PORT, BLE_RESET_PIN, 0);
    for (volatile int i = 0; i < 100000; i++) { }
    gpio_write(BLE_RESET_PORT, BLE_RESET_PIN, 1);
    for (volatile int i = 0; i < 1000000; i++) { }
}

void ble_set_link_key(const uint8_t key[16]) {
    memcpy(s_link_key, key, 16);
    s_key_set = 1;
}

/* ---- AES-CTR encrypt/decrypt a buffer (same operation both ways) ---- */
static void aes_ctr_xor(const uint8_t key[16], uint8_t nonce[16],
                        uint8_t *data, uint16_t len) {
    uint8_t keystream[16];
    uint8_t counter[16];
    memcpy(counter, nonce, 16);
    for (uint16_t i = 0; i < len; i += 16) {
        aes128_ecb(key, counter, keystream);
        uint16_t block = (len - i < 16) ? (len - i) : 16;
        for (uint16_t j = 0; j < block; j++) data[i + j] ^= keystream[j];
        /* Increment counter (big-endian, last 8 bytes) */
        for (int k = 15; k >= 8; k--) {
            if (++counter[k] != 0) break;
        }
    }
}

/* ---- Send raw bytes over UART (blocking) ---- */
static void uart_write(const uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        while (!(USART3->ISR & USART_ISR_TXE)) { }
        USART3->TDR = buf[i];
    }
}

/* ---- Send a protocol frame (unencrypted at this layer; encryption is at
 *      the protocol layer via proto_encode which adds CRC but not encryption.
 *      The BLE link layer encryption is applied here if a key is set.) ---- */
int ble_uart_send(const uint8_t *buf, uint16_t len) {
    if (s_key_set && len > 0) {
        /* Encrypt in-place in a local copy */
        static uint8_t enc[PROTO_MAX_PAYLOAD + 8];
        if (len > sizeof(enc)) len = sizeof(enc);
        memcpy(enc, buf, len);
        uint8_t nonce[16] = {0};
        /* Use a simple nonce: first 8 bytes = 0, last 8 = frame counter.
         * For a production system, use a proper non-repeating nonce. */
        static uint64_t ctr = 0;
        ctr++;
        for (int i = 0; i < 8; i++) nonce[15 - i] = (uint8_t)(ctr >> (i * 8));
        aes_ctr_xor(s_link_key, nonce, enc, len);
        /* Prepend nonce (last 8 bytes only, to save bandwidth) */
        uart_write(nonce + 8, 8);
        uart_write(enc, len);
    } else {
        uart_write(buf, len);
    }
    return (int)len;
}

/* ---- Receive with timeout ---- */
int ble_uart_recv(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms) {
    uint16_t got = 0;
    uint32_t deadline = timeout_ms * 48000;  /* approx cycles */
    while (got < maxlen && deadline--) {
        if (s_rx_tail != s_rx_head) {
            buf[got++] = s_rx_buf[s_rx_tail];
            s_rx_tail = (s_rx_tail + 1) % BLE_RX_BUF_SIZE;
        }
    }
    return (int)got;
}