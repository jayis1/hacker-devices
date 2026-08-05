/*
 * uart_monitor.h — Target UART monitor driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef UART_MONITOR_H
#define UART_MONITOR_H

#include <stdint.h>
#include "../board.h"

void uart_monitor_init(uint32_t baudrate);
void uart_monitor_reset(void);
void uart_monitor_get_response(char *buf, uint32_t buf_len);
void uart_monitor_isr(void);
void uart_monitor_dma_isr(void);

/* Check if trigger word was seen (for UART trigger source) */
bool uart_monitor_trigger_word_seen(void);

#endif /* UART_MONITOR_H */