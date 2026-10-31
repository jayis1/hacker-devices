/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef UQ_ENGINE_H
#define UQ_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define OB_QUALIFY 3u
#define OB_SETUP 5u
#define OB_EARLY 1u
#define OB_ORPHAN 2u
#define OB_DUPLICATE 4u
#define OB_INTERFACE 8u
#define OB_GAP 16u
#define OB_EXHAUSTED 32u
#define OB_WATCHDOG 64u
#define OB_WAIT 0u
#define OB_TRACK 1u
/* Disjoint per-slot masks: active means allocated; cooling means quarantine.
 * A slot with neither bit is available. Remaining ticks freeze on a halt.
 * OB_ prefixes are inherited platform conventions, not wire identifiers. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, cooling, stable;
    uint32_t observed, rounds, attempts, launched, completed, rejected;
    uint32_t gaps, missed, expired, peak, tx_fail;
    uint32_t remaining[2];
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
