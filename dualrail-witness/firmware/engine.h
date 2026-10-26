/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef DW_ENGINE_H
#define DW_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define DW_QUALIFY 3u
#define DW_GRACE 20u
#define DW_INVALID 1u
#define DW_UNSAFE 2u
#define DW_SILENT 4u
#define DW_GAP 8u
#define DW_TX 16u
#define DW_EXHAUSTED 32u
#define DW_WATCHDOG 64u
/* Bit order: ALLOW, DENY, ENABLE, FAULT. All active high. */
typedef struct {
    uint32_t now;
    uint32_t invalid;
    uint32_t unsafe;
    uint32_t silent;
    uint32_t invalid_ms;
    uint32_t unsafe_ms;
    uint32_t silent_ms;
    uint32_t max_invalid;
    uint32_t max_unsafe;
    uint32_t gaps;
    uint32_t missed;
    uint32_t tx_fail;
    uint32_t faults;
    uint8_t levels;
    uint8_t valid;
    uint8_t candidate;
    uint8_t age[4];
    bool started;
    bool halted;
    bool invalid_active;
    bool unsafe_active;
    bool silent_active;
    bool silent_reported;
} Witness;
void witness_init(Witness *w);
void witness_sample(Witness *w, uint32_t now, uint8_t raw);
void witness_halt(Witness *w, uint32_t reason);
void witness_tx_failure(Witness *w);
bool witness_consistent(const Witness *w);
#endif
