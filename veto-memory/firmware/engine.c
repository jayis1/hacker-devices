/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Speculative updates ensure that a saturated counter cannot publish a
 * partially classified commit. Halt abandons only the prior committed round. */
static void add(Ledger *s, uint32_t *value)
{
    if (*value == UINT32_MAX)
        s->exhausted = true;
    else
        ++*value;
}
void ledger_init(Ledger *s)
{
    memset(s, 0, sizeof *s);
}
static void clear_round(Ledger *s)
{
    s->veto = 0;
    s->seen = 0;
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    /* Conservation guarantees room: abandoned + active <= rounds. */
    if (s->phase == VM_ACTIVE)
        ++s->abandoned;
    s->halted = 1;
    s->faults |= reason;
    s->phase = VM_WAIT;
    s->valid = 0;
    clear_round(s);
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
/* Input qualification is independent. Candidate chatter on ABORT must not
 * suppress recognition of a sustained COMMIT on a different channel. */
static void qualify(Ledger *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        if (!s->started || (raw & mask) != (s->candidates & mask)) {
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
            s->runs[i] = 0;
        } else if (s->runs[i] < VM_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == VM_QUALIFY) {
            s->valid |= mask;
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
        }
    }
}
static void latch_veto(Ledger *s)
{
    if (!s->veto) {
        s->veto = 1;
        add(s, &s->veto_rounds);
    }
}
static void baseline(Ledger *s)
{
    s->phase = VM_READY;
    clear_round(s);
    add(s, &s->baselines);
}
static void begin_round(Ledger *s)
{
    s->phase = VM_ACTIVE;
    clear_round(s);
    add(s, &s->rounds);
}
/* A previously qualified preparation dropping is an irrevocable veto for
 * this round, even if it returns before COMMIT. Abort is also retained.
 * Merely seeing the second prepare late is not a withdrawal. */
static void update_veto(Ledger *s, uint32_t previous, uint8_t raw, bool commit)
{
    uint32_t withdrawn = previous & ~s->levels & s->seen & 3u;
    for (unsigned side = 0; side < 2; ++side) {
        if (withdrawn & (1u << side)) {
            add(s, &s->withdrawals);
            latch_veto(s);
        }
    }
    if ((s->levels & 8u) || (commit && (raw & 8u)))
        latch_veto(s);
    s->seen |= s->levels & 3u;
}
static void decide(Ledger *s, uint32_t previous, uint8_t raw)
{
    add(s, &s->commits);
    /* Veto has precedence over missing preparation. This is classification
     * of a sampled contract, not proof of an actual durable commit. */
    if (s->veto) {
        add(s, &s->vetoed);
        s->faults |= VM_VETOED;
    } else if ((previous & s->levels & raw & 3u) != 3u) {
        add(s, &s->early);
        s->faults |= VM_EARLY;
    } else {
        add(s, &s->accepted);
    }
    s->phase = VM_DRAIN;
}
static void policy(Ledger *s, uint32_t previous, uint8_t raw)
{
    if (s->valid != 15u)
        return;
    add(s, &s->observed);
    bool empty = !s->levels && !raw;
    bool commit = (s->levels & ~previous & 4u) != 0;
    if (s->phase == VM_WAIT) {
        /* Mid-round attachment is outside coverage until all four wires
         * are observed low. The owner must ensure this means idle. */
        if (empty)
            baseline(s);
        return;
    }
    if (s->phase == VM_DRAIN) {
        if (empty) {
            baseline(s);
        } else if (commit) {
            add(s, &s->commits);
            add(s, &s->repeated);
            s->faults |= VM_REPEAT;
        }
        return;
    }
    if (s->phase == VM_READY) {
        if (!s->levels)
            return;
        begin_round(s);
    }
    /* Empty means cancellation, not a withdrawal finding. Its boundary
     * explicitly ends the round and permits a new decision next round. */
    if (empty) {
        add(s, &s->cancelled);
        baseline(s);
        return;
    }
    update_veto(s, previous, raw, commit);
    if (commit)
        decide(s, previous, raw);
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u) {
        add(s, &s->gaps);
        if (UINT32_MAX - s->missed < delta - 1u)
            s->exhausted = true;
        else
            s->missed += delta - 1u;
        /* Silence cannot reconstruct which round owns a pre-gap veto.
         * Require operator restart instead of silently forgetting it. */
        ledger_halt(s, VM_GAP);
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
        ledger_halt(s, VM_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Necessary, independently reproducible invariants used by the encoder.
 * They cannot authenticate input evidence or prove historical realizability. */
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->phase > VM_DRAIN || s->veto > 1u || s->seen > 3u || s->halted > 1u)
        return false;
    if (!!s->halted != !!(s->faults & (VM_GAP | VM_EXHAUSTED | VM_WATCHDOG)))
        return false;
    if (s->halted && (s->valid || s->phase || s->seen || s->veto))
        return false;
    if (s->phase && (s->valid != 15u || !s->baselines))
        return false;
    if (s->phase <= VM_READY && (s->seen || s->veto))
        return false;
    if (s->phase == VM_READY && s->levels)
        return false;
    if (s->phase == VM_ACTIVE && (s->levels & 4u))
        return false;
    if (s->veto && !s->veto_rounds)
        return false;
    uint64_t first = (uint64_t)s->accepted + s->early + s->vetoed;
    uint64_t rounds = first + s->cancelled + s->abandoned + (s->phase == VM_ACTIVE);
    if (rounds != s->rounds || first + s->repeated != s->commits)
        return false;
    if (s->phase == VM_DRAIN && !first)
        return false;
    if (s->repeated && !first)
        return false;
    if (s->vetoed > s->veto_rounds || s->veto_rounds > s->rounds)
        return false;
    if (s->withdrawals && !s->veto_rounds)
        return false;
    if (s->baselines > s->observed || s->rounds > s->observed)
        return false;
    if (s->commits > s->observed || s->gaps > 1u || s->abandoned > 1u)
        return false;
    if ((uint64_t)s->baselines > (uint64_t)s->rounds + 1u)
        return false;
    if ((uint64_t)s->withdrawals > 2ull * s->observed)
        return false;
    if (!!s->early != !!(s->faults & VM_EARLY))
        return false;
    if (!!s->vetoed != !!(s->faults & VM_VETOED))
        return false;
    if (!!s->repeated != !!(s->faults & VM_REPEAT))
        return false;
    if (!!s->gaps != !!(s->faults & VM_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
