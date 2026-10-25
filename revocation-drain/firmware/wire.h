/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RD_WIRE_H
#define RD_WIRE_H
#include <stddef.h>
#include <stdint.h>
#include "drain.h"
uint16_t wire_crc(const char *s, size_t n);
size_t wire_boot(char *out, size_t cap, uint32_t reset);
size_t wire_policy(char *out, size_t cap);
size_t wire_status(char *out, size_t cap, const Drain *d);
#endif
