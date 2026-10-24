/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "lease.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void step(lease_t *s, uint32_t *t, uint8_t v, unsigned n)
{
    while (n--) lease_sample(s, ++*t, v);
}
int main(void)
{
    lease_t s;
    uint32_t t = 0;
    lease_init(&s, t, 12);
    step(&s, &t, 14, 5);
    assert(s.alarms & LEASE_NO_HEARTBEAT);
    assert(s.violations[0] == 1);
    step(&s, &t, 15, 5);
    assert(s.heartbeat_seen);
    assert(s.alarms == 0);
    step(&s, &t, 15, LEASE_TIMEOUT);
    assert(s.alarms & LEASE_EXPIRED);
    step(&s, &t, 11, 5);
    assert(s.alarms & LEASE_RESET_ENABLED);
    assert(s.alarms & LEASE_NO_HEARTBEAT);
    step(&s, &t, 7, 5);
    assert(s.alarms & LEASE_NOT_READY);
    /* A missed sample destroys continuity, not a fictitious valid heartbeat. */
    lease_sample(&s, t += 10, 15);
    assert(s.gaps == 1 && !s.heartbeat_seen);
    assert(s.alarms & LEASE_UNCERTAIN);
    step(&s, &t, 15, 10);
    assert(!s.heartbeat_seen);
    step(&s, &t, 14, 5);
    step(&s, &t, 15, 5);
    assert(s.heartbeat_seen && s.alarms == 0);
    /* Qualification rejects a one-sample pulse. */
    uint32_t edges = s.heartbeats;
    step(&s, &t, 14, 1);
    step(&s, &t, 15, 5);
    assert(s.heartbeats == edges);
    /* Same timestamp must not advance qualification. */
    for (int i = 0; i < 10; ++i) lease_sample(&s, t, 14);
    assert(s.levels == 15);
    lease_init(&s, UINT32_MAX - 2, 12);
    t = UINT32_MAX - 2;
    step(&s, &t, 13, 5);
    assert(s.heartbeat_seen && s.gaps == 0);
    /* Queue overflow is explicit, and status counters remain authoritative. */
    for (int i = 0; i < 100; ++i) step(&s, &t, (i & 1) ? 15 : 13, 5);
    assert(s.dropped > 0);
    lease_event_t e;
    unsigned count = 0;
    while (lease_pop(&s, &e)) ++count;
    assert(count == LEASE_QUEUE);
    char line[WIRE_CAPACITY];
    size_t n = wire_status(line, sizeof line, &s, t);
    assert(n > 0 && line[n - 1] == '\n');
    assert(strncmp(line, "LW1,S,", 6) == 0);
    assert(wire_status(line, 3, &s, t) == 0);
    /* Gap recovery must not accept a one-sample ENABLE-low glitch. */
    lease_init(&s, 0, 14);
    lease_sample(&s, 10, 12);
    assert(s.levels & LEASE_ENABLE);
    assert(s.alarms & LEASE_UNCERTAIN);
    assert(s.alarms & LEASE_NO_HEARTBEAT);
    /* A heartbeat age near UINT32_MAX must saturate across normal samples. */
    lease_init(&s, 0, 14);
    t = 0;
    step(&s, &t, 15, 5);
    s.heartbeat_age = UINT32_MAX - 1;
    step(&s, &t, 15, 5);
    assert(s.alarms & LEASE_EXPIRED);
    assert(lease_age(&s, t) == UINT32_MAX);
    puts("lease engine and encoder tests passed");
}
