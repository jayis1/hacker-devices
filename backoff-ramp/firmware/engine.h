/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef BR_ENGINE_H
#define BR_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define BR_QUALIFY 3u
#define BR_BASE 10u
#define BR_CAP 7u
#define BR_DEADLINE 1000u
#define BR_EARLY 1u
#define BR_MALFORMED 2u
#define BR_GAP 4u
#define BR_EXHAUSTED 8u
#define BR_WATCHDOG 16u
#define BR_TIMEOUT 32u
#define BR_UNKNOWN 0u
#define BR_READY 1u
#define BR_PENDING 2u
/* ATTEMPT, FAILURE, SUCCESS, RESET; diagnostic inputs only. */
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t halted;
    uint32_t faults;
    uint32_t gaps;
    uint32_t missed;
    uint32_t state;
    uint32_t grade;
    uint32_t remaining;
    uint32_t age;
    uint32_t attempts;
    uint32_t failures;
    uint32_t successes;
    uint32_t abandoned;
    uint32_t early;
    uint32_t malformed;
    uint32_t timeouts;
    uint32_t resets;
    uint32_t coverage;
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    uint8_t previous;
    bool baseline;
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
