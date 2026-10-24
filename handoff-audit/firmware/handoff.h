/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef HA_HANDOFF_H
#define HA_HANDOFF_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define HA_QUALIFY 3u
#define HA_BREAK_MS 20u
#define HA_SETTLE_MS 50u
#define HA_QUEUE 32u
/* Input vector: command A, command B, feedback A, feedback B. */
enum ha_kind {
    HA_LEVELS=1,
    HA_COMMAND_OVERLAP,
    HA_FEEDBACK_OVERLAP,
    HA_SHORT_BREAK,
    HA_MISMATCH_A,
    HA_MISMATCH_B,
    HA_TRANSITION,
    HA_GAP,
    HA_SYNC
};
struct ha_event {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
};
struct handoff {
    uint32_t time;
    uint8_t levels;
    uint8_t candidate;
    uint8_t samples;
    bool synced;
    uint8_t last_path;
    bool off_seen;
    uint32_t off_age;
    uint32_t last_dead;
    uint32_t transitions;
    uint32_t command_overlap;
    uint32_t feedback_overlap;
    uint32_t short_break;
    uint32_t mismatch[2];
    uint32_t mismatch_age[2];
    bool mismatch_active[2];
    bool mismatch_reported[2];
    uint32_t gaps;
    uint32_t missed;
    uint32_t dropped;
    uint32_t tx_fail;
    uint32_t sequence;
    struct ha_event queue[HA_QUEUE];
    unsigned head;
    unsigned count;
};
void ha_init(struct handoff *h, uint32_t now);
void ha_sample(struct handoff *h, uint32_t now, uint8_t levels);
bool ha_pop(struct handoff *h, struct ha_event *event);
void ha_tx_fail(struct handoff *h);
#endif
