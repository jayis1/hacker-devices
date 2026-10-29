/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef SF_ENGINE_H
#define SF_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define SF_QUALIFY 3u
#define SF_CHANGED 1u
#define SF_UNLOCKED 2u
#define SF_GAP 4u
#define SF_EXHAUSTED 8u
#define SF_WATCHDOG 16u
#define SF_REJECTED 32u
#define SF_UNKNOWN 0u
#define SF_OPEN 1u
#define SF_SEALED 2u
/* CFG0, CFG1, SEAL, RESET. RESET dominates all policy decisions. */
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t halted;
    uint32_t faults;
    uint32_t gaps;
    uint32_t missed;
    uint32_t state;
    uint32_t captured;
    uint32_t seals;
    uint32_t resets;
    uint32_t rejected;
    uint32_t coverage;
    uint32_t mismatch;
    uint32_t unsealed;
    uint32_t totals[2];
    uint32_t episodes[2];
    uint32_t streak[2];
    uint32_t maximum[2];
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    uint8_t previous;
    bool reset_seen;
    bool tracked;
    bool started;
    bool exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
