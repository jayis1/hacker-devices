/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RS_ENGINE_H
#define RS_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define RS_QUALIFY 3u
#define RS_SETUP 5u
#define RS_DUPLICATE 1u
#define RS_INVALID 2u
#define RS_GAP 4u
#define RS_EXHAUSTED 8u
#define RS_WATCHDOG 16u
#define RS_WAIT 0u
#define RS_IDLE 1u
#define RS_SESSION 2u
/* TAG0 PA0, TAG1 PA1, ACCEPT PA4, SESSION PB0: input only. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, seen, stable;
    uint32_t observed, sessions, ended, admissions, fresh, duplicate, invalid;
    uint32_t gaps, missed, tx_fail;
    uint8_t candidates, runs[4];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
