/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef EW_ERASE_H
#define EW_ERASE_H
#include <stdbool.h>
#include <stdint.h>
#define EW_QUALIFY 3u
#define EW_MIN_BUSY 100u
#define EW_START_LIMIT 500u
#define EW_FINISH_LIMIT 5000u
#define EW_QUEUE 32u
/* Input bits: request, busy, done, access. All active high. */
enum ew_state { EW_UNKNOWN, EW_IDLE, EW_WAIT_BUSY, EW_BUSY, EW_COMPLETE, EW_FAULT };
enum ew_kind { EW_LEVEL=1, EW_STATE, EW_UNSAFE, EW_REJECT, EW_TIMEOUT,
               EW_ABORT, EW_COMPLETED, EW_GAP };
struct ew_event {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
};
struct erase {
    uint32_t time;
    uint32_t elapsed;
    uint32_t busy_ms;
    uint32_t completed;
    uint32_t rejected;
    uint32_t unsafe;
    uint32_t timeouts;
    uint32_t aborted;
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
    struct ew_event queue[EW_QUEUE];
    unsigned head;
    unsigned count;
};
void ew_init(struct erase *e, uint32_t now);
void ew_sample(struct erase *e, uint32_t now, uint8_t levels);
void ew_tx_fail(struct erase *e);
bool ew_pop(struct erase *e, struct ew_event *out);
#endif
