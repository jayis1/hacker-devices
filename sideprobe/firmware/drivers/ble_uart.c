/*
 * drivers/ble_uart.c — nRF52840 BLE-UART bridge driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The nRF52840 module runs a transparent BLE-UART service (NUS-style). It
 * presents a UART to the STM32 (USART2 @ 115200, PA2/PA3, AF7). Data written to
 * the UART appears as BLE notifications on the connected phone; data from the
 * phone arrives on the UART RX. The nRF handles BLE encryption (LE Secure
 * Connections, 128-bit AES-CCM) transparently.
 *
 * This driver is a simple ring-buffered UART with interrupt-driven RX.
 */

#include <stdint.h>
#include "../board.h"
#include "../registers.h"

extern void sp_delay_us(volatile uint32_t us);

#define BLE_RX_BUF_SZ 512u
static volatile uint8_t  ble_rx_buf[BLE_RX_BUF_SZ];
static volatile uint32_t ble_rx_head = 0;
static volatile uint32_t ble_rx_tail = 0;

/* USART2 on PA2(TX)/PA3(RX), AF7 */
void ble_uart_init(void) {
    uint32_t moder = GPIO_MODER(GPIOA_BASE);
    moder &= ~((3u << 4) | (3u << 6));    /* PA2, PA3 */
    moder |= (2u << 4) | (2u << 6);
    GPIO_MODER(GPIOA_BASE) = moder;
    uint32_t afrl = GPIO_AFRL(GPIOA_BASE);
    afrl &= ~((0xFu << (2 * 4)) | (0xFu << (3 * 4)));
    afrl |= (7u << (2 * 4)) | (7u << (3 * 4));
    GPIO_AFRL(GPIOA_BASE) = afrl;

    USART_CR1(USART2_BASE) = 0;
    USART_BRR(USART2_BASE) = (APB1_HZ / 115200u);
    USART_CR3(USART2_BASE) = 0;
    USART_CR2(USART2_BASE) = 0;
    /* Enable RXNE interrupt */
    USART_CR1(USART2_BASE) = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | (1u << 5);
    /* Enable USART2 IRQ in NVIC */
    *(volatile uint32_t *)0xE000E100 |= (1u << 38); /* USART2 IRQn = 38 on H7 */
}

void USART2_Handler(void) __attribute__((interrupt("IRQ")));
void USART2_Handler(void) {
    if (USART_ISR(USART2_BASE) & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART_RDR(USART2_BASE);
        uint32_t next = (ble_rx_head + 1) % BLE_RX_BUF_SZ;
        if (next != ble_rx_tail) {
            ble_rx_buf[ble_rx_head] = b;
            ble_rx_head = next;
        }
    }
}

int ble_uart_recv(uint8_t *buf, uint32_t maxlen, uint32_t timeout_ms) {
    /* timeout_ms is a hint; with maxlen==0 we just check the buffer state */
    if (maxlen == 0) {
        return (ble_rx_head != ble_rx_tail) ? 1 : 0;
    }
    uint32_t deadline = timeout_ms * 1000u;
    while (ble_rx_head == ble_rx_tail) {
        if (deadline == 0) return 0; /* timeout, no data */
        sp_delay_us(100);
        deadline -= (deadline >= 100) ? 100 : deadline;
    }
    uint8_t b = ble_rx_buf[ble_rx_tail];
    ble_rx_tail = (ble_rx_tail + 1) % BLE_RX_BUF_SZ;
    *buf = b;
    return 1;
}

int ble_uart_send(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        while (!(USART_ISR(USART2_BASE) & USART_ISR_TXE)) { }
        USART_TDR(USART2_BASE) = buf[i];
    }
    return (int)len;
}