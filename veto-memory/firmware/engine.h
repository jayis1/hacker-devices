/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef VM_ENGINE_H
#define VM_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define VM_QUALIFY 3u
#define VM_EARLY 1u
#define VM_VETOED 2u
#define VM_REPEAT 4u
#define VM_GAP 8u
#define VM_EXHAUSTED 16u
#define VM_WATCHDOG 32u
#define VM_WAIT 0u
#define VM_READY 1u
#define VM_ACTIVE 2u
#define VM_DRAIN 3u
/* Bits: PREP_A, PREP_B, COMMIT, ABORT. No target outputs. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, veto, seen;
    uint32_t observed, baselines, rounds, accepted, early, vetoed;
    uint32_t cancelled, abandoned, commits, repeated, veto_rounds, withdrawals;
    uint32_t gaps, missed, tx_fail;
    uint8_t candidates, runs[4];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
