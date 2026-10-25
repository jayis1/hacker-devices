/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "consent.h"
#include <string.h>

/* Saturation preserves long-running evidence; timestamps alone wrap. */
static uint32_t add(uint32_t a, uint32_t b)
{
    return UINT32_MAX-a<b ? UINT32_MAX : a+b;
}

static void event(struct consent *c, uint32_t kind, uint32_t value)
{
    uint32_t sequence=c->sequence++;
    if (c->count==CG_QUEUE) {
        c->dropped=add(c->dropped,1);
        return;
    }
    unsigned slot=(c->head+c->count)%CG_QUEUE;
    c->queue[slot]=(struct cg_event){sequence,c->time,kind,value};
    ++c->count;
}

static void state(struct consent *c, enum cg_state next)
{
    if (c->state==(uint8_t)next)
        return;
    c->state=(uint8_t)next;
    c->age=0;
    event(c,CG_STATE,next);
}

static void reject(struct consent *c)
{
    c->rejected=add(c->rejected,1);
    event(c,CG_REJECTED,c->rejected);
    state(c,CG_LOCKED);
}

static void expire(struct consent *c)
{
    c->expired=add(c->expired,1);
    event(c,CG_EXPIRED,c->expired);
    state(c,CG_LOCKED);
}

static void paired(struct consent *c, uint32_t skew)
{
    c->skew_ms=skew;
    c->pairs=add(c->pairs,1);
    event(c,CG_PAIR,c->pairs);
    state(c,CG_PAIRED);
}

/* Evaluate permission against the PRIOR state and the NEW vector together.
   Pair and permission in one sample do not prove permission came later. */
static void permission(struct consent *c, uint8_t levels)
{
    bool asserted=(levels&4u)!=0;
    bool legal=c->state==CG_PAIRED && (levels&3u)==3u && !(levels&8u);
    bool unsafe=asserted && !legal;
    if (unsafe && !c->unsafe_active) {
        c->unsafe=add(c->unsafe,1);
        event(c,CG_UNSAFE,c->unsafe);
    }
    c->unsafe_active=unsafe;
}

static void qualified(struct consent *c, uint8_t levels)
{
    permission(c,levels);
    /* Inhibit cancels an accepted pair and prevents preparation. Inhibit
       deassertion cannot rearm while either consent or permission is held. */
    if (c->state==CG_PAIRED && ((levels&3u)!=3u || (levels&8u))) {
        c->revoked=add(c->revoked,1);
        event(c,CG_REVOKED,c->revoked);
    }
    if (levels==0) {
        state(c,CG_READY);
        return;
    }
    if (levels&8u) {
        state(c,CG_LOCKED);
        return;
    }
    switch (c->state) {
    case CG_UNKNOWN:
        /* Cold attach with held levels provides no baseline evidence. */
        state(c,CG_LOCKED);
        break;
    case CG_READY:
        if ((levels&3u)==3u)
            paired(c,0);
        else if (levels&4u)
            reject(c);
        else if (levels&1u)
            state(c,CG_WAIT_A);
        else if (levels&2u)
            state(c,CG_WAIT_B);
        break;
    case CG_WAIT_A:
    case CG_WAIT_B: {
        uint8_t first=c->state==CG_WAIT_A?1u:2u;
        if (!(levels&first)) {
            reject(c);
        } else if (c->age>CG_WINDOW) {
            expire(c);
        } else if ((levels&3u)==3u) {
            paired(c,c->age);
        } else if (levels&4u) {
            reject(c);
        }
        break;
    }
    case CG_PAIRED:
        if ((levels&3u)!=3u)
            state(c,CG_LOCKED);
        break;
    case CG_LOCKED:
        /* Only the all-low baseline above releases this latch. */
        break;
    default:
        state(c,CG_UNKNOWN);
        c->valid=false;
        break;
    }
}

void cg_init(struct consent *c, uint32_t now)
{
    memset(c,0,sizeof *c);
    c->time=now;
    c->state=CG_UNKNOWN;
}

void cg_sample(struct consent *c, uint32_t now, uint8_t levels)
{
    uint32_t delta=now-c->time;
    if (!delta)
        return;
    c->time=now;
    levels&=15u;
    if (delta!=1u) {
        /* Unknown acquisition intervals cannot count toward qualification or
           preserve a consent pair. Keep the last levels as invalid evidence. */
        c->gaps=add(c->gaps,1);
        c->missed=add(c->missed,delta-1u);
        c->valid=false;
        c->stable=0;
        c->candidate=levels;
        c->unsafe_active=false;
        state(c,CG_UNKNOWN);
        event(c,CG_GAP,delta-1u);
        return;
    }
    c->age=add(c->age,1);
    if (levels!=c->candidate) {
        c->candidate=levels;
        c->stable=1;
    } else if (c->stable<CG_QUALIFY) {
        ++c->stable;
    }
    if (c->stable==CG_QUALIFY && (!c->valid || levels!=c->levels)) {
        c->levels=levels;
        c->valid=true;
        event(c,CG_LEVEL,levels);
        qualified(c,levels);
    }
    /* An unchanged first input must still expire. A candidate second input
       does not extend the window until its whole vector is qualified. */
    if ((c->state==CG_WAIT_A || c->state==CG_WAIT_B) && c->age>CG_WINDOW)
        expire(c);
}

void cg_tx_fail(struct consent *c)
{
    c->tx_fail=add(c->tx_fail,1);
}

bool cg_pop(struct consent *c, struct cg_event *out)
{
    if (!out || !c->count)
        return false;
    *out=c->queue[c->head];
    c->head=(c->head+1u)%CG_QUEUE;
    --c->count;
    return true;
}
