/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Every acquisition update is transactional: exhaustion cannot publish a
 * half-counted transfer or destroy starts=completed+aborted+active. */
static bool increment(uint32_t *value)
{
    if (*value == UINT32_MAX)
        return false;
    ++*value;
    return true;
}

static bool add(uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount)
        return false;
    *value += amount;
    return true;
}

void latch_init(Latch *c)
{
    memset(c, 0, sizeof *c);
}

void latch_halt(Latch *c, uint32_t fault)
{
    /* An active start reserves one representable terminal outcome. */
    if (c->active) {
        ++c->aborted;
        c->active = false;
    }
    c->age = 0;
    c->trial_faults = 0;
    c->halted = true;
    c->faults |= fault;
}

void latch_tx_failure(Latch *c)
{
    /* Transport diagnostics saturate without altering retained acquisition. */
    if (c->tx_fail != UINT32_MAX)
        ++c->tx_fail;
}

static void reset_qualification(Latch *c, uint8_t raw)
{
    c->candidate = raw;
    memset(c->stable, 0, sizeof c->stable);
    c->valid = 0;
    c->quiet = 0;
    c->low_seen = false;
}

static void qualify(Latch *c, uint8_t raw)
{
    /* Independent channels prevent a chattering strobe from hiding data. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ c->candidate) & bit) {
            c->candidate ^= bit;
            c->stable[i] = 0;
        } else if (c->stable[i] < PL_QUALIFY) {
            ++c->stable[i];
        }
        if (c->stable[i] == PL_QUALIFY) {
            c->levels = (uint8_t)((c->levels & ~bit) | (raw & bit));
            c->valid |= bit;
        }
    }
}

static bool abort_trial(Latch *c)
{
    if (!c->active)
        return true;
    if (!increment(&c->aborted))
        return false;
    c->active = false;
    c->age = 0;
    c->trial_faults = 0;
    return true;
}

static bool mark_trial(Latch *c, uint8_t bit, uint32_t *counter)
{
    if (c->trial_faults & bit)
        return true;
    if (!increment(counter))
        return false;
    c->trial_faults |= bit;
    c->faults |= bit;
    return true;
}

static bool finish_trial(Latch *c)
{
    if (!increment(&c->completed))
        return false;
    if (c->trial_faults) {
        if (!increment(&c->bad))
            return false;
    } else {
        if (!increment(&c->good))
            return false;
        if (!increment(&c->bins[c->captured & 3u]))
            return false;
    }
    c->active = false;
    c->age = 0;
    c->trial_faults = 0;
    return true;
}

static bool odd(uint8_t code)
{
    return ((code ^ (code >> 1) ^ (code >> 2)) & 1u) != 0;
}

static bool begin_trial(Latch *c, uint8_t raw)
{
    if (!increment(&c->starts))
        return false;
    c->active = true;
    c->age = 0;
    c->trial_faults = 0;
    c->captured = c->levels & 7u;
    if (!odd(c->captured)) {
        if (!mark_trial(c, PL_PARITY, &c->parity))
            return false;
    }
    /* Quiet time is capped, not accumulated across a timer rollover. */
    if (c->quiet < PL_SETUP || (raw & 7u) != c->captured) {
        if (!mark_trial(c, PL_SETUP_FAULT, &c->setup))
            return false;
    }
    return true;
}

static bool gap(Latch *c, uint8_t raw, uint32_t elapsed)
{
    if (!increment(&c->gaps))
        return false;
    if (!add(&c->missed, elapsed - 1u))
        return false;
    c->faults |= PL_GAP;
    if (!abort_trial(c))
        return false;
    reset_qualification(c, raw);
    return true;
}

