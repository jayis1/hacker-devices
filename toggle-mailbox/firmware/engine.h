/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef TM_ENGINE_H
#define TM_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define TM_SETUP 5u
#define TM_QUALIFY 3u
#define TM_BASELINE 20u
#define TM_DEADLINE 1000u
enum {
    TM_SETUP_FAIL=1u, TM_HOLD=2u, TM_OVERTAKE=4u, TM_ORPHAN=8u,
    TM_OVERLAP=16u, TM_TIMEOUT=32u, TM_GAP=64u,
    TM_EXHAUSTED=128u, TM_WATCHDOG=256u
};
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t ready;
    uint32_t halted;
    uint32_t faults;
    uint32_t captured;
    uint32_t active;
    uint32_t age;
    uint32_t starts;
    uint32_t acks;
    uint32_t completed;
    uint32_t abandoned;
    uint32_t setup_fail;
    uint32_t hold;
    uint32_t overtake;
    uint32_t orphan;
    uint32_t overlap;
    uint32_t timeouts;
    uint32_t gaps;
    uint32_t missed;
    uint32_t max_latency;
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    uint8_t baseline;
    uint8_t setup;
    bool started;
    bool lost;
    bool exhausted;
} Mailbox;
void mailbox_init(Mailbox *s);
void mailbox_sample(Mailbox *s, uint32_t now, uint8_t raw);
void mailbox_halt(Mailbox *s, uint32_t reason);
void mailbox_tx_failure(Mailbox *s);
bool mailbox_valid(const Mailbox *s);
#endif
