/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static uint32_t clock_ms;
static void hold(Ledger *s, unsigned raw, unsigned ticks)
{
    while(ticks--) {
        ledger_sample(s, ++clock_ms, raw);
        assert(ledger_valid(s));
    }
}
static void start(Ledger *s, unsigned readers)
{
    clock_ms=0; ledger_init(s); hold(s,readers,8);
    assert(s->phase==QC_READY);
}
int main(void)
{
    Ledger s;
    /* Every retirement cohort and every release ordering, including new
       post-cut readers that must not become obligations for the old object. */
    for(unsigned mask=0;mask<4;mask++) {
        for(unsigned first=0;first<2;first++) {
            start(&s,mask); hold(&s,mask|4,4);
            assert(s.cohort==mask && s.owed==mask && s.rounds==1);
            unsigned remaining=mask & ~(1u<<first);
            hold(&s,remaining,4);
            hold(&s,remaining|(1u<<first),4); /* new reader */
            hold(&s,1u<<first,4); /* release other old reader */
            assert(!s.owed);
            hold(&s,(1u<<first)|8,4);
            assert(s.accepted==1 && !s.early && s.phase==QC_DRAIN);
            hold(&s,1u<<first,4); assert(s.phase==QC_READY);
        }
    }
    start(&s,3); hold(&s,7,4); hold(&s,11,4);
    assert(s.early==1 && !s.accepted && s.owed==0);
    start(&s,1); hold(&s,5,4); hold(&s,8,4);
    assert(s.early==1); /* final exit tied to reclaim is not later */
    start(&s,0); hold(&s,12,4); assert(s.early==1);
    start(&s,0); hold(&s,8,4); assert(s.orphan==1);
    start(&s,1); hold(&s,5,4); hold(&s,1,4); hold(&s,5,4);
    assert(s.halted && s.overlap==1 && s.abandoned==1);
    start(&s,3); hold(&s,4,4); /* exit tied to retirement retained */
    assert(s.owed==3); hold(&s,0,2); assert(!s.owed);
    hold(&s,8,4); assert(s.accepted==1);
    start(&s,1); hold(&s,5,4); clock_ms+=2; hold(&s,0,1);
    assert(s.halted && s.abandoned==1 && s.gaps==1 && s.missed==2);
    start(&s,0); s.observed=UINT32_MAX; hold(&s,0,1);
    assert(s.halted && (s.faults&QC_EXHAUSTED));
    start(&s,0); s.now=UINT32_MAX; clock_ms=UINT32_MAX;
    hold(&s,0,1); assert(!s.halted);
    start(&s,0); hold(&s,4,4); hold(&s,0,4); hold(&s,8,4);
    char frame[768]; assert(wire_status(frame,sizeof frame,&s));
    assert(!wire_status(frame,4,&s)); assert(!wire_status(NULL,100,&s));
    s.accepted++; assert(!ledger_valid(&s));
    /* Exhaustion during an active cohort must discard its committed debt
       exactly once, not leave a partially retired or partially freed object. */
    start(&s,3); hold(&s,7,4); s.observed=UINT32_MAX;
    hold(&s,3,1);
    assert(s.halted && s.abandoned==1 && s.discarded==2);
    hold(&s,0,20); assert(s.discarded==2 && s.abandoned==1);
    /* Independent lane chatter must not suppress RECLAIM qualification. */
    start(&s,3); hold(&s,7,4);
    for(unsigned i=0;i<8;i++) hold(&s,10u|(i&1u),1);
    assert(s.early==1 && s.reclaims==1);
    /* Enumerate every three-vector continuation from every reader baseline.
       Each vector is paced to qualification. Check encoder invariants on
       every tick, including orphan/overlap and terminal continuations. */
    for(unsigned initial=0;initial<4;initial++)
        for(unsigned a=0;a<16;a++)
            for(unsigned b=0;b<16;b++)
                for(unsigned c=0;c<16;c++) {
                    start(&s,initial);
                    hold(&s,a,4); hold(&s,b,4); hold(&s,c,4);
                    assert(wire_status(frame,sizeof frame,&s));
                }
    puts("engine: cohorts, endpoints, exhaustion, chatter, 16384 continuations PASS");
}
