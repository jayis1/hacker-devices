/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LATCH_BOARD_H
#define LATCH_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* NUCLEO-F401RE MB1136, reset clock HSI 16 MHz, no bootloader. */
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define CONTACT_A_MASK ((1u << 0) | (1u << 1) | (1u << 4))
#define CONTACT_B_MASK (1u << 0)
#define SERIAL_BUFFER 256u
void board_init(void);
uint8_t board_contacts(void);
uint32_t board_millis(void);
uint32_t board_reset_flags(void);
bool board_watchdog_start(void);
void board_watchdog_feed(void);
bool serial_submit(const char *data, size_t length);
void serial_poll(void);
bool serial_busy(void);
bool serial_take_timeout(void);
#endif
