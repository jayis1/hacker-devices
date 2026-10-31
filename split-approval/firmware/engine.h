/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef SP_ENGINE_H
#define SP_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define SP_QUALIFY 3u
#define SP_SETUP 5u
#define SP_SELF 1u
#define SP_ORPHAN 2u
#define SP_DUPLICATE 4u
#define SP_INTERFACE 8u
#define SP_GAP 16u
#define SP_EXHAUSTED 32u
#define SP_WATCHDOG 64u
#define SP_WAIT 0u
#define SP_TRACK 1u
/* order bits 0..1 retain last proposer for slots 0..1; bits 2..3 retain
 * ever-proposed membership. Approvers must differ from the stored proposer. */
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
