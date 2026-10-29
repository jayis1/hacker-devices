/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef AP_ENGINE_H
#define AP_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define AP_QUALIFY 3u
#define AP_ORPHAN 1u
#define AP_INVERSION 2u
#define AP_COLLISION 4u
#define AP_GAP 8u
#define AP_EXHAUSTED 16u
#define AP_WATCHDOG 32u
/* H request, L request, H grant, L grant in increasing bit order. */
typedef struct {
    uint32_t now, levels, valid, halted, faults;
    uint32_t gaps, missed, ready, observed;
    uint32_t starts, accepted, orphans, inversions, collided;
    uint32_t high, low, overlaps, overlap_ticks, collision_active;
    uint32_t baselines, tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    bool started;
    bool exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
