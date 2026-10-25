/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RL_WIRE_H
#define RL_WIRE_H
#include <stddef.h>
#include "budget.h"
uint16_t wire_crc(const char *data, size_t size);
size_t wire_status(char *out, size_t capacity, const Budget *b, uint32_t tx_fail);
size_t wire_event(char *out, size_t capacity, const Event *e);
size_t wire_policy(char *out, size_t capacity);
size_t wire_boot(char *out, size_t capacity, uint32_t flags);
#endif
