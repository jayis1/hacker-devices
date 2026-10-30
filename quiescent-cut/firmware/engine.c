/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

static unsigned population(uint32_t mask)
{
    return (mask & 1u) + ((mask >> 1) & 1u);
}
/* Updates are speculative until the complete sample can be committed. */
static void add(Ledger *s, uint32_t *value, unsigned amount)
{
    if (UINT32_MAX - *value < amount)
        s->exhausted = true;
    else
        *value += amount;
}
void ledger_init(Ledger *s)
{
    memset(s, 0, sizeof *s);
}
static void discard_cohort(Ledger *s)
{
    /* captured conservation guarantees room for committed obligations. */
    s->discarded += population(s->owed);
    s->owed = 0;
    s->cohort = 0;
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    if (s->phase == QC_ACTIVE)
        ++s->abandoned;
    discard_cohort(s);
    s->halted = 1;
    s->faults |= reason;
    s->phase = QC_WAIT;
    s->valid = 0;
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
/* Qualify channels independently: unrelated reader chatter cannot conceal
 * a sustained reclaim indication. A gap halts instead of bridging history. */
static void qualify(Ledger *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        if (!s->started || (raw & mask) != (s->candidates & mask)) {
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
            s->runs[i] = 0;
        } else if (s->runs[i] < QC_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == QC_QUALIFY) {
            s->valid |= mask;
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
        }
    }
}
static void baseline(Ledger *s)
{
    s->phase = QC_READY;
    s->cohort = 0;
    s->owed = 0;
    add(s, &s->baselines, 1);
}
static void retire(Ledger *s, uint32_t previous, uint8_t raw)
{
    s->phase = QC_ACTIVE;
    add(s, &s->rounds, 1);
    /* Union at the cut is deliberately conservative. A tied reader exit
     * remains owed until a later low observation; a raw new reader is also
     * included rather than treating debounce latency as historical absence. */
    s->cohort = (previous | s->levels | raw) & 3u;
    s->owed = s->cohort;
    add(s, &s->captured, population(s->cohort));
}
static void quiesce(Ledger *s, uint8_t raw)
{
    uint32_t cleared = s->owed & ~(s->levels | raw) & 3u;
    add(s, &s->released, population(cleared));
    s->owed &= ~cleared;
    /* A lane which starts another read section does not reacquire an old
     * obligation. Adapter correctness must establish that it cannot obtain
     * the retired object again; this instrument cannot inspect pointers. */
}
static void reclaim(Ledger *s, bool eligible)
{
    add(s, &s->reclaims, 1);
    if (s->phase != QC_ACTIVE) {
        add(s, &s->orphan, 1);
        s->faults |= QC_ORPHAN;
        return;
    }
    if (eligible) {
        add(s, &s->accepted, 1);
    } else {
        add(s, &s->early, 1);
        s->faults |= QC_EARLY;
    }
    /* Even an unsafe reclaim ends this one-object observation. It is not
     * allowed to become accepted later; a fresh retirement is required. */
    discard_cohort(s);
    s->phase = QC_DRAIN;
}
static void policy(Ledger *s, uint32_t previous, uint8_t raw)
{
    if (s->valid != 15u)
        return;
    add(s, &s->observed, 1);
    bool quiet = !((s->levels | raw) & 12u);
    bool cut = (s->levels & ~previous & 4u) != 0;
    bool free_edge = (s->levels & ~previous & 8u) != 0;
    bool eligible = s->phase == QC_ACTIVE && !s->owed;
    if (s->phase == QC_WAIT) {
        if (quiet)
            baseline(s);
        return;
    }
    /* No second object may be introduced until the previous reclaim has
     * drained. Correlation cannot be repaired by guessing which object a
     * later reclaim refers to. Count both observable operations then halt. */
    if (cut && s->phase != QC_READY) {
        add(s, &s->overlap, 1);
        if (free_edge) {
            add(s, &s->reclaims, 1);
            add(s, &s->orphan, 1);
            s->faults |= QC_ORPHAN;
        }
        ledger_halt(s, QC_OVERLAP);
        return;
    }
    if (cut) {
        retire(s, previous, raw);
        eligible = false;
    } else if (s->phase == QC_ACTIVE) {
        quiesce(s, raw);
    }
    /* Use prior eligibility: a last reader leaving on this same sample
     * cannot retroactively establish that reclaim followed the grace period. */
    if (free_edge)
        reclaim(s, eligible);
    if (s->phase == QC_DRAIN && quiet)
        baseline(s);
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u) {
        add(s, &s->gaps, 1);
        add(s, &s->missed, delta - 1u);
        ledger_halt(s, QC_GAP);
        return;
    }
    uint32_t previous = s->levels;
    qualify(s, raw);
    s->started = true;
    policy(s, previous, raw);
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
        ledger_halt(s, QC_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Necessary snapshot checks, not authentication or a full waveform proof. */
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->phase > QC_DRAIN || s->cohort > 3u || s->owed > 3u || s->halted > 1u)
        return false;
    if (s->owed & ~s->cohort)
        return false;
    if (!!s->halted != !!(s->faults & 60u))
        return false;
    if (s->halted && (s->valid || s->phase || s->cohort || s->owed))
        return false;
    if (s->phase && (s->valid != 15u || !s->baselines))
        return false;
    if (s->phase != QC_ACTIVE && (s->cohort || s->owed))
        return false;
    uint64_t closed = (uint64_t)s->accepted + s->early;
    if (closed + s->abandoned + (s->phase == QC_ACTIVE) != s->rounds)
        return false;
    if (closed + s->orphan != s->reclaims)
        return false;
    if ((uint64_t)s->released + s->discarded + population(s->owed) != s->captured)
        return false;
    if (s->captured > 2ull * s->rounds)
        return false;
    if (s->phase == QC_ACTIVE && s->captured < population(s->cohort))
        return false;
    if (s->phase == QC_DRAIN && !closed)
        return false;
    if (s->baselines > s->observed || s->rounds > s->observed || s->reclaims > s->observed)
        return false;
    if (s->gaps > 1u || s->overlap > 1u || s->abandoned > 1u)
        return false;
    if ((uint64_t)s->baselines > closed + 1u)
        return false;
    if (!!s->early != !!(s->faults & QC_EARLY))
        return false;
    if (!!s->orphan != !!(s->faults & QC_ORPHAN))
        return false;
    if (!!s->overlap != !!(s->faults & QC_OVERLAP))
        return false;
    if (!!s->gaps != !!(s->faults & QC_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
