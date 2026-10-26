/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "isolation.h"
#include <limits.h>
#include <string.h>

/* No inference is made about sub-sample transitions. Each counter records
 * an observation within a continuous acquisition segment. */
void isolation_init(Isolation *s)
{
    memset(s, 0, sizeof *s);
    s->minimum = UINT32_MAX;
}

/* Validate an entire snapshot before exposing it on the wire. This also
 * catches internal state corruption rather than emitting plausible evidence. */
bool isolation_valid(const Isolation *s)
{
    if (!s || s->state > IW_HALTED || s->levels > 15 ||
        s->last_owner > 2 || s->faults > 511)
        return false;
    uint64_t total = 0;
    for (unsigned i = 0; i < 4; ++i)
        total += s->histogram[i];
    if (total != s->switches)
        return false;
    if ((uint64_t)s->histogram[0] + s->histogram[1] != s->early)
        return false;
    if (!s->switches) {
        if (s->minimum != UINT32_MAX)
            return false;
    } else {
        unsigned first = 0;
        while (first < 4 && !s->histogram[first])
            ++first;
        if (s->minimum == UINT32_MAX)
            return false;
        if (first == 0 && s->minimum != 0)
            return false;
        if (first == 1 && (s->minimum < 1 || s->minimum >= IW_DEAD_MS))
            return false;
        if (first == 2 && (s->minimum < IW_DEAD_MS || s->minimum >= 100))
            return false;
        if (first == 3 && s->minimum < 100)
            return false;
    }
    if (s->overlap_ms && !s->overlaps)
        return false;
    if (s->early && !(s->faults & IW_EARLY))
        return false;
    if (s->overlaps && !(s->faults & IW_OVERLAP))
        return false;
    if (s->unrequested && !(s->faults & IW_UNREQUESTED))
        return false;
    if (s->gaps && !(s->faults & IW_GAP))
        return false;
    if (s->tx_fail && !(s->faults & IW_TX))
        return false;
    if (s->state == IW_HALTED) {
        if (!(s->faults & (IW_WATCHDOG | IW_EXHAUSTED)) || s->quiet || s->last_owner)
            return false;
    } else if (s->faults & (IW_WATCHDOG | IW_EXHAUSTED)) {
        return false;
    }
    if (s->state == IW_BASELINE && (s->last_owner || s->quiet >= IW_DEAD_MS))
        return false;
    if (s->state == IW_READY && (s->levels >> 2) == 3)
        return false;
    for (unsigned i = 0; i < 2; ++i) {
        if (s->starved[i] && !(s->faults & (IW_STARVED_A << i)))
            return false;
        if (s->bypass[i] > s->services[1u-i])
            return false;
        if (s->state != IW_HALTED) {
            bool waiting = (s->levels & (1u << i)) && !(s->levels & (4u << i));
            if (!waiting && (s->wait[i] || s->bypass[i] || s->reported[i]))
                return false;
            if ((s->wait[i] >= IW_WAIT_MS || s->bypass[i] >= IW_BYPASSES) && !s->reported[i])
                return false;
        }
    }
    return true;
}

void isolation_halt(Isolation *s, uint32_t reason)
{
    s->faults |= reason;
    s->state = IW_HALTED;
    s->quiet = 0;
    s->last_owner = 0;
}

static bool increment(Isolation *s, uint32_t *value)
{
    if (*value == UINT32_MAX) {
        isolation_halt(s, IW_EXHAUSTED);
        return false;
    }
    ++*value;
    return true;
}

void isolation_tx_failure(Isolation *s)
{
    s->faults |= IW_TX;
    (void)increment(s, &s->tx_fail);
}

static unsigned owner(unsigned levels)
{
    return (levels >> 2) & 3u;
}

static unsigned unauthorized(unsigned levels)
{
    return owner(levels) & ~(levels & 3u);
}

static void reset_wait(Isolation *s)
{
    for (unsigned i = 0; i < 2; ++i) {
        s->wait[i] = 0;
        s->bypass[i] = 0;
        s->reported[i] = false;
    }
}

/* Waiting is independent of the secondary ownership-baseline state.
 * A held peer enable contributes elapsed waiting but only one service edge.
 * A simultaneous new request and peer grant cannot prove a prior bypass. */
static void fairness(Isolation *s, unsigned old, unsigned next)
{
    unsigned granted = owner(next) & ~owner(old) & next;
    for (unsigned i = 0; i < 2; ++i) {
        if ((granted & (1u << i)) && !increment(s, &s->services[i]))
            return;
    }
    for (unsigned i = 0; i < 2; ++i) {
        unsigned bit = 1u << i;
        bool waiting = (next & bit) && !(owner(next) & bit);
        bool was_waiting = (old & bit) && !(owner(old) & bit);
        if (!waiting) {
            s->wait[i] = 0;
            s->bypass[i] = 0;
            s->reported[i] = false;
            continue;
        }
        if (!was_waiting) {
            s->wait[i] = 0;
            s->bypass[i] = 0;
            s->reported[i] = false;
            continue;
        }
        if (s->wait[i] < UINT32_MAX - 1u)
            ++s->wait[i];
        if ((granted & (1u << (1u-i))) && !increment(s, &s->bypass[i]))
            return;
        if (!s->reported[i] && (s->wait[i] >= IW_WAIT_MS || s->bypass[i] >= IW_BYPASSES)) {
            s->faults |= IW_STARVED_A << i;
            if (!increment(s, &s->starved[i]))
                return;
            s->reported[i] = true;
        }
    }
}

