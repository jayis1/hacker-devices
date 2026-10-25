/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef DB_BUDGET_H
#define DB_BUDGET_H
#include <stdbool.h>
#include <stdint.h>
#define DB_WINDOW 10000u
#define DB_LIMIT 2000u
#define DB_RESPONSE 20u
/* The counters measure sampled milliseconds, not sub-tick pulse widths. */
enum {
    DB_EXCESS = 1u,
    DB_NO_CONSENT = 2u,
    DB_INHIBITED = 4u,
    DB_NO_ALARM = 8u,
    DB_GAP = 16u,
    DB_TX = 32u,
    DB_WATCHDOG = 64u
};
typedef struct {
    uint8_t slots[DB_WINDOW];
    uint32_t last;
    uint32_t fill;
    uint32_t cursor;
    uint32_t occupied;
    uint32_t unauthorized;
    uint32_t total_active;
    uint32_t total_denied;
    uint32_t excess_episodes;
    uint32_t gaps;
    uint32_t missed;
    uint32_t faults;
    uint32_t tx_fail;
    uint32_t over_age;
    uint32_t peak;
    uint8_t levels;
    bool started;
    bool over;
} Budget;
void budget_init(Budget *b);
void budget_sample(Budget *b, uint32_t now, uint8_t levels);
void budget_tx_failure(Budget *b);
bool budget_valid(const Budget *b);
uint32_t budget_add(uint32_t a, uint32_t b);
#endif
