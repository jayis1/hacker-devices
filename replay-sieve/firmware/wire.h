/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_WIRE_H
#define QC_WIRE_H
#include <stddef.h>
#include "engine.h"
size_t wire_boot(char *out, size_t cap, uint32_t reset);
size_t wire_policy(char *out, size_t cap);
size_t wire_status(char *out, size_t cap, const Ledger *s);
#endif
