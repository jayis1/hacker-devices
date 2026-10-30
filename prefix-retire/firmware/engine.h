/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef PR_ENGINE_H
#define PR_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define PR_QUALIFY 3u
#define PR_SETUP 5u
#define PR_ORDER 1u
#define PR_UNREADY 2u
#define PR_DUPLICATE 4u
#define PR_INTERFACE 8u
#define PR_GAP 16u
#define PR_EXHAUSTED 32u
#define PR_WATCHDOG 64u
#define PR_WAIT 0u
#define PR_TRACK 1u
/* Two-bit ordinal plus READY PA4 and RETIRE PB0; all target pins inputs. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, ready, next, stable;
    uint32_t observed, rounds, attempts, prepared, retired, rejected;
    uint32_t gaps, missed, tx_fail;
    uint8_t candidates, runs[3];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
