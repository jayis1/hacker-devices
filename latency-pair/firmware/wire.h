/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LP_WIRE_H
#define LP_WIRE_H
#include <stddef.h>
#include "pair.h"
uint16_t wire_crc(const char *data, size_t length);
size_t wire_boot(char *out, size_t capacity, uint32_t reset);
size_t wire_policy(char *out, size_t capacity);
size_t wire_status(char *out, size_t capacity, const Pair *pair);
#endif
