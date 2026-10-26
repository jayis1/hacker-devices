/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef BB_BUDGET_H
#define BB_BUDGET_H
#include <stdbool.h>
#include <stdint.h>
#define BB_CAPACITY 4u
#define BB_PERIOD 1000u
#define BB_GRACE 20u
#define BB_QUALIFY 3u
#define BB_BASELINE 20u
enum { BB_UNKNOWN, BB_TRACKING, BB_HALTED };
enum {
    BB_EXCESS=1u, BB_DENIED=2u, BB_MISSING=4u, BB_GAP=8u,
    BB_TX=16u, BB_EXHAUSTED=32u, BB_WATCHDOG=64u
};
typedef struct {
    uint32_t now;
    uint32_t phase;
    uint32_t levels;
    uint32_t qualified;
    uint32_t tokens;
    uint32_t refill;
    uint32_t empty_age;
    uint32_t faults;
    uint32_t admissions;
    uint32_t allowed;
    uint32_t excess;
    uint32_t denied;
    uint32_t missing;
    uint32_t gaps;
    uint32_t segments;
    uint32_t tx_fail;
    uint32_t minted;
    uint32_t discarded;
    uint32_t baseline;
    uint8_t candidate[4];
    uint8_t stable[4];
    bool started;
} Budget;
void budget_init(Budget *s);
void budget_sample(Budget *s, uint32_t now, unsigned raw);
void budget_halt(Budget *s, uint32_t reason);
void budget_tx_failure(Budget *s);
bool budget_valid(const Budget *s);
#endif
