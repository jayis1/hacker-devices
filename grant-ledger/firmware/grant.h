/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef GL_GRANT_H
#define GL_GRANT_H
#include <stdint.h>
#include <stdbool.h>
#define GL_REQUEST 1u
#define GL_GRANT 2u
#define GL_USE 4u
#define GL_REVOKE 8u
#define GL_WINDOW_MS 1000u
#define GL_QUALIFY 3u
#define GL_QUEUE 32u
enum gl_state { GL_IDLE, GL_PENDING, GL_CREDIT };
enum gl_fault {
    GL_NO_CREDIT, GL_ORPHAN, GL_OVERLAP, GL_AMBIGUOUS,
    GL_REVOKED_USE, GL_EXPIRED, GL_FAULT_COUNT
};
enum gl_kind { GL_LEVEL=1, GL_FAULT, GL_STATE, GL_GAP, GL_ACCEPT, GL_SYNC };
struct gl_event {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
};
struct grant {
    uint32_t time;
    uint32_t age;
    uint32_t requests;
    uint32_t grants;
    uint32_t accepted;
    uint32_t revoked;
    uint32_t gaps;
    uint32_t missed;
    uint32_t dropped;
    uint32_t tx_fail;
    uint32_t sequence;
    uint32_t faults[GL_FAULT_COUNT];
    uint8_t levels;
    uint8_t candidate;
    uint8_t stable;
    uint8_t state;
    bool synced;
    struct gl_event queue[GL_QUEUE];
    unsigned head;
    unsigned used;
};
void grant_init(struct grant *g, uint32_t time);
void grant_sample(struct grant *g, uint32_t time, uint8_t raw);
bool grant_pop(struct grant *g, struct gl_event *out);
void grant_tx_fail(struct grant *g);
uint32_t gl_sat_add(uint32_t a, uint32_t b);
#endif
