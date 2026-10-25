/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef CG_CONSENT_H
#define CG_CONSENT_H
#include <stdbool.h>
#include <stdint.h>
#define CG_QUALIFY 3u
#define CG_WINDOW 250u
#define CG_QUEUE 32u
/* Bit order is consent A, consent B, permission, inhibit; active high. */
enum cg_state { CG_UNKNOWN, CG_READY, CG_WAIT_A, CG_WAIT_B, CG_PAIRED, CG_LOCKED };
enum cg_kind { CG_LEVEL=1, CG_STATE, CG_UNSAFE, CG_REJECTED, CG_EXPIRED,
               CG_REVOKED, CG_PAIR, CG_GAP };
struct cg_event {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
};
struct consent {
    uint32_t time;
    uint32_t age;
    uint32_t skew_ms;
    uint32_t pairs;
    uint32_t rejected;
    uint32_t unsafe;
    uint32_t expired;
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
    struct cg_event queue[CG_QUEUE];
    unsigned head;
    unsigned count;
};
void cg_init(struct consent *c, uint32_t now);
void cg_sample(struct consent *c, uint32_t now, uint8_t levels);
void cg_tx_fail(struct consent *c);
bool cg_pop(struct consent *c, struct cg_event *out);
#endif
