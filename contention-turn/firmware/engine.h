/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef CT_ENGINE_H
#define CT_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define CT_QUALIFY 3u
#define CT_ORPHAN 1u
#define CT_UNFAIR 2u
#define CT_COLLISION 4u
#define CT_GAP 8u
#define CT_EXHAUSTED 16u
#define CT_WATCHDOG 32u
/* Bit order: request A, request B, grant A, grant B.
 * turn: zero unknown; one prefers A; two prefers B. */
typedef struct {
    uint32_t now, levels, valid, halted, faults;
    uint32_t gaps, missed, ready, observed;
    uint32_t starts, accepted, orphans, unfair, collided;
    uint32_t side_a, side_b, overlaps, overlap_ticks, collision_active;
    uint32_t baselines, turn, seeds, contended, tx_fail;
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
