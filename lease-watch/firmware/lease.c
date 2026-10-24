/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "lease.h"
#include <string.h>

uint32_t lease_increment(uint32_t value)
{
    return value == UINT32_MAX ? value : value + 1u;
}

static uint32_t add_saturated(uint32_t value, uint32_t amount)
{
    if (UINT32_MAX - value < amount)
        return UINT32_MAX;
    return value + amount;
}

static void event_push(lease_t *s, uint32_t now, uint8_t kind, uint32_t value)
{
    /* Sequence wraps by design; dropped saturates and never hides loss. */
    uint32_t sequence = s->sequence++;
    if (s->count == LEASE_QUEUE) {
        s->dropped = lease_increment(s->dropped);
        return;
    }
    size_t index = (s->head + s->count) % LEASE_QUEUE;
    s->queue[index].sequence = sequence;
    s->queue[index].time = now;
    s->queue[index].kind = kind;
    s->queue[index].value = value;
    ++s->count;
}

bool lease_pop(lease_t *s, lease_event_t *event)
{
    if (!s || !event || !s->count)
        return false;
    *event = s->queue[s->head];
    s->head = (s->head + 1u) % LEASE_QUEUE;
    --s->count;
    return true;
}

void lease_tx_failure(lease_t *s)
{
    s->tx_failures = lease_increment(s->tx_failures);
}

uint32_t lease_age(const lease_t *s, uint32_t now)
{
    (void)now; /* Ages accumulate at sampling and saturate rather than wrap. */
    if (!s->heartbeat_seen)
        return UINT32_MAX;
    return s->heartbeat_age;
}

static void heartbeat(lease_t *s, uint32_t now)
{
    if (s->heartbeat_seen) {
        uint32_t period = s->heartbeat_age;
        s->last_period = period;
        if (period < s->min_period)
            s->min_period = period;
        if (period > s->max_period)
            s->max_period = period;
    }
    s->last_heartbeat = now;
    s->heartbeat_age = 0;
    s->heartbeat_seen = true;
    s->uncertain = false;
    s->heartbeats = lease_increment(s->heartbeats);
    event_push(s, now, EVENT_HEARTBEAT, s->heartbeats);
}

static uint8_t alarm_mask(const lease_t *s, uint32_t now)
{
    uint8_t mask = s->uncertain ? LEASE_UNCERTAIN : 0;
    /* This instrument judges enabled operation, not intentional idle time. */
    if (!(s->levels & LEASE_ENABLE))
        return mask;
    if (!s->heartbeat_seen)
        mask |= LEASE_NO_HEARTBEAT;
    else if (lease_age(s, now) >= LEASE_TIMEOUT)
        mask |= LEASE_EXPIRED;
    if (!(s->levels & LEASE_RESET_N))
        mask |= LEASE_RESET_ENABLED;
    if (!(s->levels & LEASE_READY))
        mask |= LEASE_NOT_READY;
    if (s->uncertain)
        mask |= LEASE_UNCERTAIN;
    return mask;
}

static void update_alarms(lease_t *s, uint32_t now)
{
    uint8_t mask = alarm_mask(s, now);
    uint8_t entering = mask & (uint8_t)~s->alarms;
    for (unsigned i = 0; i < LEASE_ALARM_COUNT; ++i) {
        if (entering & (1u << i))
            s->violations[i] = lease_increment(s->violations[i]);
    }
    bool stale = (mask & (LEASE_EXPIRED | LEASE_NO_HEARTBEAT)) != 0;
    if (stale && !s->stale_active) {
        s->stale_duration = 0;
        s->stale_active = true;
    }
    if (s->stale_active) {
        uint32_t elapsed = s->stale_duration;
        if (elapsed > s->max_enabled_stale)
            s->max_enabled_stale = elapsed;
        if (!stale)
            s->stale_active = false;
    }
    if (mask != s->alarms) {
        s->alarms = mask;
        event_push(s, now, EVENT_ALARM, mask);
    }
}

void lease_init(lease_t *s, uint32_t now, uint8_t levels)
{
    memset(s, 0, sizeof *s);
    s->levels = levels & 15u;
    s->last_sample = now;
    s->min_period = UINT32_MAX;
    s->heartbeat_armed = !(levels & LEASE_HB);
    /* Initial electrical state is observed, but never counted as an edge. */
    for (unsigned i = 0; i < 4; ++i) {
        s->candidate[i] = (levels >> i) & 1u;
        s->run[i] = LEASE_QUALIFY;
    }
    update_alarms(s, now);
}

static void continuity_lost(lease_t *s, uint32_t now, uint32_t elapsed,
                            uint8_t levels)
{
    s->gaps = lease_increment(s->gaps);
    s->missed = add_saturated(s->missed, elapsed - 1u);
    s->heartbeat_seen = false;
    s->uncertain = true;
    /* Preserve qualified levels until three new samples establish a change. */
    s->heartbeat_armed = false;
    for (unsigned i = 0; i < 4; ++i) {
        s->candidate[i] = (levels >> i) & 1u;
        s->run[i] = 0;
    }
    event_push(s, now, EVENT_GAP, elapsed);
}

static uint8_t qualify(lease_t *s, uint8_t raw)
{
    uint8_t next = s->levels;
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (raw >> i) & 1u;
        if (bit != s->candidate[i]) {
            s->candidate[i] = bit;
            s->run[i] = 1;
        } else if (s->run[i] < LEASE_QUALIFY) {
            ++s->run[i];
        }
        if (s->run[i] >= LEASE_QUALIFY) {
            if (bit)
                next |= (uint8_t)(1u << i);
            else
                next &= (uint8_t)~(1u << i);
        }
    }
    return next;
}

void lease_sample(lease_t *s, uint32_t now, uint8_t levels)
{
    uint32_t elapsed = now - s->last_sample;
    if (!elapsed)
        return;
    s->last_sample = now;
    s->heartbeat_age = add_saturated(s->heartbeat_age, elapsed);
    if (s->stale_active)
        s->stale_duration = add_saturated(s->stale_duration, elapsed);
    levels &= 15u;
    if (elapsed != 1u) {
        continuity_lost(s, now, elapsed, levels);
        update_alarms(s, now);
        return;
    }
    uint8_t old = s->levels;
    s->levels = qualify(s, levels);
    if (!(s->levels & LEASE_HB) && s->run[0] >= LEASE_QUALIFY)
        s->heartbeat_armed = true;
    /* Reset dominates a simultaneous heartbeat. A new rise is needed later. */
    if (!(s->levels & LEASE_RESET_N)) {
        s->heartbeat_seen = false;
    } else if (s->heartbeat_armed && !(old & LEASE_HB) && (s->levels & LEASE_HB)) {
        heartbeat(s, now);
        s->heartbeat_armed = false;
    }
    if (old != s->levels)
        event_push(s, now, EVENT_LEVEL, s->levels);
    update_alarms(s, now);
}
