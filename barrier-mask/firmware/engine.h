/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef BM_ENGINE_H
#define BM_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define BM_CAPACITY 4u
#define BM_SETUP 5u
#define BM_QUALIFY 3u
#define BM_BASELINE 20u
#define BM_DEADLINE 1000u
enum {
    BM_DUPLICATE=1u, BM_EARLY=2u, BM_TIMEOUT=4u, BM_COLLISION=8u,
    BM_UNSTABLE=16u, BM_GAP=32u, BM_EXHAUSTED=64u, BM_WATCHDOG=128u
};
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t ready;
    uint32_t halted;
    uint32_t faults;
    uint32_t mask;
    uint32_t active;
    uint32_t age;
    uint32_t rounds;
    uint32_t arrivals;
    uint32_t releases;
    uint32_t accepted;
    uint32_t completed;
    uint32_t abandoned;
    uint32_t discarded;
    uint32_t duplicate;
    uint32_t early;
    uint32_t collision;
    uint32_t unstable;
    uint32_t timeouts;
    uint32_t gaps;
    uint32_t missed;
    uint32_t tx_fail;
    uint8_t candidates;
    uint8_t runs[4];
    uint8_t baseline;
    uint8_t setup;
    bool started;
    bool lost;
    bool exhausted;
} Barrier;
void barrier_init(Barrier *s);
void barrier_sample(Barrier *s, uint32_t now, uint8_t raw);
void barrier_halt(Barrier *s, uint32_t reason);
void barrier_tx_failure(Barrier *s);
bool barrier_valid(const Barrier *s);
#endif
