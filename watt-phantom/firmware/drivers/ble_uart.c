/*
 * ble_uart.c — nRF52840 BLE UART bridge driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Communicates with the nRF52840 BLE module (NINA-B302) over USART2.
 * The BLE module runs a Nordic UART Service (NUS) transparent UART
 * bridge, allowing the MCU to send/receive data as if it were a
 * serial port. The BLE module handles the BLE 5.0 stack, connection
 * management, and encryption independently.
 *
 * The MCU sends commands and receives responses as ASCII text lines
 * terminated by \r\n. Binary data (e.g., covert channel data, power
 * profiles) is sent as hex-encoded strings.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* ---- USART2 register helpers ---- */
#define USART2_CR1   (*(volatile uint32_t *)(USART2_BASE + USART_CR1_OFFSET))
#define USART2_CR2   (*(volatile uint32_t *)(USART2_BASE + USART_CR2_OFFSET))
#define USART2_CR3   (*(volatile uint32_t *)(USART2_BASE + USART_CR3_OFFSET))
#define USART2_ISR   (*(volatile uint32_t *)(USART2_BASE + USART_ISR_OFFSET))
#define USART2_RDR   (*(volatile uint32_t *)(USART2_BASE + USART_RDR_OFFSET))
#define USART2_TDR   (*(volatile uint32_t *)(USART2_BASE + USART_TDR_OFFSET))

/* ---- RX ring buffer ---- */
#define BLE_RX_BUF_SIZE  512u
static uint8_t  s_rx_buf[BLE_RX_BUF_SIZE];
static uint16_t s_rx_head = 0;
static uint16_t s_rx_tail = 0;

/* ---- TX state ---- */
static uint8_t  s_tx_busy = 0;

/* ---- BLE connection state (updated by NINA-B302 status line) ---- */
static uint8_t  s_ble_connected = 0;

/* ---- Init BLE UART ---- */
int ble_uart_init(void) {
    /* USART2 is already initialized in board_init.c */
    /* Enable RXNE interrupt */
    USART2_CR1 |= USART_CR1_RXNEIE;

    /* Enable USART2 in NVIC (IRQ 38 on STM32G474) */
    NVIC_ISER0 = (1u << (38 - 0));

    s_rx_head = 0;
    s_rx_tail = 0;
    s_tx_busy = 0;
    s_ble_connected = 0;

    /* Send AT command to check BLE module presence */
    const char *at_cmd = "AT\r\n";
    ble_uart_send((const uint8_t *)at_cmd, 4);

    return 0;
}

/* ---- USART2 IRQ handler (RX) ---- */
void USART2_IRQHandler(void) {
    if (USART2_ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART2_RDR;
        uint16_t next = (s_rx_head + 1) % BLE_RX_BUF_SIZE;
        if (next != s_rx_tail) {
            s_rx_buf[s_rx_head] = byte;
            s_rx_head = next;
        }
    }
}

/* ---- Send data over BLE UART (blocking) ---- */
int ble_uart_send(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        /* Wait for TX empty */
        uint32_t timeout = millis() + 100;
        while (!(USART2_ISR & USART_ISR_TXE)) {
            if (millis() > timeout) return -1;
        }
        USART2_TDR = buf[i];
    }
    return 0;
}

/* ---- Receive data from BLE UART (non-blocking with timeout) ---- */
int ble_uart_recv(uint8_t *buf, uint32_t maxlen, uint32_t timeout_ms) {
    uint32_t count = 0;
    uint32_t deadline = millis() + timeout_ms;

    while (count < maxlen) {
        if (s_rx_tail != s_rx_head) {
            buf[count++] = s_rx_buf[s_rx_tail];
            s_rx_tail = (s_rx_tail + 1) % BLE_RX_BUF_SIZE;
        } else {
            if (timeout_ms > 0 && millis() > deadline) break;
            if (timeout_ms == 0) break; /* Non-blocking */
        }
    }
    return (int)count;
}

/* ---- BLE UART poll (called from main loop) ---- */
void ble_uart_poll(void) {
    /* Check for BLE module status messages (e.g., "CONNECTED", "DISCONNECTED") */
    static char line_buf[64];
    static uint8_t line_pos = 0;

    while (s_rx_tail != s_rx_head) {
        char c = (char)s_rx_buf[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) % BLE_RX_BUF_SIZE;

        if (c == '\n' || c == '\r') {
            if (line_pos > 0) {
                line_buf[line_pos] = '\0';
                /* Check for BLE status messages */
                if (strcmp(line_buf, "CONNECTED") == 0) {
                    s_ble_connected = 1;
                    extern device_state_t g_state;
                    g_state.ble_connected = 1;
                } else if (strcmp(line_buf, "DISCONNECTED") == 0) {
                    s_ble_connected = 0;
                    extern device_state_t g_state;
                    g_state.ble_connected = 0;
                }
                line_pos = 0;
            }
        } else {
            if (line_pos < sizeof(line_buf) - 1) {
                line_buf[line_pos++] = c;
            }
        }
    }
}