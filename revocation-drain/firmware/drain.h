/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RD_DRAIN_H
#define RD_DRAIN_H
#include <stdint.h>
#include <stdbool.h>
#define RD_QUIET_MS 20u
#define RD_DEADLINE_MS 500u
#define RD_REVOKE 1u
#define RD_ADMIT 2u
#define RD_BUSY 4u
#define RD_ACK 8u
enum { RD_WAIT, RD_READY, RD_DRAIN, RD_SEALED, RD_FAULT };
enum {
    RD_LEAK=1u, RD_EARLY=2u, RD_TIMEOUT=4u, RD_WITHDRAWN=8u,
    RD_REOPEN=16u, RD_GAP=32u, RD_TX=64u, RD_WATCHDOG=128u,
    RD_OVERFLOW=256u, RD_ORPHAN=512u
};
typedef struct {
    uint32_t last;
    uint32_t started;
    uint32_t completed;
    uint32_t abandoned;
    uint32_t age;
    uint32_t quiet;
    uint32_t admissions;
    uint32_t blocked;
    uint32_t early;
    uint32_t gaps;
    uint32_t tx_fail;
    uint32_t faults;
    uint8_t levels;
    uint8_t state;
    bool initialized;
    bool quiet_valid;
} Drain;
void drain_init(Drain *d);
void drain_sample(Drain *d, uint32_t now, uint8_t levels);
void drain_halt(Drain *d, uint32_t reason);
void drain_tx_failure(Drain *d);
bool drain_consistent(const Drain *d);
#endif
