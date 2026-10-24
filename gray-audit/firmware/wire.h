/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef GRAY_WIRE_H
#define GRAY_WIRE_H
#include "audit.h"
#include <stddef.h>
#define WIRE_CAPACITY 384u
size_t wire_axis(char *out, size_t capacity, const audit_t *a, unsigned index, uint32_t now);
size_t wire_histogram(char *out, size_t capacity, const audit_t *a, unsigned index, uint32_t now);
size_t wire_health(char *out, size_t capacity, const audit_t *a, uint32_t now);
size_t wire_event(char *out, size_t capacity, const audit_event_t *e);
size_t wire_boot(char *out, size_t capacity, uint32_t flags);
#endif
