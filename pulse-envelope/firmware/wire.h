/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef PE_WIRE_H
#define PE_WIRE_H
#include "engine.h"
#include <stddef.h>
size_t wire_status(char *out, size_t size, const Witness *w);
size_t wire_policy(char *out, size_t size);
size_t wire_boot(char *out, size_t size, uint32_t reset);
#endif
