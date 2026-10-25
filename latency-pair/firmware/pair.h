/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LP_PAIR_H
#define LP_PAIR_H
#include <stdbool.h>
#include <stdint.h>
#define LP_MAX_DURATION 1000u
#define LP_PAIR_WAIT 10000u
#define LP_COHORT 32u
#define LP_THRESHOLD 5u
/* Input bit positions follow J2 pin order. */
#define LP_BUSY 1u
#define LP_CLASS 2u
#define LP_VALID 4u
#define LP_STOP 8u
#define LP_LABEL (1u << 0)
#define LP_INVALID (1u << 1)
#define LP_INHIBIT (1u << 2)
#define LP_TIMEOUT (1u << 3)
#define LP_ORDER (1u << 4)
#define LP_GAP (1u << 5)
#define LP_TX (1u << 6)
#define LP_WATCHDOG (1u << 7)
enum { LP_WAIT, LP_READY, LP_ACTIVE, LP_DONE };
typedef struct {
    uint32_t last;
    uint32_t state;
    uint32_t levels;
    uint32_t pairs;
    uint32_t sum_a;
    uint32_t sum_b;
    uint32_t a_slower;
    uint32_t b_slower;
    uint32_t equal;
    uint32_t large;
    uint32_t max_delta;
    uint32_t rejected;
    uint32_t gaps;
    uint32_t tx_fail;
    uint32_t faults;
    uint32_t age;
    uint32_t pending_age;
    uint32_t first_duration;
    uint8_t first_label;
    uint8_t active_label;
    bool initialized;
    bool ready;
    bool active;
    bool pending;
} Pair;
void pair_init(Pair *p);
void pair_sample(Pair *p, uint32_t now, uint8_t levels);
void pair_tx_failure(Pair *p);
bool pair_valid(const Pair *p);
#endif
