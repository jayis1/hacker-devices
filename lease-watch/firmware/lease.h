/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LEASE_H
#define LEASE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Revision A policy: milliseconds, active-high enable and ready. */
#define LEASE_TIMEOUT 250u
#define LEASE_QUALIFY 3u
#define LEASE_QUEUE 32u
#define LEASE_NO_HEARTBEAT 1u
#define LEASE_EXPIRED 2u
#define LEASE_RESET_ENABLED 4u
#define LEASE_NOT_READY 8u
#define LEASE_UNCERTAIN 16u
#define LEASE_HB 1u
#define LEASE_ENABLE 2u
#define LEASE_RESET_N 4u
#define LEASE_READY 8u
#define LEASE_ALARM_COUNT 5u

typedef enum {
    EVENT_LEVEL = 1,
    EVENT_ALARM = 2,
    EVENT_GAP = 3,
    EVENT_HEARTBEAT = 4
} lease_kind_t;

typedef struct {
    uint32_t sequence;
    uint32_t time;
    uint32_t value;
    uint8_t kind;
} lease_event_t;

typedef struct {
    uint8_t levels;
    uint8_t candidate[4];
    uint8_t run[4];
    uint8_t alarms;
    bool heartbeat_seen;
    bool uncertain;
    bool heartbeat_armed;
    uint32_t heartbeat_age;
    uint32_t last_sample;
    uint32_t last_heartbeat;
    uint32_t heartbeats;
    uint32_t gaps;
    uint32_t missed;
    uint32_t dropped;
    uint32_t tx_failures;
    uint32_t sequence;
    uint32_t violations[LEASE_ALARM_COUNT];
    uint32_t max_period;
    uint32_t min_period;
    uint32_t last_period;
    uint32_t max_enabled_stale;
    uint32_t stale_duration;
    bool stale_active;
    lease_event_t queue[LEASE_QUEUE];
    size_t head;
    size_t count;
} lease_t;

void lease_init(lease_t *s, uint32_t now, uint8_t levels);
void lease_sample(lease_t *s, uint32_t now, uint8_t levels);
bool lease_pop(lease_t *s, lease_event_t *event);
void lease_tx_failure(lease_t *s);
uint32_t lease_age(const lease_t *s, uint32_t now);
uint32_t lease_increment(uint32_t value);
#endif
