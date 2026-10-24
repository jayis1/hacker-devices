/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LEASE_WIRE_H
#define LEASE_WIRE_H
#include "lease.h"
#define WIRE_CAPACITY 384u
size_t wire_status(char *out, size_t cap, const lease_t *s, uint32_t now);
size_t wire_event(char *out, size_t cap, const lease_event_t *event);
size_t wire_boot(char *out, size_t cap, uint32_t reset_flags);
#endif
