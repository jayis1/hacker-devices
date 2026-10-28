/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef PE_ENGINE_H
#define PE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define PE_QUALIFY 3u
#define PE_MIN 10u
#define PE_MAX 200u
#define PE_RECOVERY 50u
#define PE_SHORT 1u
#define PE_LONG 2u
#define PE_RAPID 4u
#define PE_GAP 8u
#define PE_TX 16u
#define PE_EXHAUSTED 32u
#define PE_WATCHDOG 64u
/* phase: 0 unknown (needs low), 1 observed low, 2 tracked high.
 * Age and pulse widths are censored at 201 ms; low age at 50 ms. */
typedef struct {
    uint32_t phase;
    uint32_t age;
    uint32_t starts;
    uint32_t completed;
    uint32_t shorts;
    uint32_t longs;
    uint32_t rapid;
    uint32_t aborted;
    uint32_t last;
    uint32_t peak;
} Channel;
typedef struct {
    uint32_t now;
    uint32_t faults;
    uint32_t gaps;
    uint32_t missed;
    uint32_t tx_fail;
    Channel channel[4];
    uint8_t levels;
    uint8_t valid;
    uint8_t candidate;
    uint8_t age[4];
    bool started;
    bool halted;
} Witness;
void witness_init(Witness *w);
void witness_sample(Witness *w, uint32_t now, uint8_t raw);
void witness_halt(Witness *w, uint32_t reason);
void witness_tx_failure(Witness *w);
bool witness_consistent(const Witness *w);
#endif
