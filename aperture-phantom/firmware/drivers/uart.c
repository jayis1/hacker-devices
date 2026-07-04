/*
 * aperture-phantom / firmware / drivers / uart.c
 *
 * USART3 driver for the BLE module console and debug log. Polled TX,
 * timeout-bounded RX. Used by ble_uart.c for the framed protocol and by
 * logf() for status output.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "board.h"

extern volatile uint32_t g_ticks_ms;

void uart_init(uint32_t baud) {
    USART3_CR1 = 0;
    USART3_CR2 = 0;
    USART3_CR3 = 0;
    /* APB1 = 120 MHz. OVER8=1, BRR = APB1/baud. */
    USART3_BRR = (120000000u + baud / 2) / baud;
    USART3_CR1 = (1u << 3)     /* TE */
               | (1u << 2)     /* RE */
               | (1u << 0);    /* UE */
}

void uart_putc(char c) {
    if (c == '\n') uart_putc('\r');
    while ((USART3_ISR & USART3_ISR_TXE) == 0) { }
    USART3_TDR = (uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

int uart_getc(char *c, uint32_t timeout_ms) {
    uint32_t t0 = g_ticks_ms;
    while ((USART3_ISR & USART3_ISR_RXNE) == 0) {
        if ((g_ticks_ms - t0) > timeout_ms) return -1;
    }
    *c = (char)USART3_RDR;
    return 0;
}

void uart_write(const uint8_t *buf, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) {
        while ((USART3_ISR & USART3_ISR_TXE) == 0) { }
        USART3_TDR = buf[i];
    }
}

int uart_read(uint8_t *buf, uint16_t n, uint32_t timeout_ms) {
    uint32_t t0 = g_ticks_ms;
    for (uint16_t i = 0; i < n; i++) {
        while ((USART3_ISR & USART3_ISR_RXNE) == 0) {
            if ((g_ticks_ms - t0) > timeout_ms) return -1;
        }
        buf[i] = (uint8_t)USART3_RDR;
    }
    return 0;
}