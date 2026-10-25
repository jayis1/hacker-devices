/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "pair.h"
#include <string.h>
#include <limits.h>

static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX)
        ++*value;
}

static bool permitted(uint8_t levels)
{
    return (levels & (LP_VALID | LP_STOP)) == LP_VALID;
}

static uint8_t label(uint8_t levels)
{
    return (levels & LP_CLASS) ? 1u : 0u;
}

static void refresh(Pair *p)
{
    if (p->pairs == LP_COHORT)
        p->state = LP_DONE;
    else if (p->active)
        p->state = LP_ACTIVE;
    else if (p->ready)
        p->state = LP_READY;
    else
        p->state = LP_WAIT;
}

void pair_init(Pair *p)
{
    memset(p, 0, sizeof *p);
}

/* One rejection denotes an abandoned candidate pair, possibly just its first
 * operation. Both halves are discarded together: never salvage a biased half. */
static void abandon(Pair *p, uint32_t reason)
{
    if (p->active || p->pending)
        increment(&p->rejected);
    p->faults |= reason;
    p->active = false;
    p->pending = false;
    p->ready = false;
    p->age = 0;
    p->pending_age = 0;
    p->first_duration = 0;
}

static void accumulate(Pair *p, uint32_t duration)
{
    uint32_t a = p->active_label ? p->first_duration : duration;
    uint32_t b = p->active_label ? duration : p->first_duration;
    uint32_t delta = a > b ? a - b : b - a;
    /* At most 32 pairs, each half at most 1000 ticks. No wrapping sum. */
    p->sum_a += a;
    p->sum_b += b;
    ++p->pairs;
    if (a > b)
        ++p->a_slower;
    else if (b > a)
        ++p->b_slower;
    else
        ++p->equal;
    if (delta >= LP_THRESHOLD)
        ++p->large;
    if (delta > p->max_delta)
        p->max_delta = delta;
    p->pending = false;
    p->pending_age = 0;
    p->first_duration = 0;
}

static void complete(Pair *p)
{
    if (!p->pending) {
        p->first_label = p->active_label;
        p->first_duration = p->age;
        p->pending = true;
        p->pending_age = 0;
    } else {
        accumulate(p, p->age);
    }
    p->active = false;
    p->age = 0;
    /* The falling observation itself provides an idle baseline. */
    p->ready = true;
}

static void observe_active(Pair *p, uint8_t levels)
{
    /* Check class on the completion observation too. This deliberately rejects
     * an adapter that changes class at the same sample as its falling edge. */
    if (label(levels) != p->active_label) {
        abandon(p, LP_LABEL);
        return;
    }
    ++p->age;
    if (p->age > LP_MAX_DURATION) {
        abandon(p, LP_TIMEOUT);
        return;
    }
    if (!(levels & LP_BUSY))
        complete(p);
}

static void observe_idle(Pair *p, uint8_t previous, uint8_t levels)
{
    if (!(levels & LP_BUSY)) {
        p->ready = true;
        return;
    }
    if (!p->ready)
        return; /* A high-at-attach pulse has no known starting edge. */
    p->active = true;
    p->ready = false;
    p->active_label = label(levels);
    p->age = 0;
    if (label(previous) != p->active_label) {
        abandon(p, LP_LABEL);
        return;
    }
    if (p->pending && p->active_label == p->first_label)
        abandon(p, LP_ORDER);
}

void pair_sample(Pair *p, uint32_t now, uint8_t levels)
{
    levels &= 15u;
    if (!p->initialized) {
        p->initialized = true;
        p->last = now;
        p->levels = levels;
        p->ready = permitted(levels) && !(levels & LP_BUSY);
        refresh(p);
        return;
    }
    uint32_t delta = now - p->last;
    if (!delta)
        return;
    uint8_t previous = (uint8_t)p->levels;
    p->last = now;
    p->levels = levels;
    /* Completed cohorts are immutable; transport failures remain reportable. */
    if (p->pairs == LP_COHORT)
        return;
    if (delta != 1u) {
        increment(&p->gaps);
        abandon(p, LP_GAP);
        refresh(p);
        return;
    }
    if (!permitted(levels)) {
        uint32_t reason = 0;
        if (!(levels & LP_VALID))
            reason |= LP_INVALID;
        if (levels & LP_STOP)
            reason |= LP_INHIBIT;
        if (p->active || p->pending)
            abandon(p, reason);
        p->ready = false;
        refresh(p);
        return;
    }
    if (p->pending) {
        ++p->pending_age;
        if (p->pending_age >= LP_PAIR_WAIT) {
            abandon(p, LP_TIMEOUT);
            refresh(p);
            return;
        }
    }
    if (p->active)
        observe_active(p, levels);
    else
        observe_idle(p, previous, levels);
    refresh(p);
}

void pair_tx_failure(Pair *p)
{
    increment(&p->tx_fail);
    p->faults |= LP_TX;
}

bool pair_valid(const Pair *p)
{
    if (!p || p->levels > 15u || p->state > LP_DONE || p->faults > 255u)
        return false;
    if (p->pairs > LP_COHORT || p->large > p->pairs)
        return false;
    if (p->a_slower > p->pairs || p->b_slower > p->pairs || p->equal > p->pairs)
        return false;
    if (p->a_slower + p->b_slower + p->equal != p->pairs)
        return false;
    if (p->sum_a < p->pairs || p->sum_b < p->pairs)
        return false;
    if (p->sum_a > p->pairs * LP_MAX_DURATION || p->sum_b > p->pairs * LP_MAX_DURATION)
        return false;
    if (p->max_delta >= LP_MAX_DURATION || (p->large == 0 && p->max_delta >= LP_THRESHOLD))
        return false;
    if (p->large && p->max_delta < LP_THRESHOLD)
        return false;
    if (!p->pairs && p->max_delta)
        return false;
    uint32_t unequal = p->a_slower + p->b_slower;
    if (p->large > unequal || (unequal == 0) != (p->max_delta == 0))
        return false;
    int32_t difference = (int32_t)p->sum_b - (int32_t)p->sum_a;
    int32_t lower = (int32_t)p->b_slower - (int32_t)(p->a_slower * p->max_delta);
    int32_t upper = (int32_t)(p->b_slower * p->max_delta) - (int32_t)p->a_slower;
    if (difference < lower || difference > upper)
        return false;
    uint32_t absolute = difference < 0 ? (uint32_t)-difference : (uint32_t)difference;
    if (p->pairs == 1u && p->max_delta != absolute)
        return false;
    if (p->state == LP_ACTIVE && (p->levels & 13u) != 5u)
        return false;
    if (p->state == LP_READY && (p->levels & 13u) != 4u)
        return false;
    if (p->gaps && !(p->faults & LP_GAP))
        return false;
    if (p->tx_fail && !(p->faults & LP_TX))
        return false;
    if (p->age > LP_MAX_DURATION || p->pending_age >= LP_PAIR_WAIT)
        return false;
    if (!p->active && p->age)
        return false;
    if (!p->pending && (p->pending_age || p->first_duration))
        return false;
    if (p->pending && (!p->first_duration || p->first_duration > LP_MAX_DURATION))
        return false;
    if ((p->state == LP_DONE) != (p->pairs == LP_COHORT))
        return false;
    if (p->state != LP_DONE && (p->state == LP_ACTIVE) != p->active)
        return false;
    return true;
}
