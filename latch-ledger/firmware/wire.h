/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LATCH_WIRE_H
#define LATCH_WIRE_H
#include "monitor.h"
#include <stddef.h>
#define WIRE_CAPACITY 256u
uint16_t wire_crc(const uint8_t *data, size_t length);
size_t wire_event(char *out, size_t capacity, const event_t *event);
size_t wire_status(char *out, size_t capacity, const monitor_t *m,
                   uint8_t channel, uint32_t now);
size_t wire_pair(char *out, size_t capacity, const monitor_t *m,
                 uint8_t pair, uint32_t now);
size_t wire_health(char *out, size_t capacity, const monitor_t *m,
                   uint32_t now);
size_t wire_boot(char *out, size_t capacity, uint32_t reset_flags);
#endif
