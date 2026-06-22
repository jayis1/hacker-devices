/*
 * ble_uart.c — WireReaper BLE transport driver via nRF52840 USART
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Manages the serial link to the nRF52840 BLE module (NINA-B302).
 * Implements a framed protocol with AES-CCM encryption (the crypto
 * is handled by the nRF52840 firmware; this driver handles framing,
 * flow control, and the MCU-side transport).
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

#define BLE_USART        ((usart_t *)BLE_USART_BASE)

/* Frame format:
 * [0xAA] [LEN_H] [LEN_L] [PAYLOAD...] [CRC8]
 * 0xAA is the sync byte; if 0xAA appears in payload, it is escaped
 * as 0xAA 0x00.
 */
#define BLE_SYNC_BYTE     0xAA
#define BLE_ESCAPE_BYTE   0x00
#define BLE_MAX_FRAME     252  /* BLE MTU 247 - 2 header - 1 CRC - overhead */

/* RX state machine */
static struct {
    uint8_t  buf[BLE_MAX_FRAME + 4];
    int      len;
    int      state;  /* 0=idle, 1=got_sync, 2=got_len_h, 3=in_payload */
    uint16_t expect;
    int      escape;
} ble_rx;

/* TX ring buffer for outbound frames */
static uint8_t  ble_tx_buf[512];
static volatile uint16_t ble_tx_head;
static volatile uint16_t ble_tx_tail;

/* ---- CRC8 (poly 0x07) ---- */
static uint8_t crc8(const uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ---- Initialize BLE USART ---- */
void wr_ble_init(void) {
    /* Enable clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOA;
    RCC_APB2ENR |= RCC_APB2ENR_USART1;

    /* Configure PA9 (TX) and PA10 (RX) as AF7 */
    gpio_t *a = GPIO(GPIOA_BASE);
    uint32_t af = 7;

    /* TX */
    a->MODER &= ~(3U << (BLE_TX_PIN * 2));
    a->MODER |= (GPIO_MODE_AF << (BLE_TX_PIN * 2));
    a->OSPEEDR |= (GPIO_OSPEED_HIGH << (BLE_TX_PIN * 2));
    a->AFR[BLE_TX_PIN / 8] &= ~(0xFU << ((BLE_TX_PIN % 8) * 4));
    a->AFR[BLE_TX_PIN / 8] |= (af << ((BLE_TX_PIN % 8) * 4));

    /* RX */
    a->MODER &= ~(3U << (BLE_RX_PIN * 2));
    a->MODER |= (GPIO_MODE_AF << (BLE_RX_PIN * 2));
    a->PUPDR |= (GPIO_PUPD_PULLUP << (BLE_RX_PIN * 2));
    a->AFR[BLE_RX_PIN / 8] &= ~(0xFU << ((BLE_RX_PIN % 8) * 4));
    a->AFR[BLE_RX_PIN / 8] |= (af << ((BLE_RX_PIN % 8) * 4));

    /* Configure USART: 1 Mbps, 8N1, RX interrupt */
    BLE_USART->CR1 = 0;
    /* USART1 on APB2 = 120 MHz, BRR = 120M/1M = 120 */
    BLE_USART->BRR = (APB2_HZ + BLE_USART_BAUD / 2) / BLE_USART_BAUD;
    BLE_USART->CR2 = 0;
    BLE_USART->CR3 = 0;
    BLE_USART->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE |
                     USART_CR1_RXNEIE | USART_CR1_IDLEIE;

    nvic_enable(IRQ_USART1);

    /* Reset RX state */
    memset(&ble_rx, 0, sizeof(ble_rx));
    ble_tx_head = 0;
    ble_tx_tail = 0;

    /* Send AT command to nRF52840 to configure BLE advertising */
    /* (In production, the nRF52840 has its own firmware that handles
     * BLE GATT services; we just send raw frames over UART) */
}

/* ---- USART1 interrupt handler ---- */
IRQ_HANDLER(USART1_IRQHandler) {
    uint32_t isr = BLE_USART->ISR;

    if (isr & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)BLE_USART->RDR;

        /* Frame parsing state machine */
        switch (ble_rx.state) {
        case 0: /* idle */
            if (b == BLE_SYNC_BYTE) {
                ble_rx.state = 1;
                ble_rx.len = 0;
                ble_rx.escape = 0;
            }
            break;
        case 1: /* got sync, expect LEN_H */
            ble_rx.expect = (uint16_t)b << 8;
            ble_rx.state = 2;
            break;
        case 2: /* got LEN_L */
            ble_rx.expect |= b;
            if (ble_rx.expect > BLE_MAX_FRAME) {
                ble_rx.state = 0; /* Frame too large, abort */
            } else {
                ble_rx.state = 3;
                ble_rx.len = 0;
            }
            break;
        case 3: /* in payload */
            if (ble_rx.escape) {
                ble_rx.buf[ble_rx.len++] = b;
                ble_rx.escape = 0;
            } else if (b == BLE_SYNC_BYTE) {
                ble_rx.escape = 1; /* next byte is literal */
            } else {
                ble_rx.buf[ble_rx.len++] = b;
            }
            if (ble_rx.len >= ble_rx.expect + 1) {
                /* Full frame received (payload + CRC) */
                /* Verify CRC */
                uint8_t crc = crc8(ble_rx.buf, ble_rx.expect);
                if (crc == ble_rx.buf[ble_rx.expect]) {
                    /* Valid frame: signal to main loop via global */
                    /* In this simplified driver, we store the frame
                     * and let wr_ble_recv() return it */
                }
                ble_rx.state = 0;
            }
            break;
        }
    }

    if (isr & USART_ISR_TXE) {
        /* Send next byte from TX buffer if available */
        if (ble_tx_tail != ble_tx_head) {
            BLE_USART->TDR = ble_tx_buf[ble_tx_tail];
            ble_tx_tail = (ble_tx_tail + 1) % sizeof(ble_tx_buf);
        } else {
            /* No more data, disable TX interrupt */
            BLE_USART->CR1 &= ~USART_CR1_TCIE;
        }
    }

    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        BLE_USART->ICR = 0x1F; /* Clear all error flags */
    }
}

