/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef PL_ENGINE_H
#define PL_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define PL_SETUP 5u
#define PL_HOLD 10u
#define PL_QUALIFY 3u
/* Sticky findings; counts may overlap within a single transfer. */
enum {
    PL_PARITY=1u, PL_SETUP_FAULT=2u, PL_HOLD_FAULT=4u,
    PL_COLLISION=8u, PL_GAP=16u, PL_EXHAUSTED=32u, PL_WATCHDOG=64u
};
typedef struct {
    uint32_t now, faults;
    uint32_t starts, completed, good, bad, aborted;
    uint32_t parity, setup, hold, collisions, gaps, missed, tx_fail;
    uint32_t bins[4];
    uint8_t levels, valid, candidate, stable[4];
    uint8_t captured, age, quiet, trial_faults;
    bool started, active, low_seen, halted;
} Latch;
void latch_init(Latch *c);
void latch_sample(Latch *c, uint32_t now, uint8_t raw);
void latch_halt(Latch *c, uint32_t fault);
void latch_tx_failure(Latch *c);
bool latch_valid(const Latch *c);
#endif
