/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef SA_ENGINE_H
#define SA_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define SA_CAPACITY 4u
#define SA_SETUP 5u
#define SA_QUALIFY 3u
#define SA_BASELINE 20u
enum {
    SA_DUPLICATE = 1u,
    SA_ORPHAN = 2u,
    /* Bit 2 is reserved, not a finding. */
    SA_COLLISION = 8u,
    SA_UNSTABLE = 16u,
    SA_GAP = 32u,
    SA_EXHAUSTED = 64u,
    SA_WATCHDOG = 128u
};
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t ready;
    uint32_t halted;
    uint32_t faults;
    uint32_t pending;
    uint32_t occupied;
    uint32_t allocations;
    uint32_t releases;
    uint32_t retired;
    uint32_t abandoned;
    uint32_t duplicate;
    uint32_t orphan;
    uint32_t reuse;
    uint32_t collision;
    uint32_t unstable;
    uint32_t gaps;
    uint32_t missed;
    uint32_t peak;
    uint32_t tx_fail;
    uint8_t seen;
    uint8_t candidates;
    uint8_t runs[4];
    uint8_t baseline;
    uint8_t setup;
    bool started;
    bool lost;
    bool exhausted;
} Slots;
void slots_init(Slots *s);
void slots_sample(Slots *s, uint32_t now, uint8_t raw);
void slots_halt(Slots *s, uint32_t reason);
void slots_tx_failure(Slots *s);
bool slots_valid(const Slots *s);
#endif