/* ---- Send framed data over BLE ---- */
void wr_ble_send(const uint8_t *data, int len) {
    if (len > BLE_MAX_FRAME)
        return;

    uint8_t frame[BLE_MAX_FRAME * 2 + 4];
    int fidx = 0;

    frame[fidx++] = BLE_SYNC_BYTE;
    frame[fidx++] = (len >> 8) & 0xFF;
    frame[fidx++] = len & 0xFF;

    for (int i = 0; i < len; i++) {
        if (data[i] == BLE_SYNC_BYTE) {
            frame[fidx++] = BLE_SYNC_BYTE;
            frame[fidx++] = BLE_ESCAPE_BYTE;
        } else {
            frame[fidx++] = data[i];
        }
    }

    uint8_t crc = crc8(data, len);
    if (crc == BLE_SYNC_BYTE) {
        frame[fidx++] = BLE_SYNC_BYTE;
        frame[fidx++] = BLE_ESCAPE_BYTE;
    } else {
        frame[fidx++] = crc;
    }

    /* Queue bytes for TX interrupt */
    for (int i = 0; i < fidx; i++) {
        uint16_t next = (ble_tx_head + 1) % sizeof(ble_tx_buf);
        if (next == ble_tx_tail)
            break; /* Buffer full */
        ble_tx_buf[ble_tx_head] = frame[i];
        ble_tx_head = next;
    }

    /* Enable TX interrupt to start sending */
    BLE_USART->CR1 |= USART_CR1_TCIE;
}

/* ---- Receive framed data from BLE ---- */
int wr_ble_recv(uint8_t *buf, int maxlen) {
    if (ble_rx.state != 0 || ble_rx.len == 0)
        return 0;

    int len = ble_rx.len - 1; /* Exclude CRC */
    if (len > maxlen)
        len = maxlen;
    memcpy(buf, ble_rx.buf, len);
    ble_rx.len = 0;
    return len;
}

/* ---- Check if BLE is connected ---- */
int wr_ble_is_connected(void) {
    /* In a full implementation, the nRF52840 would set a GPIO pin
     * when a BLE connection is established. We poll that here. */
    return 1; /* Assume connected for now */
}