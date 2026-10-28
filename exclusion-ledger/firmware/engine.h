/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef EL_ENGINE_H
#define EL_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define EL_QUALIFY 3u
#define EL_GAP 64u
#define EL_EXHAUSTED 128u
#define EL_WATCHDOG 256u
#define EL_PAIRS 6u
/* Pair order: 01, 02, 03, 12, 13, 23. */
typedef struct {
    uint32_t entries;
    uint32_t duration;
    uint32_t maximum;
    uint32_t age;
    uint32_t active;
} Pair;
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t halted;
    uint32_t faults;
    uint32_t observed;
    uint32_t gaps;
    uint32_t missed;
    Pair pair[EL_PAIRS];
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    bool started;
    bool exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
