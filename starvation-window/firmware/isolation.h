/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef IW_ISOLATION_H
#define IW_ISOLATION_H
#include <stdbool.h>
#include <stdint.h>
#define IW_DEAD_MS 20u
#define IW_WAIT_MS 100u
#define IW_BYPASSES 3u
#define IW_BASELINE 0u
#define IW_READY 1u
#define IW_HALTED 2u
#define IW_OVERLAP (1u << 0)
#define IW_EARLY (1u << 1)
#define IW_UNREQUESTED (1u << 2)
#define IW_GAP (1u << 3)
#define IW_TX (1u << 4)
#define IW_WATCHDOG (1u << 5)
#define IW_EXHAUSTED (1u << 6)
#define IW_STARVED_A (1u << 7)
#define IW_STARVED_B (1u << 8)
/* All counters are lifetime observations, not certified physical events. */
typedef struct {
    uint32_t now;
    uint32_t state;
    uint32_t levels;
    uint32_t quiet;
    uint32_t last_owner;
    uint32_t switches;
    uint32_t early;
    uint32_t overlaps;
    uint32_t overlap_ms;
    uint32_t unrequested;
    uint32_t gaps;
    uint32_t tx_fail;
    uint32_t faults;
    uint32_t minimum;
    uint32_t histogram[4];
    uint32_t wait[2];
    uint32_t bypass[2];
    uint32_t starved[2];
    uint32_t services[2];
    bool reported[2];
    bool initialized;
} Isolation;
void isolation_init(Isolation *s);
bool isolation_valid(const Isolation *s);
void isolation_sample(Isolation *s, uint32_t now, uint8_t levels);
void isolation_halt(Isolation *s, uint32_t reason);
void isolation_tx_failure(Isolation *s);
#endif
