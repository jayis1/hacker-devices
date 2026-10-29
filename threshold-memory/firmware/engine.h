/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef TM_ENGINE_H
#define TM_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define TM_QUALIFY 3u
#define TM_SETTLE 10u
#define TM_MISSING 1u
#define TM_SPURIOUS 2u
#define TM_GAP 4u
#define TM_EXHAUSTED 8u
#define TM_WATCHDOG 16u
/* State 0 has no history; 1 expects low, 2 expects high. */
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t halted;
    uint32_t faults;
    uint32_t gaps;
    uint32_t missed;
    uint32_t state;
    uint32_t settle;
    uint32_t mismatch;
    uint32_t coverage;
    uint32_t good;
    uint32_t bad_high;
    uint32_t bad_low;
    uint32_t episodes;
    uint32_t streak;
    uint32_t longest;
    uint32_t rises;
    uint32_t falls;
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[3];
    bool started;
    bool exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
