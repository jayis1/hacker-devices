/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef QW_QUORUM_H
#define QW_QUORUM_H
#include <stdbool.h>
#include <stdint.h>
#define QW_SETTLE_MS 50u
#define QW_DISSENT_MS 100u
#define QW_QUALIFY 3u
#define QW_QUEUE 32u

enum qw_kind {
    QW_LEVELS = 1,
    QW_UNSAFE,
    QW_DENIED,
    QW_DISSENT_A,
    QW_DISSENT_B,
    QW_DISSENT_C,
    QW_GAP,
    QW_READY
};

struct qw_event {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
};

struct quorum {
    uint32_t time;
    uint8_t levels;
    uint8_t candidate;
    uint8_t consecutive;
    bool valid;
    uint8_t majority;
    uint8_t dissent;
    uint8_t alarms;
    uint8_t mismatch;
    uint32_t unsafe;
    uint32_t denied;
    uint32_t dissent_count[3];
    uint32_t mismatch_ms;
    uint32_t max_mismatch;
    uint32_t dissent_ms[3];
    uint32_t max_dissent[3];
    uint32_t gaps;
    uint32_t missed;
    uint32_t dropped;
    uint32_t tx_fail;
    uint32_t sequence;
    struct qw_event queue[QW_QUEUE];
    unsigned head;
    unsigned count;
};

void qw_init(struct quorum *q, uint32_t now);
void qw_sample(struct quorum *q, uint32_t now, uint8_t raw);
bool qw_pop(struct quorum *q, struct qw_event *event);
void qw_tx_fail(struct quorum *q);
#endif
