/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LR_ENGINE_H
#define LR_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LR_QUALIFY 3u
#define LR_DESCENDING 1u
#define LR_AMBIGUOUS 2u
#define LR_COLLISION 4u
#define LR_GAP 8u
#define LR_EXHAUSTED 16u
#define LR_WATCHDOG 32u
/* Rank is LOW < HIGH. Bits: A_LOW, A_HIGH, B_LOW, B_HIGH. */
typedef struct {
    uint32_t observed, baselines, starts, solo, ascending, descending, ambiguous;
} Client;
typedef struct {
    uint32_t now, levels, valid, halted, faults, gaps, missed, ready;
    Client client[2];
    uint32_t overlaps[2], overlap_ticks[2], active, tx_fail;
    uint8_t candidates, runs[4];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
