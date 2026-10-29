/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef ST_ENGINE_H
#define ST_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define ST_QUALIFY 3u
#define ST_MISSING 1u
#define ST_SPURIOUS 2u
#define ST_GAP 4u
#define ST_EXHAUSTED 8u
#define ST_WATCHDOG 16u
#define ST_REJECTED 32u
/* TRIP, CLEAR, AUTH, LATCH; set dominates clear. All are active high. */
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t halted;
    uint32_t faults;
    uint32_t gaps;
    uint32_t missed;
    uint32_t known;
    uint32_t expected;
    uint32_t sets;
    uint32_t clears;
    uint32_t rejected;
    uint32_t coverage;
    uint32_t missing;
    uint32_t spurious;
    uint32_t totals[2];
    uint32_t episodes[2];
    uint32_t streak[2];
    uint32_t maximum[2];
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    uint8_t previous;
    bool tracked;
    bool started;
    bool exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
