/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <string.h>
#include <limits.h>

/* Evidence updates are transactional. Exhaustion rolls back the entire tick. */
static void add(Stack *s, uint32_t *counter, uint32_t amount)
{
    if (UINT32_MAX - *counter < amount) {
        s->exhausted = true;
        return;
    }
    *counter += amount;
}
static void finding(Stack *s, uint32_t reason, uint32_t *counter)
{
    s->faults |= reason;
    add(s, counter, 1);
}
static void abandon(Stack *s)
{
    /* Conservation guarantees this addition fits for a valid committed state. */
    add(s, &s->abandoned, s->depth);
    s->depth = 0;
    s->top = 0;
    s->ready = 0;
    s->lost = true;
}
void stack_init(Stack *s)
{
    memset(s, 0, sizeof *s);
}
void stack_halt(Stack *s, uint32_t reason)
{
    if (s->halted)
        return;
    abandon(s);
    s->halted = 1;
    s->faults |= reason;
}
void stack_tx_failure(Stack *s)
{
    /* Transport loss is independent of acquisition; saturation is explicit. */
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Stack *s, uint8_t raw)
{
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
        } else if (s->runs[i] < CS_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == CS_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~mask;
        }
    }
}
static void gap(Stack *s, uint32_t delta)
{
    finding(s, CS_GAP, &s->gaps);
    add(s, &s->missed, delta - 1u);
    abandon(s);
    s->valid = 0;
    s->setup = 0;
    s->baseline = 0;
    s->started = false;
    memset(s->runs, 0, sizeof s->runs);
}
static void operations(Stack *s, uint8_t raw, uint32_t previous)
{
    uint32_t rising = s->levels & ~previous;
    bool push = (rising & 4u) != 0;
    bool pop = (rising & 8u) != 0;
    unsigned tag = s->levels & 3u;
    if (!push && !pop)
        return;
    if (push)
        add(s, &s->pushes, 1);
    if (pop)
        add(s, &s->pops, 1);
    /* Count attempted push before abandoning, including malformed pushes. */
    if (push && pop) {
        finding(s, CS_COLLISION, &s->collision);
        if (push)
            add(s, &s->abandoned, 1);
        abandon(s);
        return;
    }
    /* A held opposite operation makes the next edge ambiguous as well. */
    if ((push && (s->levels & 8u)) || (pop && (s->levels & 4u))) {
        finding(s, CS_COLLISION, &s->collision);
        if (push)
            add(s, &s->abandoned, 1);
        abandon(s);
        return;
    }
    if (s->setup < CS_SETUP || (raw & 3u) != tag) {
        finding(s, CS_UNSTABLE, &s->unstable);
        if (push)
            add(s, &s->abandoned, 1);
        abandon(s);
        return;
    }
    if (push) {
        if (s->depth == CS_CAPACITY) {
            finding(s, CS_OVERFLOW, &s->overflow);
            add(s, &s->abandoned, 1);
            abandon(s);
            return;
        }
        s->tags[s->depth++] = (uint8_t)tag;
        s->top = tag;
        if (s->depth > s->peak)
            s->peak = s->depth;
        return;
    }
    if (!s->depth) {
        finding(s, CS_UNDERFLOW, &s->underflow);
        abandon(s);
        return;
    }
    if (tag != s->tags[s->depth - 1u]) {
        finding(s, CS_MISMATCH, &s->mismatch);
        abandon(s);
        return;
    }
    --s->depth;
    s->top = s->depth ? s->tags[s->depth - 1u] : 0;
    add(s, &s->matched, 1);
}
static void sample_tick(Stack *s, uint32_t now, uint8_t raw)
{
    uint32_t previous = s->levels;
    bool acquired = s->valid == 15;
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u)
        gap(s, delta);
    qualify(s, raw);
    s->started = true;
    /* Stability refers to sampled raw tag matching the qualified tag. */
    if (s->valid == 15 && (raw & 3u) == (s->levels & 3u) &&
        (previous & 3u) == (s->levels & 3u)) {
        if (s->setup < CS_SETUP)
            ++s->setup;
    } else {
        s->setup = 0;
    }
    if (!s->ready) {
        /* Quiet is only a startup convention, never proof of resynchronization. */
        if (!s->lost && s->valid == 15 && !(raw & 12u) && !(s->levels & 12u)) {
            if (s->baseline < CS_BASELINE)
                ++s->baseline;
            if (s->baseline == CS_BASELINE)
                s->ready = 1;
        } else {
            s->baseline = 0;
        }
        return;
    }
    if (acquired && s->valid == 15)
        operations(s, raw, previous);
}
void stack_sample(Stack *s, uint32_t now, uint8_t raw)
{
    if (s->started && now == s->now)
        return;
    if (s->halted) {
        s->now = now;
        return;
    }
    Stack next = *s;
    sample_tick(&next, now, raw & 15u);
    if (next.exhausted) {
        /* Preserve the old ledger, then abandon its previously pending frames. */
        s->now = now;
        stack_halt(s, CS_EXHAUSTED);
    } else {
        *s = next;
    }
}
bool stack_valid(const Stack *s)
{
    if (!s || s->levels > 15 || s->valid > 15 || s->ready > 1 || s->halted > 1)
        return false;
    if (s->depth > CS_CAPACITY || s->peak > CS_CAPACITY || s->depth > s->peak)
        return false;
    if (s->top > 3 || (!s->depth && s->top))
        return false;
    if (s->faults > 255 || (s->ready && (s->valid != 15 || s->halted || s->lost)))
        return false;
    if (!s->ready && s->depth)
        return false;
    if ((uint64_t)s->matched + s->abandoned + s->depth != s->pushes)
        return false;
    if ((uint64_t)s->matched + s->mismatch + s->underflow > s->pops)
        return false;
    if (s->overflow > s->pushes || s->peak > s->pushes)
        return false;
    if ((uint64_t)s->collision + s->unstable + s->overflow + s->mismatch + s->underflow > 1)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->halted != !!(s->faults & (CS_EXHAUSTED | CS_WATCHDOG)))
        return false;
    if (!!s->mismatch != !!(s->faults & CS_MISMATCH))
        return false;
    if (!!s->underflow != !!(s->faults & CS_UNDERFLOW))
        return false;
    if (!!s->overflow != !!(s->faults & CS_OVERFLOW))
        return false;
    if (!!s->collision != !!(s->faults & CS_COLLISION))
        return false;
    if (!!s->unstable != !!(s->faults & CS_UNSTABLE))
        return false;
    if (!!s->gaps != !!(s->faults & CS_GAP))
        return false;
    if (s->ready && s->faults)
        return false;
    return true;
}
