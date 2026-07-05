/*
 * ble_uart.c — UART bridge to the nRF52840 BLE module
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The nRF52840 runs a transparent UART-BLE bridge firmware (pre-flashed).
 * The MCU talks to it over USART1 at 115200 8N1. This driver provides
 * blocking read/write against a small ring buffer fed by the USART RX ISR.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

#define BLE_RX_BUF 256
static volatile uint8_t ble_rx[BLE_RX_BUF];
static volatile uint16_t ble_rx_head = 0, ble_rx_tail = 0;

int ble_uart_init(void) {
    /* USART1 already enabled in board_init; enable RXNE interrupt */
    USART1_CR1 |= (1u<<5); /* RXNEIE */
    NVIC_ISER0 |= (1u<<(27)); /* USART1 IRQn on G4 */
    return 0;
}

int ble_uart_write(const void *buf, uint16_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    for (uint16_t i = 0; i < len; i++) {
        while (!(USART1_ISR & USART_ISR_TXE)) {}
        USART1_TDR = p[i];
    }
    return len;
}

int ble_uart_read(void *buf, uint16_t maxlen) {
    uint8_t *p = (uint8_t *)buf;
    uint16_t n = 0;
    while (n < maxlen && ble_rx_head != ble_rx_tail) {
        p[n++] = ble_rx[ble_rx_tail];
        ble_rx_tail = (ble_rx_tail + 1) % BLE_RX_BUF;
    }
    return (int)n;
}

void USART1_IRQHandler(void) {
    if (USART1_ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART1_RDR;
        uint16_t next = (ble_rx_head + 1) % BLE_RX_BUF;
        if (next != ble_rx_tail) {
            ble_rx[ble_rx_head] = b;
            ble_rx_head = next;
        }
        /* else: overflow, drop */
    }
}