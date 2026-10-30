/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef QO_ENGINE_H
#define QO_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QO_QUALIFY 3u
#define QO_SETUP 5u
#define QO_OVERTAKE 1u
#define QO_ORPHAN 2u
#define QO_DUPLICATE 4u
#define QO_INTERFACE 8u
#define QO_GAP 16u
#define QO_EXHAUSTED 32u
#define QO_WATCHDOG 64u
#define QO_WAIT 0u
#define QO_TRACK 1u
/* Each live tag occurs exactly once. order packs head first in low two bits.
 * Its length is population(active); unused bits must be zero. Tag zero is
 * valid, so order alone cannot distinguish an empty queue from [0]. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, active, order, stable;
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
