/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef EW_WIRE_H
#define EW_WIRE_H
#include <stddef.h>
#include "erase.h"
uint16_t ew_crc(const char *data, size_t size);
size_t ew_boot(char *out, size_t capacity, uint32_t flags);
size_t ew_policy(char *out, size_t capacity);
size_t ew_status(char *out, size_t capacity, const struct erase *e);
size_t ew_event_frame(char *out, size_t capacity, const struct ew_event *event);
#endif
