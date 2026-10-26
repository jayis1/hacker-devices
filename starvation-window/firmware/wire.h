/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef IW_WIRE_H
#define IW_WIRE_H
#include "isolation.h"
#include <stddef.h>
uint16_t wire_crc(const char *data, size_t length);
size_t wire_status(char *out, size_t size, const Isolation *s);
size_t wire_policy(char *out, size_t size);
size_t wire_boot(char *out, size_t size, uint32_t flags);
#endif
