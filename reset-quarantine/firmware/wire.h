/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RQ_WIRE_H
#define RQ_WIRE_H
#include <stddef.h>
#include "quarantine.h"
uint16_t rq_crc(const char *data, size_t size);
size_t rq_boot(char *out, size_t capacity, uint32_t flags);
size_t rq_policy(char *out, size_t capacity);
size_t rq_event_frame(char *out, size_t capacity, const struct rq_event *event);
size_t rq_status(char *out, size_t capacity, const struct quarantine *c);
#endif
