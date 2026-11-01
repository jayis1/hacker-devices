/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef BS_ENGINE_H
#define BS_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define BS_QUALIFY 3u
#define BS_SETUP 5u
#define BS_SLOTS 4u
#define BS_CONFLICT 1u
#define BS_ORPHAN 2u
#define BS_DUPLICATE 4u
#define BS_INTERFACE 8u
#define BS_GAP 16u
#define BS_EXHAUSTED 32u
#define BS_WATCHDOG 64u
#define BS_WAIT 0u
#define BS_TRACK 1u
/* uses[] survives release and pool drain, but not observer reset. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, depth, stable;
    uint32_t observed, rounds, attempts, launched, completed, rejected;
    uint32_t gaps, missed, queued, peak, uses[BS_SLOTS], tx_fail;
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
uint32_t ledger_conflicts(unsigned tag);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