static void observe_requests(Isolation *s, unsigned old, unsigned next)
{
    unsigned rising = unauthorized(next) & ~unauthorized(old);
    for (unsigned bit = 0; bit < 2; ++bit) {
        if (!(rising & (1u << bit)))
            continue;
        s->faults |= IW_UNREQUESTED;
        if (!increment(s, &s->unrequested))
            return;
    }
}

static void observe_overlap(Isolation *s, unsigned old, unsigned next)
{
    if (next != 3u)
        return;
    s->faults |= IW_OVERLAP;
    if (old != 3u) {
        (void)increment(s, &s->overlaps);
        return;
    }
    /* Only intervals bracketed by two overlap samples are accumulated. */
    (void)increment(s, &s->overlap_ms);
}

static unsigned bucket(uint32_t duration)
{
    if (duration == 0)
        return 0;
    if (duration < IW_DEAD_MS)
        return 1;
    if (duration < 100u)
        return 2;
    return 3;
}

static void record_switch(Isolation *s, uint32_t dead)
{
    /* Reserve all affected counters before changing any aggregate. */
    unsigned bin = bucket(dead);
    if (s->switches == UINT32_MAX || s->histogram[bin] == UINT32_MAX ||
        (dead < IW_DEAD_MS && s->early == UINT32_MAX)) {
        isolation_halt(s, IW_EXHAUSTED);
        return;
    }
    ++s->switches;
    ++s->histogram[bin];
    if (dead < s->minimum)
        s->minimum = dead;
    if (dead < IW_DEAD_MS) {
        ++s->early;
        s->faults |= IW_EARLY;
    }
}

static void baseline(Isolation *s, unsigned old, unsigned next)
{
    s->last_owner = 0;
    if (next != 0) {
        s->quiet = 0;
        return;
    }
    if (old != 0) {
        s->quiet = 0;
        return;
    }
    if (s->quiet < IW_DEAD_MS)
        ++s->quiet;
    if (s->quiet >= IW_DEAD_MS)
        s->state = IW_READY;
}

static void ownership(Isolation *s, unsigned old, unsigned next)
{
    if (next == 3u) {
        /* An overlap destroys a unique predecessor. Reacquire baseline. */
        s->state = IW_BASELINE;
        s->last_owner = 0;
        s->quiet = 0;
        return;
    }
    if (next == 0) {
        if (old != 0) {
            s->quiet = 0;
        } else if (s->quiet < UINT32_MAX - 1u) {
            ++s->quiet;
        }
        return;
    }
    if (next == old)
        return;
    /* A simultaneous A-to-B transition demonstrates zero dead samples. */
    uint32_t dead = old == 0 ? s->quiet : 0;
    if (s->last_owner != 0 && s->last_owner != next)
        record_switch(s, dead);
    if (s->state == IW_HALTED)
        return;
    s->last_owner = next;
    s->quiet = 0;
}

static void discontinuity(Isolation *s, unsigned levels)
{
    reset_wait(s);
    s->faults |= IW_GAP;
    if (!increment(s, &s->gaps))
        return;
    s->state = IW_BASELINE;
    s->quiet = 0;
    s->last_owner = 0;
    /* Count an observed unsafe level at segment entry, but never duration
     * across the missing interval. Segments can recount one physical fault. */
    observe_requests(s, 0, levels);
    if (s->state == IW_HALTED)
        return;
    observe_overlap(s, 0, owner(levels));
}

void isolation_sample(Isolation *s, uint32_t now, uint8_t levels)
{
    levels &= 15u;
    if (!s->initialized) {
        s->initialized = true;
        s->now = now;
        s->levels = levels;
        if (s->state == IW_HALTED)
            return;
        observe_requests(s, 0, levels);
        if (s->state != IW_HALTED)
            observe_overlap(s, 0, owner(levels));
        return;
    }
    uint32_t elapsed = now - s->now;
    if (!elapsed)
        return;
    unsigned old = s->levels;
    s->now = now;
    s->levels = levels;
    if (s->state == IW_HALTED)
        return;
    if (elapsed != 1u) {
        discontinuity(s, levels);
        return;
    }
    fairness(s, old, levels);
    if (s->state == IW_HALTED)
        return;
    observe_requests(s, old, levels);
    if (s->state == IW_HALTED)
        return;
    observe_overlap(s, owner(old), owner(levels));
    if (s->state == IW_HALTED)
        return;
    if (s->state == IW_BASELINE)
        baseline(s, owner(old), owner(levels));
    else
        ownership(s, owner(old), owner(levels));
}
