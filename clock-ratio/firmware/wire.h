/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef CR_WIRE_H
#define CR_WIRE_H
#include "engine.h"
#include <stddef.h>
size_t wire_boot(char *out, size_t cap, uint32_t reset);
size_t wire_policy(char *out, size_t cap);
size_t wire_status(char *out, size_t cap, const Clock *c);
#endif
