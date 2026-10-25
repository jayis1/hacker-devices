/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "erase.h"
#include <string.h>

static uint32_t add(uint32_t value, uint32_t increment)
{
    if (UINT32_MAX-value<increment)
        return UINT32_MAX;
    return value+increment;
}

static void emit(struct erase *e, unsigned kind, uint32_t value)
{
    struct ew_event event={e->sequence++,e->time,kind,value};
    if (e->count==EW_QUEUE) {
        e->dropped=add(e->dropped,1);
        return;
    }
    unsigned tail=(e->head+e->count)%EW_QUEUE;
    e->queue[tail]=event;
    ++e->count;
}

bool ew_pop(struct erase *e, struct ew_event *out)
{
    if (!e || !out || !e->count)
        return false;
    *out=e->queue[e->head];
    e->head=(e->head+1)%EW_QUEUE;
    --e->count;
    return true;
}

void ew_tx_fail(struct erase *e)
{
    e->tx_fail=add(e->tx_fail,1);
}

void ew_init(struct erase *e, uint32_t now)
{
    memset(e,0,sizeof *e);
    e->time=now;
    e->state=EW_UNKNOWN;
}

static void transition(struct erase *e, enum ew_state state)
{
    if (e->state==state)
        return;
    e->state=(uint8_t)state;
    e->elapsed=0;
    emit(e,EW_STATE,state);
}

static void reject(struct erase *e)
{
    e->rejected=add(e->rejected,1);
    emit(e,EW_REJECT,e->rejected);
    transition(e,EW_FAULT);
}

static void access_check(struct erase *e, uint8_t next)
{
    /* Completion and access must be separately observed qualified vectors.
     * Previously completed state alone is insufficient if its handshake drops.
     */
    bool permitted=e->state==EW_COMPLETE && (next&7u)==5u;
    bool unsafe=(next&8u) && !permitted;
    if (unsafe && !e->unsafe_active) {
        e->unsafe=add(e->unsafe,1);
        emit(e,EW_UNSAFE,e->unsafe);
    }
    e->unsafe_active=unsafe;
}

static void qualified(struct erase *e, uint8_t next)
{
    bool changed=!e->valid || e->levels!=next;
    e->valid=true;
    e->levels=next;
    if (changed)
        emit(e,EW_LEVEL,next);
    access_check(e,next);
    unsigned handshake=next&7u;
    /* A full all-low vector is the only resynchronization boundary. */
    if (!next) {
        if (e->state==EW_WAIT_BUSY || e->state==EW_BUSY) {
            e->aborted=add(e->aborted,1);
            emit(e,EW_ABORT,e->aborted);
        }
        e->busy_ms=0;
        transition(e,EW_IDLE);
        return;
    }
    switch (e->state) {
    case EW_UNKNOWN:
    case EW_FAULT:
        /* Do not infer a successful cycle from an unobserved prefix. */
        return;
    case EW_IDLE:
        if (handshake==1u) {
            e->busy_ms=0;
            transition(e,EW_WAIT_BUSY);
        } else if (handshake!=0u) {
            reject(e);
        }
        break;
    case EW_WAIT_BUSY:
        if (handshake==3u) {
            e->busy_ms=0;
            transition(e,EW_BUSY);
        } else if (handshake!=1u) {
            reject(e);
        }
        break;
    case EW_BUSY:
        if (handshake==5u && e->busy_ms>=EW_MIN_BUSY) {
            e->completed=add(e->completed,1);
            emit(e,EW_COMPLETED,e->completed);
            transition(e,EW_COMPLETE);
        } else if (handshake!=3u) {
            reject(e);
        }
        break;
    case EW_COMPLETE:
        if (handshake!=5u)
            reject(e);
        break;
    default:
        transition(e,EW_UNKNOWN);
        break;
    }
}

static void interval(struct erase *e)
{
    if (!e->valid)
        return;
    e->elapsed=add(e->elapsed,1);
    if (e->state==EW_BUSY)
        e->busy_ms=add(e->busy_ms,1);
    bool expired=(e->state==EW_WAIT_BUSY && e->elapsed>=EW_START_LIMIT)
              || (e->state==EW_BUSY && e->elapsed>=EW_FINISH_LIMIT);
    if (expired) {
        e->timeouts=add(e->timeouts,1);
        emit(e,EW_TIMEOUT,e->timeouts);
        transition(e,EW_FAULT);
    }
}

void ew_sample(struct erase *e, uint32_t now, uint8_t levels)
{
    uint32_t delta=now-e->time;
    if (!delta)
        return;
    e->time=now;
    levels&=15u;
    if (delta!=1u) {
        e->gaps=add(e->gaps,1);
        e->missed=add(e->missed,delta-1u);
        e->valid=false;
        e->stable=0;
        e->busy_ms=0;
        e->unsafe_active=false;
        transition(e,EW_UNKNOWN);
        emit(e,EW_GAP,delta-1u);
        /* The first post-gap observation does not qualify continuity. */
        return;
    }
    interval(e);
    if (!e->stable || levels!=e->candidate) {
        e->candidate=levels;
        e->stable=1;
    } else if (e->stable<EW_QUALIFY) {
        ++e->stable;
    }
    if (e->stable==EW_QUALIFY)
        qualified(e,levels);
}
