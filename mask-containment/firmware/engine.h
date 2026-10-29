/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef MC_ENGINE_H
#define MC_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define MC_QUALIFY 3u
#define MC_EXCESS0 1u
#define MC_EXCESS1 2u
#define MC_GAP 4u
#define MC_EXHAUSTED 8u
#define MC_WATCHDOG 16u
/* Raw bits: allowed0, allowed1, effective0, effective1.
   Pair histogram state = allowed + 2*effective. Denial is not a fault. */
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t halted;
    uint32_t faults;
    uint32_t gaps;
    uint32_t missed;
    uint32_t excess;
    uint32_t denied;
    uint32_t bins[8];
    uint32_t episodes[4];
    uint32_t streak[4];
    uint32_t maximum[4];
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
