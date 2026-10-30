/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef DD_ENGINE_H
#define DD_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define DD_QUALIFY 3u
#define DD_SETUP 5u
#define DD_DEPENDENCY 1u
#define DD_ORPHAN 2u
#define DD_DUPLICATE 4u
#define DD_INTERFACE 8u
#define DD_GAP 16u
#define DD_EXHAUSTED 32u
#define DD_WATCHDOG 64u
#define DD_WAIT 0u
#define DD_TRACK 1u
/* TAG0/1 plus START PA4 and FINISH PB0. Graph: 0 -> {1,2} -> 3. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, done, stable;
    uint32_t observed, rounds, attempts, launched, completed, rejected;
    uint32_t gaps, missed, tx_fail;
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
