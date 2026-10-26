/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef DW_BOARD_H
#define DW_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define CORE_HZ 16000000u
#define SERIAL_BAUD 115200u
#define SERIAL_BUFFER 384u
/* ALLOW PA0; DENY PA1; ENABLE PA4; FAULT PB0. */
void board_init(void);
uint8_t board_levels(void);
uint32_t board_millis(void);
uint32_t board_reset_flags(void);
bool board_watchdog_start(void);
void board_watchdog_feed(void);
bool serial_submit(const char *data, size_t size);
void serial_poll(void);
bool serial_busy(void);
bool serial_take_timeout(void);
#endif
