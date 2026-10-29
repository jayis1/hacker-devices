/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Every sample is evaluated on a copy. This preserves conservation when any
 * cumulative diagnostic reaches its representable limit. */
static void add(Ledger *s, uint32_t *value)
{
    if (*value == UINT32_MAX)
        s->exhausted = true;
    else
        ++*value;
}
static void end_episode(Ledger *s)
{
    s->mismatch = 0;
    s->streak = 0;
}
static void unknown(Ledger *s)
{
    s->state = 0;
    s->settle = 0;
    end_episode(s);
}
static void forget(Ledger *s)
{
    unknown(s);
    s->valid = 0;
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
    forget(s);
    s->halted = 1;
    s->faults |= reason;
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
/* Qualify the two-bit level as a bundle, independently of output and inhibit.
 * Otherwise independently delayed data bits could synthesize a threshold code
 * that was never observed as a complete raw bus value. */
static void qualify(Ledger *s, uint8_t raw)
{
    static const uint8_t masks[3] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || (raw & mask) != (s->candidates & mask)) {
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
            s->runs[i] = 0;
        } else if (s->runs[i] < TM_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == TM_QUALIFY) {
            s->valid |= mask;
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
        }
    }
}
static void expectation(Ledger *s, uint32_t next)
{
    if (s->state == next)
        return;
    if (next == 2u)
        add(s, &s->rises);
    else
        add(s, &s->falls);
    s->state = next;
    s->settle = TM_SETTLE;
    end_episode(s);
}
static void compare(Ledger *s)
{
    bool output = (s->levels & 4u) != 0;
    bool expected = s->state == 2u;
    add(s, &s->coverage);
    if (output == expected) {
        add(s, &s->good);
        end_episode(s);
        return;
    }
    if (expected) {
        add(s, &s->bad_high);
        s->faults |= TM_MISSING;
    } else {
        add(s, &s->bad_low);
        s->faults |= TM_SPURIOUS;
    }
    if (!s->mismatch)
        add(s, &s->episodes);
    s->mismatch = 1;
    add(s, &s->streak);
    if (s->streak > s->longest)
        s->longest = s->streak;
}
static void observe(Ledger *s)
{
    /* Initial qualification is explicit. Once qualified, a channel retains its
     * last accepted value while a new candidate is being qualified. */
    if (s->valid != 15u)
        return;
    if (s->levels & 8u) {
        unknown(s);
        return;
    }
    uint32_t before = s->state;
    unsigned level = s->levels & 3u;
    if (level == 0u)
        expectation(s, 1u);
    else if (level == 3u)
        expectation(s, 2u);
    /* Mid-band values cannot establish memory after attachment or a gap. */
    if (!s->state)
        return;
    /* Exactly ten subsequent ticks of grace, not eleven and not perpetual
     * refresh while the input remains at an extreme. */
    if (s->state != before)
        return;
    if (s->settle) {
        --s->settle;
        if (s->settle)
            return;
    }
    compare(s);
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    if (s->started && delta != 1u) {
        add(s, &s->gaps);
        if (UINT32_MAX - s->missed < delta - 1u)
            s->exhausted = true;
        else
            s->missed += delta - 1u;
        s->faults |= TM_GAP;
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
    tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        ledger_halt(s, TM_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Necessary consistency bounds also implemented by the companion. They reject
 * internally impossible records, not maliciously forged plausible evidence. */
bool ledger_valid(const Ledger *s)
{
    if (!s || s->levels > 15u || s->valid > 15u || s->faults > 31u)
        return false;
    if (s->state > 2u || s->halted > 1u || s->mismatch > 1u)
        return false;
    if (s->settle > TM_SETTLE)
        return false;
    if (!!s->halted != !!(s->faults & (TM_EXHAUSTED | TM_WATCHDOG)))
        return false;
    if (s->halted && (s->state || s->valid))
        return false;
    if (s->state && (s->valid != 15u || (s->levels & 8u)))
        return false;
    if (!s->state && (s->settle || s->mismatch || s->streak))
        return false;
    if (s->settle && (s->mismatch || s->streak))
        return false;
    if ((s->valid & 3u) != 0u && (s->valid & 3u) != 3u)
        return false;
    if (s->state && !s->settle &&
        !!s->mismatch != ((s->state == 2u) != !!(s->levels & 4u)))
        return false;
    if ((uint64_t)s->good + s->bad_high + s->bad_low != s->coverage)
        return false;
    uint64_t bad = (uint64_t)s->bad_high + s->bad_low;
    if (s->episodes > bad || s->longest > bad)
        return false;
    if (!!bad != !!s->episodes || !!bad != !!s->longest)
        return false;
    if ((uint64_t)s->longest + s->episodes > bad + 1u)
        return false;
    if ((uint64_t)s->episodes * s->longest < bad)
        return false;
    if (s->streak > s->longest || !!s->streak != !!s->mismatch)
        return false;
    if (s->mismatch && ((s->state == 2u) == !!(s->levels & 4u)))
        return false;
    if (s->state == 2u && !s->rises)
        return false;
    if (s->state == 1u && !s->falls)
        return false;
    if (s->state && (s->levels & 3u) == 0u && s->state != 1u)
        return false;
    if (s->state && (s->levels & 3u) == 3u && s->state != 2u)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->gaps != !!(s->faults & TM_GAP))
        return false;
    if (!!s->bad_high != !!(s->faults & TM_MISSING))
        return false;
    if (!!s->bad_low != !!(s->faults & TM_SPURIOUS))
        return false;
    return true;
}
