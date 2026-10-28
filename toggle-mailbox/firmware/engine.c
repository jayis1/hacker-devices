/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* Every acquisition update is speculative until all counters fit. */
static void add(Mailbox *s, uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount) {
        s->exhausted = true;
        return;
    }
    *value += amount;
}
static bool equal_phase(uint32_t levels)
{
    return ((levels >> 2) & 1u) == ((levels >> 3) & 1u);
}
static void abandon(Mailbox *s)
{
    if (s->active)
        add(s, &s->abandoned, 1);
    s->active = 0;
    s->age = 0;
    s->ready = 0;
    s->lost = true;
}
static void finding(Mailbox *s, uint32_t flag, uint32_t *counter)
{
    add(s, counter, 1);
    s->faults |= flag;
    abandon(s);
}
void mailbox_init(Mailbox *s)
{
    memset(s, 0, sizeof *s);
}
void mailbox_halt(Mailbox *s, uint32_t reason)
{
    if (s->halted)
        return;
    /* In a valid ledger active implies there is room for its abandonment. */
    abandon(s);
    s->faults |= reason;
    s->halted = 1;
}
void mailbox_tx_failure(Mailbox *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Mailbox *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        bool high = (raw & mask) != 0;
        if (!s->started || high != ((s->candidates & mask) != 0)) {
            if (high)
                s->candidates |= mask;
            else
                s->candidates &= (uint8_t)~mask;
            s->runs[i] = 0;
        } else if (s->runs[i] < TM_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == TM_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~(uint32_t)mask;
        }
    }
}
static void gap(Mailbox *s, uint32_t delta)
{
    finding(s, TM_GAP, &s->gaps);
    add(s, &s->missed, delta - 1u);
    s->valid = 0;
    s->setup = 0;
    s->baseline = 0;
    s->started = false;
    memset(s->runs, 0, sizeof s->runs);
}
static void operations(Mailbox *s, uint8_t raw, uint32_t previous)
{
    uint32_t changed = s->levels ^ previous;
    bool request = (changed & 4u) != 0;
    bool ack = (changed & 8u) != 0;
    /* Pre-account a new request as abandoned until it is admitted below.
       This preserves all observed requests even on a timeout/fault tick. */
    if (request) {
        add(s, &s->starts, 1);
        add(s, &s->abandoned, 1);
    }
    if (ack)
        add(s, &s->acks, 1);
    if (s->active && s->age > TM_DEADLINE) {
        finding(s, TM_TIMEOUT, &s->timeouts);
        return;
    }
    if (request && ack) {
        finding(s, TM_OVERLAP, &s->overlap);
        return;
    }
    if (request && s->active) {
        finding(s, TM_OVERTAKE, &s->overtake);
        return;
    }
    /* Hold includes the acknowledgment endpoint, and uses raw samples so
       a rejected one-tick excursion cannot masquerade as stable data. */
    if (s->active && ((raw & 3u) != s->captured ||
                       (s->levels & 3u) != s->captured)) {
        finding(s, TM_HOLD, &s->hold);
        return;
    }
    if (ack) {
        if (!s->active) {
            finding(s, TM_ORPHAN, &s->orphan);
            return;
        }
        add(s, &s->completed, 1);
        if (s->age > s->max_latency)
            s->max_latency = s->age;
        s->active = 0;
        s->age = 0;
    }
    if (request) {
        if (s->setup < TM_SETUP || (raw & 3u) != (s->levels & 3u)) {
            finding(s, TM_SETUP_FAIL, &s->setup_fail);
            return;
        }
        if (s->exhausted)
            return;
        --s->abandoned;
        s->active = 1;
        s->captured = s->levels & 3u;
        s->age = 0;
    }
}
static void sample_tick(Mailbox *s, uint32_t now, uint8_t raw)
{
    uint32_t previous = s->levels;
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u)
        gap(s, delta);
    qualify(s, raw);
    s->started = true;
    if (s->valid == 15 && (raw & 3u) == (s->levels & 3u) &&
        (previous & 3u) == (s->levels & 3u)) {
        if (s->setup < TM_SETUP)
            ++s->setup;
    } else {
        s->setup = 0;
    }
    if (!s->ready) {
        /* Equal phases are a prospective idle baseline, not proof of history.
           Once correlation is lost, equality cannot identify the old message. */
        if (!s->lost && s->valid == 15 && raw == s->levels && equal_phase(raw)) {
            if (s->baseline < TM_BASELINE)
                ++s->baseline;
            if (s->baseline == TM_BASELINE)
                s->ready = 1;
        } else {
            s->baseline = 0;
        }
        return;
    }
    if (s->active)
        ++s->age;
    operations(s, raw, previous);
}
void mailbox_sample(Mailbox *s, uint32_t now, uint8_t raw)
{
    if (s->started && now == s->now)
        return;
    if (s->halted) {
        s->now = now;
        return;
    }
    Mailbox next = *s;
    sample_tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        mailbox_halt(s, TM_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool state_valid(const Mailbox *s)
{
    if (s->levels > 15 || s->valid > 15 || s->captured > 3)
        return false;
    if (s->ready > 1 || s->halted > 1 || s->active > 1 || s->faults > 511)
        return false;
    if (s->active && (!s->ready || s->age > TM_DEADLINE || equal_phase(s->levels)))
        return false;
    if (s->active && (s->levels & 3u) != s->captured)
        return false;
    if (!s->active && s->age)
        return false;
    if (s->ready && (s->valid != 15 || s->faults || s->halted || s->lost))
        return false;
    if (s->ready && !s->active && !equal_phase(s->levels))
        return false;
    return true;
}
static bool ledger_valid(const Mailbox *s)
{
    if ((uint64_t)s->completed + s->abandoned + s->active != s->starts)
        return false;
    if (s->completed > s->acks || s->max_latency > TM_DEADLINE)
        return false;
    if ((!s->completed && s->max_latency) || (s->completed && !s->max_latency))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (s->setup_fail > s->abandoned || s->hold > s->abandoned ||
        s->timeouts > s->abandoned || 2ull * s->overtake > s->abandoned)
        return false;
    if (s->overlap > s->abandoned || s->orphan > s->acks - s->completed)
        return false;
    if ((uint64_t)s->setup_fail + s->hold + s->overtake + s->orphan +
        s->overlap + s->timeouts > 1)
        return false;
    return true;
}
static bool findings_valid(const Mailbox *s)
{
    if (!!s->halted != !!(s->faults & (TM_EXHAUSTED | TM_WATCHDOG)))
        return false;
    const uint32_t counters[] = {s->setup_fail, s->hold, s->overtake,
        s->orphan, s->overlap, s->timeouts, s->gaps};
    for (unsigned i = 0; i < sizeof counters / sizeof counters[0]; ++i) {
        if (!!counters[i] != !!(s->faults & (1u << i)))
            return false;
    }
    return true;
}
bool mailbox_valid(const Mailbox *s)
{
    return s && state_valid(s) && ledger_valid(s) && findings_valid(s);
}
