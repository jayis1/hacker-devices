/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef CR_ENGINE_H
#define CR_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define CR_DIVISOR 4u
#define CR_DEADLINE 100u
#define CR_QUALIFY 3u
enum {
    CR_RATIO=1u, CR_EXCESS=2u, CR_AMBIGUOUS=4u, CR_TIMEOUT=8u,
    CR_OUTSIDE=16u, CR_GAP=32u, CR_EXHAUSTED=64u, CR_WATCHDOG=128u
};
typedef struct {
    uint32_t now, faults, intervals, good, bad, aborted;
    uint32_t ambiguous, timeouts, outside, gaps, missed, tx_fail;
    uint32_t anchors, refs, age;
    uint8_t levels, valid, candidate, stable[4];
    bool started, anchored, tick_low, halted;
} Clock;
void clock_init(Clock *c);
void clock_sample(Clock *c, uint32_t now, uint8_t raw);
void clock_halt(Clock *c, uint32_t fault);
void clock_tx_failure(Clock *c);
bool clock_valid(const Clock *c);
#endif
