/*
 * usb_cdc.c — USB CDC virtual serial port for debug/log
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal USB CDC implementation over USART1 (used as a debug port).
 * In a full implementation, this would use the USB peripheral directly
 * with proper device descriptors. For the WattPhantom, USART1 at 115200
 * baud serves as the debug interface, and can be connected to a USB-UART
 * bridge or used directly from a host with a serial terminal.
 *
 * The USB data path (D+/D-) passes through the FSUSB42 switch and is
 * primarily for the target device. This debug port is separate.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

#define USART1_CR1   (*(volatile uint32_t *)(USART1_BASE + USART_CR1_OFFSET))
#define USART1_ISR   (*(volatile uint32_t *)(USART1_BASE + USART_ISR_OFFSET))
#define USART1_RDR   (*(volatile uint32_t *)(USART1_BASE + USART_RDR_OFFSET))
#define USART1_TDR   (*(volatile uint32_t *)(USART1_BASE + USART_TDR_OFFSET))

#define USB_RX_BUF_SIZE  256u
static uint8_t  s_rx_buf[USB_RX_BUF_SIZE];
static uint16_t s_rx_head = 0;
static uint16_t s_rx_tail = 0;

/* ---- Init USB CDC (debug UART) ---- */
int usb_cdc_init(void) {
    /* USART1 already initialized in board_init.c */
    /* Enable RXNE interrupt */
    USART1_CR1 |= USART_CR1_RXNEIE;

    /* Enable USART1 in NVIC (IRQ 37 on STM32G474) */
    NVIC_ISER0 = (1u << (37 - 0));

    s_rx_head = 0;
    s_rx_tail = 0;

    const char *banner = "\r\nWattPhantom v1.0 debug port\r\nAuthor: jayis1\r\n";
    usb_cdc_send((const uint8_t *)banner, strlen(banner));

    return 0;
}

/* ---- USART1 IRQ handler ---- */
void USART1_IRQHandler(void) {
    if (USART1_ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART1_RDR;
        uint16_t next = (s_rx_head + 1) % USB_RX_BUF_SIZE;
        if (next != s_rx_tail) {
            s_rx_buf[s_rx_head] = byte;
            s_rx_head = next;
        }
    }
}

/* ---- Send data (blocking) ---- */
int usb_cdc_send(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uint32_t timeout = millis() + 100;
        while (!(USART1_ISR & USART_ISR_TXE)) {
            if (millis() > timeout) return -1;
        }
        USART1_TDR = buf[i];
    }
    return 0;
}

/* ---- Receive data (non-blocking with timeout) ---- */
int usb_cdc_recv(uint8_t *buf, uint32_t maxlen, uint32_t timeout_ms) {
    uint32_t count = 0;
    uint32_t deadline = millis() + timeout_ms;

    while (count < maxlen) {
        if (s_rx_tail != s_rx_head) {
            buf[count++] = s_rx_buf[s_rx_tail];
            s_rx_tail = (s_rx_tail + 1) % USB_RX_BUF_SIZE;
        } else {
            if (timeout_ms > 0 && millis() > deadline) break;
            if (timeout_ms == 0) break;
        }
    }
    return (int)count;
}