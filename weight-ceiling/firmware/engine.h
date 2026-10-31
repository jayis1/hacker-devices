/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef WC_ENGINE_H
#define WC_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define WC_QUALIFY 3u
#define WC_SETUP 5u
#define WC_CEILING 5u
#define WC_BUDGET 1u
#define WC_ORPHAN 2u
#define WC_DUPLICATE 4u
#define WC_INTERFACE 8u
#define WC_GAP 16u
#define WC_EXHAUSTED 32u
#define WC_WATCHDOG 64u
#define WC_WAIT 0u
#define WC_TRACK 1u
/* Four unique tags cost tag+1 credits while occupied. No temporal refill. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, load, stable;
    uint32_t observed, rounds, attempts, launched, completed, rejected;
    uint32_t gaps, missed, queued, peak, tx_fail;
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
