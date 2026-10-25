/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "drain.h"
#include <string.h>

void drain_init(Drain *d)
{
    memset(d, 0, sizeof *d);
    d->state = RD_WAIT;
}

/* started bounds completed+abandoned, so closing an active trial is safe. */
static void abandon(Drain *d)
{
    if (d->state == RD_DRAIN)
        ++d->abandoned;
    d->age = 0;
    d->quiet = 0;
    d->quiet_valid = false;
    d->state = RD_WAIT;
}

void drain_halt(Drain *d, uint32_t reason)
{
    abandon(d);
    d->faults |= reason;
    d->state = RD_FAULT;
}

static bool increment(Drain *d, uint32_t *counter)
{
    if (*counter == UINT32_MAX) {
        drain_halt(d, RD_OVERFLOW);
        return false;
    }
    ++*counter;
    return true;
}

void drain_tx_failure(Drain *d)
{
    d->faults |= RD_TX;
    (void)increment(d, &d->tx_fail);
}

/* Only qualified protocol baseline, not presumed past inactivity. */
static bool baseline(uint8_t levels)
{
    return levels == 0;
}

static void start(Drain *d, uint8_t levels)
{
    if (!increment(d, &d->started))
        return;
    d->state = RD_DRAIN;
    d->age = 0;
    d->quiet = 0;
    d->quiet_valid = !(levels & RD_BUSY);
}

static bool record_admission(Drain *d, uint8_t previous, uint8_t levels)
{
    bool rising = (levels & RD_ADMIT) && !(previous & RD_ADMIT);
    /* An admission held across revoke also fails the boundary contract. */
    bool straddling = (levels & RD_ADMIT) && (levels & RD_REVOKE)
                      && !(previous & RD_REVOKE) && !rising;
    if (rising && !increment(d, &d->admissions))
        return false;
    if ((rising && ((levels | previous) & RD_REVOKE)) || straddling) {
        d->faults |= RD_LEAK;
        if (!increment(d, &d->blocked))
            return false;
    }
    return true;
}

static void update_quiet(Drain *d, uint8_t levels)
{
    if (levels & RD_BUSY) {
        d->quiet = 0;
        d->quiet_valid = false;
    } else if (!d->quiet_valid) {
        /* This is the first low observation, not one millisecond of quiet. */
        d->quiet = 0;
        d->quiet_valid = true;
    } else if (d->quiet < RD_QUIET_MS) {
        ++d->quiet;
    }
}

static void process_drain(Drain *d, uint8_t previous, uint8_t levels)
{
    bool ack = (levels & RD_ACK) && !(previous & RD_ACK);
    bool eligible = d->quiet_valid && d->quiet >= RD_QUIET_MS;
    if (!(levels & RD_REVOKE)) {
        d->faults |= RD_WITHDRAWN;
        abandon(d);
        return;
    }
    ++d->age; /* Bounded by the deadline below, never near integer wrap. */
    if (d->age > RD_DEADLINE_MS) {
        d->faults |= RD_TIMEOUT;
        abandon(d);
        return;
    }
    update_quiet(d, levels);
    if (!ack)
        return;
    /* Deadline equality alone does not demonstrate later acknowledgment. */
    if (!eligible || (levels & (RD_BUSY | RD_ADMIT))) {
        d->faults |= RD_EARLY;
        (void)increment(d, &d->early);
        return;
    }
    ++d->completed;
    d->state = RD_SEALED;
    d->age = 0;
    d->quiet = 0;
    d->quiet_valid = false;
}

void drain_sample(Drain *d, uint32_t now, uint8_t levels)
{
    levels &= 15u;
    if (!d->initialized) {
        d->initialized = true;
        d->last = now;
        d->levels = levels;
        return;
    }
    uint32_t elapsed = now - d->last;
    if (!elapsed)
        return;
    uint8_t previous = d->levels;
    d->last = now;
    d->levels = levels;
    if (d->state == RD_FAULT)
        return;
    if (elapsed != 1u) {
        d->faults |= RD_GAP;
        abandon(d);
        (void)increment(d, &d->gaps);
        return;
    }
    if (!record_admission(d, previous, levels))
        return;
    if (d->state == RD_DRAIN) {
        process_drain(d, previous, levels);
        return;
    }
    if (d->state == RD_SEALED) {
        if (levels & (RD_BUSY | RD_ADMIT))
            d->faults |= RD_REOPEN;
        if (!(levels & RD_REVOKE))
            abandon(d);
        return;
    }
    if (d->state == RD_READY && (levels & RD_REVOKE)) {
        start(d, levels);
        if (d->state == RD_FAULT)
            return;
        if (levels & RD_ACK) {
            d->faults |= RD_EARLY;
            (void)increment(d, &d->early);
        }
        return;
    }
    if ((levels & RD_ACK) && !(previous & RD_ACK))
        d->faults |= RD_ORPHAN;
    /* A READY baseline may see ordinary work before revocation. ACK cannot
       already be high at the boundary; it is diagnosed by start above. */
    if (d->state == RD_WAIT && baseline(levels))
        d->state = RD_READY;
}

bool drain_consistent(const Drain *d)
{
    if (!d || d->state > RD_FAULT || d->levels > 15u || d->faults > 1023u)
        return false;
    if (d->completed > d->started || d->abandoned > d->started - d->completed)
        return false;
    uint32_t active = d->started - d->completed - d->abandoned;
    if (active != (uint32_t)(d->state == RD_DRAIN))
        return false;
    if (d->age > RD_DEADLINE_MS || d->quiet > RD_QUIET_MS)
        return false;
    if (d->state != RD_DRAIN && (d->age || d->quiet))
        return false;
    if (d->state == RD_DRAIN && !(d->levels & RD_REVOKE))
        return false;
    if (d->state == RD_DRAIN && (d->levels & RD_BUSY) && d->quiet)
        return false;
    if (d->state == RD_SEALED && (!d->completed || !(d->levels & RD_REVOKE)))
        return false;
    if (d->blocked && !(d->faults & RD_LEAK))
        return false;
    if (d->early && !(d->faults & RD_EARLY))
        return false;
    if (d->gaps && !(d->faults & RD_GAP))
        return false;
    if (d->tx_fail && !(d->faults & RD_TX))
        return false;
    if ((d->state == RD_FAULT) != !!(d->faults & (RD_OVERFLOW | RD_WATCHDOG)))
        return false;
    return true;
}
