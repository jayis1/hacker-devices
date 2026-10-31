/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef SG_ENGINE_H
#define SG_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define SG_QUALIFY 3u
#define SG_SETUP 5u
#define SG_GENERATION 1u
#define SG_ORPHAN 2u
#define SG_DUPLICATE 4u
#define SG_INTERFACE 8u
#define SG_GAP 16u
#define SG_EXHAUSTED 32u
#define SG_WATCHDOG 64u
#define SG_WAIT 0u
#define SG_TRACK 1u
/* order bits 0..1 retain last generation for slots 0..1; bits 2..3
 * retain which slots have ever been allocated. Released history survives. */
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
