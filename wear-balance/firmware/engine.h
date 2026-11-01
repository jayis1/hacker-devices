/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef WB_ENGINE_H
#define WB_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define WB_QUALIFY 3u
#define WB_SETUP 5u
#define WB_SLOTS 4u
#define WB_BIAS 1u
#define WB_ORPHAN 2u
#define WB_DUPLICATE 4u
#define WB_INTERFACE 8u
#define WB_GAP 16u
#define WB_EXHAUSTED 32u
#define WB_WATCHDOG 64u
#define WB_WAIT 0u
#define WB_TRACK 1u
/* uses[] survives release and pool drain, but not observer reset. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, depth, stable;
    uint32_t observed, rounds, attempts, launched, completed, rejected;
    uint32_t gaps, missed, queued, peak, uses[WB_SLOTS], tx_fail;
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
