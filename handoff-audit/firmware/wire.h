/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef HA_WIRE_H
#define HA_WIRE_H
#include "handoff.h"
uint16_t ha_crc(const char *data, size_t size);
size_t ha_boot(char *out, size_t capacity, uint32_t flags);
size_t ha_policy(char *out, size_t capacity);
size_t ha_event_frame(char *out, size_t capacity, const struct ha_event *event);
size_t ha_status(char *out, size_t capacity, const struct handoff *h);
#endif
