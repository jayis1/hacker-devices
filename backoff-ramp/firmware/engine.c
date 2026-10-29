/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* All sample accounting is transactional. A failing increment must not leave
 * half an operation in a telemetry record. Internal overflow is not exported. */
static void add(Ledger *s, uint32_t *counter)
{
    if (*counter == UINT32_MAX)
        s->exhausted = true;
    else
        ++*counter;
}
static uint32_t delay_for(uint32_t grade)
{
    return grade ? BR_BASE << (grade - 1u) : 0u;
}
static void lose(Ledger *s)
{
    if (s->state == BR_PENDING)
        add(s, &s->abandoned);
    s->state = BR_UNKNOWN;
    s->grade = 0;
    s->remaining = 0;
    s->age = 0;
    s->baseline = false;
}
static void forget(Ledger *s)
{
    lose(s);
    s->valid = 0;
    s->tracked = false;
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_init(Ledger *s)
{
    memset(s, 0, sizeof *s);
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    /* Conservation guarantees room to abandon a previously accepted attempt. */
    forget(s);
    s->faults |= reason;
    s->halted = 1;
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
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
        } else if (s->runs[i] < BR_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == BR_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~(uint32_t)mask;
        }
    }
}
static void malformed(Ledger *s, bool incoming_attempt)
{
    /* Count a new attempt even when it collides with a previous pending one.
     * It has no accepted result, so account it separately as abandoned. */
    if (incoming_attempt)
        add(s, &s->abandoned);
    add(s, &s->malformed);
    s->faults |= BR_MALFORMED;
    lose(s);
}
static void observe(Ledger *s)
{
    if (s->valid != 15u)
        return;
    uint8_t value = (uint8_t)s->levels;
    bool first = !s->tracked;
    uint8_t rises = s->tracked ? value & (uint8_t)~s->previous : 0;
    bool reset = (value & 8u) != 0;
    bool attempt = (rises & 1u) != 0;
    bool failure = (rises & 2u) != 0;
    bool success = (rises & 4u) != 0;
    s->previous = value;
    s->tracked = true;
    /* Reset dominates. Diagnostic events during reset are out of coverage. */
    if (reset) {
        if (first || (rises & 8u))
            add(s, &s->resets);
        lose(s);
        s->baseline = (value & 7u) == 0;
        return;
    }
    if (s->baseline) {
        s->baseline = false;
        if (!(value & 7u))
            s->state = BR_READY;
        return;
    }
    if (s->state == BR_UNKNOWN)
        return;
    add(s, &s->coverage);
    if (s->remaining)
        --s->remaining;
    if (s->state == BR_PENDING)
        ++s->age; /* bounded by the deadline below, never unbounded uptime */
    if (attempt)
        add(s, &s->attempts);
    /* Deadline precedence: an operation observed after the allowed response
     * interval cannot rescue correlation, even if its result arrives now. */
    if (s->state == BR_PENDING && s->age > BR_DEADLINE) {
        if (attempt)
            add(s, &s->abandoned);
        add(s, &s->timeouts);
        s->faults |= BR_TIMEOUT;
        lose(s);
        return;
    }
    unsigned events = (unsigned)attempt + (unsigned)failure + (unsigned)success;
    bool overlap = events && (value & 7u) != (rises & 7u);
    if (events > 1u || overlap || (attempt && s->state == BR_PENDING) ||
        ((failure || success) && s->state != BR_PENDING)) {
        malformed(s, attempt);
        return;
    }
    if (attempt) {
        if (s->remaining) {
            add(s, &s->early);
            s->faults |= BR_EARLY;
        }
        /* An early attempt remains correlated; otherwise its failure could
         * accidentally erase the very escalation that is under examination. */
        s->state = BR_PENDING;
        s->age = 0;
        return;
    }
    if (failure) {
        add(s, &s->failures);
        if (s->grade < BR_CAP)
            ++s->grade;
        s->remaining = delay_for(s->grade);
        s->state = BR_READY;
        s->age = 0;
    } else if (success) {
        add(s, &s->successes);
        s->grade = 0;
        s->remaining = 0;
        s->state = BR_READY;
        s->age = 0;
    }
}
static void sample_tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    if (s->started && delta != 1u) {
        add(s, &s->gaps);
        if (UINT32_MAX - s->missed < delta - 1u)
            s->exhausted = true;
        else
            s->missed += delta - 1u;
        s->faults |= BR_GAP;
        forget(s);
        s->started = false;
    }
    s->now = now;
    qualify(s, raw);
    s->started = true;
    observe(s);
}
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw)
{
    if (s->started && now == s->now)
        return;
    if (s->halted) {
        s->now = now;
        return;
    }
    Ledger next = *s;
    sample_tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        ledger_halt(s, BR_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Reject impossible snapshots before encoding. These bounds do not establish
 * authenticity, accurate electrical sampling, or truth of target indications. */
bool ledger_valid(const Ledger *s)
{
    if (!s || s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->state > BR_PENDING || s->grade > BR_CAP || s->halted > 1u)
        return false;
    if (!!s->halted != !!(s->faults & (BR_EXHAUSTED | BR_WATCHDOG)))
        return false;
    if (s->halted && (s->valid || s->state))
        return false;
    if (s->state && (s->valid != 15u || (s->levels & 8u) || !s->resets))
        return false;
    if (!s->state && (s->grade || s->remaining || s->age))
        return false;
    if (s->remaining > delay_for(s->grade))
        return false;
    if (s->grade > s->failures)
        return false;
    if (s->state != BR_PENDING && s->age)
        return false;
    if (s->age > BR_DEADLINE)
        return false;
    uint64_t accounted = (uint64_t)s->failures + s->successes + s->abandoned;
    if (accounted + (s->state == BR_PENDING) != s->attempts)
        return false;
    if (s->attempts > s->coverage || s->early > s->attempts)
        return false;
    if (s->timeouts > s->abandoned || s->malformed > s->coverage)
        return false;
    if (s->early && !s->failures)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->gaps != !!(s->faults & BR_GAP))
        return false;
    if (!!s->early != !!(s->faults & BR_EARLY))
        return false;
    if (!!s->malformed != !!(s->faults & BR_MALFORMED))
        return false;
    if (!!s->timeouts != !!(s->faults & BR_TIMEOUT))
        return false;
    return true;
}
