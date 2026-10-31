/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef OB_ENGINE_H
#define OB_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define OB_QUALIFY 3u
#define OB_SETUP 5u
#define OB_BINDING 1u
#define OB_ORPHAN 2u
#define OB_DUPLICATE 4u
#define OB_INTERFACE 8u
#define OB_GAP 16u
#define OB_EXHAUSTED 32u
#define OB_WATCHDOG 64u
#define OB_WAIT 0u
#define OB_TRACK 1u
/* Platform OB_ names are retained internally; client denotes security domain.
 * order bits 0..1 retain first domain for slots 0..1; bits 2..3 retain
 * ever-bound membership. This is neither FIFO order nor generation parity. */
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
