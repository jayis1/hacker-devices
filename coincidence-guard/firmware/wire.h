/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef CG_WIRE_H
#define CG_WIRE_H
#include <stddef.h>
#include "consent.h"
uint16_t cg_crc(const char *data, size_t size);
size_t cg_boot(char *out, size_t capacity, uint32_t flags);
size_t cg_policy(char *out, size_t capacity);
size_t cg_event_frame(char *out, size_t capacity, const struct cg_event *event);
size_t cg_status(char *out, size_t capacity, const struct consent *c);
#endif
