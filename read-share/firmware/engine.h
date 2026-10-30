/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RS_ENGINE_H
#define RS_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define RS_QUALIFY 3u
#define RS_SETUP 5u
#define RS_CONFLICT 1u
#define RS_ORPHAN 2u
#define RS_DUPLICATE 4u
#define RS_INTERFACE 8u
#define RS_GAP 16u
#define RS_EXHAUSTED 32u
#define RS_WATCHDOG 64u
#define RS_WAIT 0u
#define RS_TRACK 1u
/* TAG0/1 plus ACQUIRE PA4 and RELEASE PB0. Readers 0..2; writer 3. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, seen, stable;
    uint32_t observed, rounds, attempts, launched, completed, rejected;
    uint32_t gaps, missed, readers, writers, shared, tx_fail;
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
