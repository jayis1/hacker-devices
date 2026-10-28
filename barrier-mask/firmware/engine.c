/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <string.h>
#include <limits.h>
/* Transactional counters prevent a fault from publishing half a round. */
static void add(Barrier *s, uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount) {
        s->exhausted = true;
        return;
    }
    *value += amount;
}
static unsigned population(uint32_t mask)
{
    unsigned result = 0;
    for (unsigned i = 0; i < BM_CAPACITY; ++i) {
        if (mask & (1u << i))
            ++result;
    }
    return result;
}
static void abandon(Barrier *s)
{
    if (s->active)
        add(s, &s->abandoned, 1);
    add(s, &s->discarded, population(s->mask));
    s->active = 0;
    s->mask = 0;
    s->ready = 0;
    s->lost = true;
}
static void finding(Barrier *s, uint32_t reason, uint32_t *counter)
{
    s->faults |= reason;
    add(s, counter, 1);
    abandon(s);
}
void barrier_init(Barrier *s)
{
    memset(s, 0, sizeof *s);
}
void barrier_halt(Barrier *s, uint32_t reason)
{
    if (s->halted)
        return;
    abandon(s);
    s->halted = 1;
    s->faults |= reason;
}
void barrier_tx_failure(Barrier *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Barrier *s, uint8_t raw)
{
    /* Each channel qualifies independently, so unrelated chatter cannot mask it. */
    for (unsigned i = 0; i < 4; ++i) {
        unsigned mask = 1u << i;
        bool high = (raw & mask) != 0;
        bool candidate = (s->candidates & mask) != 0;
        if (!s->started || high != candidate) {
            if (high)
                s->candidates |= mask;
            else
                s->candidates &= ~mask;
            s->runs[i] = 0;
        } else if (s->runs[i] < BM_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == BM_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~mask;
        }
    }
}
static void gap(Barrier *s, uint32_t delta)
{
    finding(s, BM_GAP, &s->gaps);
    add(s, &s->missed, delta - 1);
    s->valid = 0;
    s->setup = 0;
    s->baseline = 0;
    s->started = false;
    memset(s->runs, 0, sizeof s->runs);
}
static void arrival(Barrier *s, unsigned tag)
{
    unsigned bit = 1u << tag;
    if (s->mask & bit) {
        finding(s, BM_DUPLICATE, &s->duplicate);
        return;
    }
    if (!s->active) {
        add(s, &s->rounds, 1);
        s->active = 1;
        s->age = 0;
    }
    s->mask |= bit;
    add(s, &s->accepted, 1);
}
static void release(Barrier *s)
{
    /* All members must precede release, never arrive on its own tick. */
    if (s->mask != 15 || !s->active) {
        finding(s, BM_EARLY, &s->early);
        return;
    }
    add(s, &s->completed, 1);
    s->active = 0;
    s->mask = 0;
    s->ready = 0;
    s->baseline = 0;
    /* A fresh quiet baseline is required between completed rounds. */
}
static void operations(Barrier *s, uint8_t raw, uint32_t previous)
{
    unsigned rising = s->levels & ~previous;
    bool arrive = (rising & 4u) != 0;
    bool releasing = (rising & 8u) != 0;
    if (arrive)
        add(s, &s->arrivals, 1);
    if (releasing)
        add(s, &s->releases, 1);
    /* Expiry takes precedence even when the fault tick contains an edge. */
    if (s->active && s->age > BM_DEADLINE) {
        finding(s, BM_TIMEOUT, &s->timeouts);
        return;
    }
    if (!arrive && !releasing)
        return;
    if ((arrive && ((s->levels | raw) & 8u)) ||
        (releasing && ((s->levels | raw) & 4u))) {
        finding(s, BM_COLLISION, &s->collision);
        return;
    }
    if (arrive) {
        unsigned tag = s->levels & 3u;
        if (s->setup < BM_SETUP || (raw & 3u) != tag) {
            finding(s, BM_UNSTABLE, &s->unstable);
            return;
        }
        arrival(s, tag);
    } else {
        release(s);
    }
}
static void sample_tick(Barrier *s, uint32_t now, uint8_t raw)
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
        if (s->setup < BM_SETUP)
            ++s->setup;
    } else {
        s->setup = 0;
    }
    if (!s->ready) {
        /* Quiet after a fault cannot reconstruct lost round identity. */
        if (!s->lost && s->valid == 15 && !(raw & 12u) && !(s->levels & 12u)) {
            if (s->baseline < BM_BASELINE)
                ++s->baseline;
            if (s->baseline == BM_BASELINE)
                s->ready = 1;
        } else {
            s->baseline = 0;
        }
        return;
    }
    if (s->active)
        ++s->age; /* Always bounded by deadline + one before abandonment. */
    operations(s, raw, previous);
}
void barrier_sample(Barrier *s, uint32_t now, uint8_t raw)
{
    if (s->started && now == s->now)
        return;
    if (s->halted) {
        s->now = now;
        return;
    }
    Barrier next = *s;
    sample_tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        barrier_halt(s, BM_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool state_valid(const Barrier *s)
{
    if (s->levels > 15 || s->valid > 15 || s->mask > 15)
        return false;
    if (s->ready > 1 || s->halted > 1 || s->active > 1 || s->faults > 255)
        return false;
    if (s->age > BM_DEADLINE + 1)
        return false;
    if (!!s->mask != !!s->active)
        return false;
    if (s->active && (!s->ready || s->age > BM_DEADLINE))
        return false;
    if (s->ready && (s->valid != 15 || s->halted || s->faults || s->lost))
        return false;
    return true;
}
static bool ledger_valid(const Barrier *s)
{
    if ((uint64_t)s->completed + s->abandoned + s->active != s->rounds)
        return false;
    if (4ull * s->completed + s->discarded + population(s->mask) != s->accepted)
        return false;
    if (s->accepted > s->arrivals || s->completed > s->releases)
        return false;
    if (s->discarded > 4ull * s->abandoned || s->discarded < s->abandoned)
        return false;
    if (s->duplicate && s->arrivals == s->accepted)
        return false;
    if (s->early && s->releases == s->completed)
        return false;
    if (s->timeouts > s->abandoned || s->duplicate > s->abandoned)
        return false;
    if ((uint64_t)s->duplicate + s->early + s->collision + s->unstable + s->timeouts > 1)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
static bool findings_valid(const Barrier *s)
{
    if (!!s->halted != !!(s->faults & (BM_EXHAUSTED | BM_WATCHDOG)))
        return false;
    if (!!s->duplicate != !!(s->faults & BM_DUPLICATE))
        return false;
    if (!!s->early != !!(s->faults & BM_EARLY))
        return false;
    if (!!s->timeouts != !!(s->faults & BM_TIMEOUT))
        return false;
    if (!!s->collision != !!(s->faults & BM_COLLISION))
        return false;
    if (!!s->unstable != !!(s->faults & BM_UNSTABLE))
        return false;
    if (!!s->gaps != !!(s->faults & BM_GAP))
        return false;
    return true;
}
bool barrier_valid(const Barrier *s)
{
    return s && state_valid(s) && ledger_valid(s) && findings_valid(s);
}
