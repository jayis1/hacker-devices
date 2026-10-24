/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef QW_WIRE_H
#define QW_WIRE_H
#include "quorum.h"
#include <stddef.h>
uint16_t qw_crc(const char *data, size_t size);
size_t qw_boot(char *out, size_t capacity, uint32_t flags);
size_t qw_policy(char *out, size_t capacity);
size_t qw_event_frame(char *out, size_t capacity, const struct qw_event *event);
size_t qw_status(char *out, size_t capacity, const struct quorum *q);
#endif