static bool update(Latch *c, uint32_t now, uint8_t raw)
{
    uint32_t elapsed = now - c->now;
    uint8_t prior = c->levels;
    uint8_t prior_valid = c->valid;
    c->now = now;
    if (!c->started) {
        c->started = true;
        reset_qualification(c, raw);
        return true;
    }
    if (elapsed != 1u)
        return gap(c, raw, elapsed);

    /* A raw excursion breaks setup even when debounce rejects it. The
     * elapsed interval ending at a changed raw observation is not trusted. */
    if ((prior_valid & 7u) == 7u && (raw & 7u) == (prior & 7u)) {
        if (c->quiet < PL_SETUP)
            ++c->quiet;
    } else {
        c->quiet = 0;
    }
    qualify(c, raw);
    if (c->valid != 15u)
        return true;

    bool rising = prior_valid == 15u && !(prior & 8u) && (c->levels & 8u);
    if (!(c->levels & 8u))
        c->low_seen = true;

    /* A pending transfer is checked at the endpoint too: a data change at
     * exactly PL_HOLD is conservatively a hold failure. */
    if (c->active) {
        ++c->age;
        if ((raw & 7u) != c->captured || (c->levels & 7u) != c->captured) {
            if (!mark_trial(c, PL_HOLD_FAULT, &c->hold))
                return false;
        }
        if (rising) {
            if (!increment(&c->collisions))
                return false;
            c->faults |= PL_COLLISION;
            if (!abort_trial(c))
                return false;
            /* This strobe is a separately counted abandoned transfer. */
            if (!increment(&c->starts) || !increment(&c->aborted))
                return false;
            c->low_seen = false;
            return true;
        }
        if (c->age >= PL_HOLD) {
            if (!finish_trial(c))
                return false;
        }
    }
    if (rising && c->low_seen) {
        c->low_seen = false;
        return begin_trial(c, raw);
    }
    return true;
}

void latch_sample(Latch *c, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (c->halted) {
        c->now = now;
        return;
    }
    if (c->started && now == c->now)
        return;
    Latch next = *c;
    if (!update(&next, now, raw)) {
        c->now = now;
        latch_halt(c, PL_EXHAUSTED);
        return;
    }
    *c = next;
}

bool latch_valid(const Latch *c)
{
    if (!c || c->levels > 15 || c->valid > 15 || c->candidate > 15)
        return false;
    if (c->captured > 7 || c->age >= PL_HOLD || c->quiet > PL_SETUP)
        return false;
    if (c->faults > 127 || c->trial_faults > 7)
        return false;
    if ((uint64_t)c->good + c->bad != c->completed)
        return false;
    if ((uint64_t)c->completed + c->aborted + c->active != c->starts)
        return false;
    uint64_t bins = 0;
    for (unsigned i = 0; i < 4; ++i) {
        bins += c->bins[i];
        if (c->stable[i] > PL_QUALIFY)
            return false;
    }
    if (bins != c->good)
        return false;
    if (!c->active && (c->age || c->trial_faults))
        return false;
    if (c->active && (c->valid != 15 || c->halted))
        return false;
    if (c->halted != ((c->faults & (PL_EXHAUSTED | PL_WATCHDOG)) != 0))
        return false;
    if ((c->parity != 0) != ((c->faults & PL_PARITY) != 0))
        return false;
    if ((c->setup != 0) != ((c->faults & PL_SETUP_FAULT) != 0))
        return false;
    if ((c->hold != 0) != ((c->faults & PL_HOLD_FAULT) != 0))
        return false;
    if ((c->collisions != 0) != ((c->faults & PL_COLLISION) != 0))
        return false;
    if ((c->gaps != 0) != ((c->faults & PL_GAP) != 0))
        return false;
    if (c->missed < c->gaps || (!c->gaps && c->missed))
        return false;
    if (c->parity > c->starts || c->setup > c->starts || c->hold > c->starts)
        return false;
    uint64_t failed_or_pending = (uint64_t)c->bad + c->aborted + c->active;
    if (c->parity > failed_or_pending || c->setup > failed_or_pending ||
        c->hold > failed_or_pending)
        return false;
    if ((uint64_t)c->collisions * 2u > c->aborted)
        return false;
    if ((c->trial_faults & c->faults) != c->trial_faults)
        return false;
    if (c->bad > (uint64_t)c->parity + c->setup + c->hold)
        return false;
    return true;
}
