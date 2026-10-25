/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "quarantine.h"
#include <string.h>

/* Saturating accumulated evidence cannot silently roll back after long runs. */
static uint32_t add(uint32_t a, uint32_t b)
{
    return UINT32_MAX-a<b ? UINT32_MAX : a+b;
}

static void event(struct quarantine *c, uint32_t kind, uint32_t value)
{
    uint32_t sequence=c->sequence++;
    if (c->count==RQ_QUEUE) {
        c->dropped=add(c->dropped,1);
        return;
    }
    unsigned slot=(c->head+c->count)%RQ_QUEUE;
    c->queue[slot]=(struct rq_event){sequence,c->time,kind,value};
    ++c->count;
}

static void state(struct quarantine *c, enum rq_state next)
{
    if (c->state==(uint8_t)next)
        return;
    c->state=(uint8_t)next;
    c->age=0;
    event(c,RQ_STATE,next);
}

static void increment(struct quarantine *c, uint32_t *counter, enum rq_kind kind)
{
    *counter=add(*counter,1);
    event(c,kind,*counter);
}

/* Evaluate incoming access before granting any new eligibility. This prevents
   a simultaneous READY/access or deadline/access sample from proving ordering. */
static void qualified(struct quarantine *c, uint8_t levels)
{
    bool reset=(levels&1u)!=0;
    bool ready=(levels&2u)!=0;
    bool access=(levels&4u)!=0;
    bool fault=(levels&8u)!=0;
    bool legal=c->state==RQ_ELIGIBLE && !reset && ready && !fault;
    bool unsafe=access && !legal;
    bool contradiction=reset && ready;

    if (unsafe && !c->unsafe_active)
        increment(c,&c->unsafe,RQ_UNSAFE);
    c->unsafe_active=unsafe;
    if (contradiction && !c->contradiction_active)
        increment(c,&c->contradictions,RQ_CONTRADICTION);
    c->contradiction_active=contradiction;
    if (c->state==RQ_ELIGIBLE && (reset || !ready || fault))
        increment(c,&c->revoked,RQ_REVOKED);
    if (c->state==RQ_SETTLING && (reset || !ready || access || fault))
        increment(c,&c->interrupted,RQ_INTERRUPTED);

    /* A clean asserted reset is the only recovery evidence after a fault or
       acquisition gap. READY under reset is recorded but does not hide reset.
       ACCESS or FAULT under reset cannot arm the next release. */
    if (fault || (reset && access)) {
        state(c,RQ_LOCKED);
        return;
    }
    if (reset) {
        state(c,RQ_RESET);
        return;
    }
    switch (c->state) {
    case RQ_UNKNOWN:
    case RQ_LOCKED:
        /* Cold attach and violated sessions need a newly observed reset. */
        break;
    case RQ_RESET:
    case RQ_RELEASED:
        if (access)
            state(c,RQ_LOCKED);
        else if (ready)
            state(c,RQ_SETTLING);
        else
            state(c,RQ_RELEASED);
        break;
    case RQ_SETTLING:
        if (access)
            state(c,RQ_LOCKED);
        else if (!ready)
            state(c,RQ_RELEASED);
        break;
    case RQ_ELIGIBLE:
        /* Readiness revocation cannot be repaired without another reset. */
        if (!ready)
            state(c,RQ_LOCKED);
        break;
    default:
        state(c,RQ_UNKNOWN);
        c->valid=false;
        break;
    }
}

void rq_init(struct quarantine *c, uint32_t now)
{
    memset(c,0,sizeof *c);
    c->time=now;
    c->state=RQ_UNKNOWN;
}

void rq_sample(struct quarantine *c, uint32_t now, uint8_t levels)
{
    uint32_t delta=now-c->time;
    if (!delta)
        return;
    c->time=now;
    levels&=15u;
    if (delta!=1u) {
        /* Never credit an unobserved interval toward the quiet-time contract. */
        c->gaps=add(c->gaps,1);
        c->missed=add(c->missed,delta-1u);
        c->valid=false;
        c->stable=0;
        c->candidate=levels;
        c->unsafe_active=false;
        c->contradiction_active=false;
        state(c,RQ_UNKNOWN);
        event(c,RQ_GAP,delta-1u);
        return;
    }
    c->age=add(c->age,1);
    if (levels!=c->candidate) {
        c->candidate=levels;
        c->stable=1;
    } else if (c->stable<RQ_QUALIFY) {
        ++c->stable;
    }
    if (c->stable==RQ_QUALIFY && (!c->valid || levels!=c->levels)) {
        c->levels=levels;
        c->valid=true;
        event(c,RQ_LEVEL,levels);
        qualified(c,levels);
    }
    /* The contract is explicitly over qualified vectors, not raw pin edges.
       A shorter raw pulse is outside this observer's guaranteed visibility. */
    if (c->valid && c->state==RQ_SETTLING && c->age>=RQ_WINDOW) {
        c->quiet_ms=c->age;
        increment(c,&c->releases,RQ_RELEASE);
        state(c,RQ_ELIGIBLE);
    }
}

void rq_tx_fail(struct quarantine *c)
{
    c->tx_fail=add(c->tx_fail,1);
}

bool rq_pop(struct quarantine *c, struct rq_event *out)
{
    if (!out || !c->count)
        return false;
    *out=c->queue[c->head];
    c->head=(c->head+1u)%RQ_QUEUE;
    --c->count;
    return true;
}
