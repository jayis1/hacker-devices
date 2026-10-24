/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef GL_WIRE_H
#define GL_WIRE_H
#include "grant.h"
#include <stddef.h>
uint16_t gl_crc(const char *data, size_t size);
size_t gl_status(char *out, size_t capacity, const struct grant *g);
size_t gl_event_frame(char *out, size_t capacity, const struct gl_event *event);
size_t gl_boot(char *out, size_t capacity, uint32_t flags);
#endif
