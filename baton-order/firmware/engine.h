/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef BO_ENGINE_H
#define BO_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define BO_QUALIFY 3u
#define BO_REPEAT 1u
#define BO_SKIP 2u
#define BO_REVERSE 4u
#define BO_COLLISION 8u
#define BO_GAP 16u
#define BO_EXHAUSTED 32u
#define BO_WATCHDOG 64u
#define BO_NONE 4u
/* Matrix row is previous owner, column is new owner. */
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t halted;
    uint32_t faults;
    uint32_t observed;
    uint32_t gaps;
    uint32_t missed;
    uint32_t anchors;
    uint32_t forward;
    uint32_t skip;
    uint32_t reverse;
    uint32_t repeat;
    uint32_t collisions;
    uint32_t last;
    uint32_t ready;
    uint32_t idle;
    uint32_t collision_active;
    uint32_t matrix[16];
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    bool started;
    bool exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
