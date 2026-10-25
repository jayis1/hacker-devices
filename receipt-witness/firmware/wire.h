/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RW_WIRE_H
#define RW_WIRE_H
#include <stddef.h>
#include "receipt.h"
uint16_t wire_crc(const char *data, size_t size);
size_t wire_status(char *out, size_t capacity, const Receipt *r, uint32_t tx_fail);
size_t wire_policy(char *out, size_t capacity);
size_t wire_boot(char *out, size_t capacity, uint32_t flags);
#endif
