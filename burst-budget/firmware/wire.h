/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef BB_WIRE_H
#define BB_WIRE_H
#include "budget.h"
#include <stddef.h>
size_t wire_status(char *out, size_t cap, const Budget *s);
size_t wire_boot(char *out, size_t cap, uint32_t flags);
size_t wire_policy(char *out, size_t cap);
uint16_t wire_crc(const char *data, size_t n);
#endif
