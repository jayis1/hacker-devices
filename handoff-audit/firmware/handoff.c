/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "handoff.h"
#include <string.h>

static uint32_t add(uint32_t value, uint32_t delta)
{
    if (UINT32_MAX-value < delta)
        return UINT32_MAX;
    return value+delta;
}

static void increment(uint32_t *value)
{
    *value=add(*value,1);
}

static void event(struct handoff *h, enum ha_kind kind, uint32_t value)
{
    uint32_t sequence=h->sequence++;
    if (h->count==HA_QUEUE) {
        increment(&h->dropped);
        return;
    }
    unsigned index=(h->head+h->count)%HA_QUEUE;
    h->queue[index].sequence=sequence;
    h->queue[index].time=h->time;
    h->queue[index].kind=(uint32_t)kind;
    h->queue[index].value=value;
    ++h->count;
}

bool ha_pop(struct handoff *h, struct ha_event *out)
{
    if (!h || !out || !h->count)
        return false;
    *out=h->queue[h->head];
    h->head=(h->head+1)%HA_QUEUE;
    --h->count;
    return true;
}

void ha_tx_fail(struct handoff *h)
{
    if (h)
        increment(&h->tx_fail);
}

void ha_init(struct handoff *h, uint32_t now)
{
    if (!h)
        return;
    memset(h,0,sizeof *h);
    h->time=now;
}

static void forget_interval(struct handoff *h)
{
    /* Lost observations cannot establish a safe transfer between domains. */
    h->synced=false;
    h->last_path=0;
    h->off_seen=false;
    h->off_age=0;
    for (unsigned i=0;i<2;++i) {
        h->mismatch_active[i]=false;
        h->mismatch_reported[i]=false;
        h->mismatch_age[i]=0;
    }
}

static void ages(struct handoff *h)
{
    if (!h->synced)
        return;
    if (h->off_seen && ((h->levels>>2)&3u)==0)
        h->off_age=add(h->off_age,1);
    for (unsigned i=0;i<2;++i) {
        if (!h->mismatch_active[i])
            continue;
        h->mismatch_age[i]=add(h->mismatch_age[i],1);
        if (h->mismatch_age[i]>=HA_SETTLE_MS && !h->mismatch_reported[i]) {
            h->mismatch_reported[i]=true;
            increment(&h->mismatch[i]);
            event(h,i==0?HA_MISMATCH_A:HA_MISMATCH_B,h->mismatch[i]);
        }
    }
}

static void track_mismatch(struct handoff *h, uint8_t vector)
{
    for (unsigned i=0;i<2;++i) {
        bool command=(vector & (1u<<i))!=0;
        bool feedback=(vector & (4u<<i))!=0;
        bool different=command!=feedback;
        if (different!=h->mismatch_active[i]) {
            h->mismatch_age[i]=0;
            h->mismatch_reported[i]=false;
        }
        h->mismatch_active[i]=different;
    }
}

static void feedback(struct handoff *h, uint8_t before, uint8_t after)
{
    if (after==3) {
        if (before!=3) {
            increment(&h->feedback_overlap);
            event(h,HA_FEEDBACK_OVERLAP,h->feedback_overlap);
        }
        /* An overlap cannot be counted as a break, even if one line falls. */
        h->off_seen=false;
        h->off_age=0;
        return;
    }
    if (after==0) {
        if (before!=0) {
            h->off_seen=true;
            h->off_age=0;
        }
        return;
    }
    if (after==before)
        return;
    if (h->last_path && after!=h->last_path) {
        h->last_dead=h->off_seen?h->off_age:0;
        increment(&h->transitions);
        event(h,HA_TRANSITION,h->last_dead);
        if (h->last_dead<HA_BREAK_MS) {
            increment(&h->short_break);
            event(h,HA_SHORT_BREAK,h->last_dead);
        }
    }
    h->last_path=after;
    h->off_seen=false;
    h->off_age=0;
}

static void qualify(struct handoff *h, uint8_t vector)
{
    uint8_t previous=h->levels;
    h->levels=vector;
    if (previous!=vector)
        event(h,HA_LEVELS,vector);
    if (!h->synced) {
        if (vector!=0)
            return;
        h->synced=true;
        h->last_path=0;
        h->off_seen=true;
        h->off_age=0;
        event(h,HA_SYNC,1);
        return;
    }
    if ((vector&3u)==3u && (previous&3u)!=3u) {
        increment(&h->command_overlap);
        event(h,HA_COMMAND_OVERLAP,h->command_overlap);
    }
    feedback(h,(previous>>2)&3u,(vector>>2)&3u);
    track_mismatch(h,vector);
}

void ha_sample(struct handoff *h, uint32_t now, uint8_t raw)
{
    if (!h)
        return;
    uint32_t elapsed=now-h->time;
    if (!elapsed)
        return;
    h->time=now;
    raw&=15u;
    if (elapsed!=1) {
        increment(&h->gaps);
        h->missed=add(h->missed,elapsed-1);
        event(h,HA_GAP,elapsed-1);
        forget_interval(h);
        event(h,HA_SYNC,0);
        h->samples=0;
        /* Preserve previous qualified levels; the new raw vector is not evidence. */
    } else {
        ages(h);
    }
    if (h->samples==0 || h->candidate!=raw) {
        h->candidate=raw;
        h->samples=1;
    } else if (h->samples<HA_QUALIFY) {
        ++h->samples;
    } else {
        return;
    }
    if (h->samples==HA_QUALIFY)
        qualify(h,raw);
}
