/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef UQ_ENGINE_H
#define UQ_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define UQ_QUALIFY 3u
#define UQ_SETUP 5u
#define UQ_SLOTS 4u
#define UQ_RANK 1u
#define UQ_LIMIT 3u /* Highest fixed rank; policy announcement, not a quota. */
#define UQ_ORPHAN 2u
#define UQ_DUPLICATE 4u
#define UQ_INTERFACE 8u
#define UQ_GAP 16u
#define UQ_EXHAUSTED 32u
#define UQ_WATCHDOG 64u
#define UQ_WAIT 0u
#define UQ_TRACK 1u
/* uses[] survives release and pool drain, but not observer reset. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, depth, stable;
    uint32_t observed, rounds, attempts, launched, completed, rejected;
    uint32_t gaps, missed, queued, peak, uses[UQ_SLOTS], stamp[UQ_SLOTS], tx_fail;
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
bool ledger_eligible(const Ledger *s, unsigned tag);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
