/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RQ_QUARANTINE_H
#define RQ_QUARANTINE_H
#include <stdbool.h>
#include <stdint.h>
#define RQ_QUALIFY 3u
#define RQ_WINDOW 250u
#define RQ_QUEUE 32u
/* Active-high bits: RESET, READY, ACCESS, FAULT. Observe, never control. */
enum rq_state {
    RQ_UNKNOWN, RQ_RESET, RQ_RELEASED, RQ_SETTLING, RQ_ELIGIBLE, RQ_LOCKED
};
enum rq_kind {
    RQ_LEVEL=1, RQ_STATE, RQ_UNSAFE, RQ_CONTRADICTION,
    RQ_INTERRUPTED, RQ_REVOKED, RQ_RELEASE, RQ_GAP
};
struct rq_event {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
};
struct quarantine {
    uint32_t time;
    uint32_t age;
    uint32_t quiet_ms;
    uint32_t releases;
    uint32_t contradictions;
    uint32_t unsafe;
    uint32_t interrupted;
    uint32_t revoked;
    uint32_t gaps;
    uint32_t missed;
    uint32_t dropped;
    uint32_t tx_fail;
    uint32_t sequence;
    uint8_t levels;
    uint8_t candidate;
    uint8_t stable;
    uint8_t state;
    bool valid;
    bool unsafe_active;
    bool contradiction_active;
    struct rq_event queue[RQ_QUEUE];
    unsigned head;
    unsigned count;
};
void rq_init(struct quarantine *c, uint32_t now);
void rq_sample(struct quarantine *c, uint32_t now, uint8_t levels);
void rq_tx_fail(struct quarantine *c);
bool rq_pop(struct quarantine *c, struct rq_event *out);
#endif
