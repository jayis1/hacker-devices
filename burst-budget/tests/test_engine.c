/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "budget.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Budget s;
static uint32_t now;
static void ticks(unsigned n, unsigned levels)
{
    while(n--) { budget_sample(&s, ++now, levels); assert(budget_valid(&s)); }
}
static void ready(void)
{
    now=0; budget_init(&s); budget_sample(&s,now,4);
    ticks(4050,4); assert(s.phase==BB_TRACKING && s.tokens==4);
}
static void pulse(void) { ticks(10,5); ticks(10,4); }
int main(void)
{
    ready();
    for(unsigned i=0;i<4;++i) pulse();
    assert(s.allowed==4 && s.excess==0 && s.tokens==0);
    pulse(); assert(s.excess==1 && (s.faults&BB_EXCESS));
    ticks(1001,4); assert(s.tokens==1); pulse(); assert(s.allowed==5);
    ready(); ticks(10,13); assert(s.denied==1 && s.allowed==0);
    ticks(10,4); ticks(10,0); assert(s.phase==BB_UNKNOWN && !s.tokens);
    ticks(10,1); assert(s.denied==2);
    ticks(10,0); ticks(10,1); assert(s.denied==3);
    ready(); budget_sample(&s,now+10,4); now+=10;
    assert(s.gaps==1 && s.phase==BB_UNKNOWN && s.tokens==0);
    ticks(4050,4); assert(s.tokens==4);
    /* Independent qualification: throttle chatter cannot hide admission. */
    ready(); for(unsigned i=0;i<10;++i) ticks(1,5|((i&1)<<1));
    assert(s.allowed==1);
    /* Refill at the same sampled instant cannot authorize an empty bucket. */
    ready(); s.discarded+=s.tokens; s.tokens=0; s.refill=999; s.levels=4;
    s.candidate[0]=1; s.stable[0]=2;
    ticks(1,5); assert(s.excess==1 && s.tokens==1);
    /* Held high is one edge, not unlimited repeated consumption. */
    ready(); ticks(100,5); assert(s.allowed==1);
    /* Unsigned timestamp wrap remains continuous. */
    ready(); s.now=UINT32_MAX; now=UINT32_MAX; ticks(1,4); assert(!s.gaps);
    ready(); s.admissions=UINT32_MAX; s.denied=UINT32_MAX; s.faults|=BB_DENIED;
    ticks(10,5); assert(s.phase==BB_HALTED && (s.faults&BB_EXHAUSTED));
    assert(s.admissions==s.denied);
    ready(); s.discarded+=s.tokens; s.tokens=0; ticks(21,4); assert(s.missing>0);
    ticks(10,6); unsigned old=s.missing; ticks(10,6); assert(s.missing==old);
    char frame[384]; assert(wire_status(frame,sizeof frame,&s)>0);
    assert(wire_status(frame,2,&s)==0);
    s.tokens=5; assert(!budget_valid(&s));
    ready(); ticks(999,4); pulse();
    assert(s.tokens==3 && s.refill<20);
    /* Reset and admission qualify together: reset takes precedence. */
    ready(); ticks(10,1); assert(s.denied==1 && s.allowed==0 && s.discarded==4);
    /* Repeated same-time samples do not earn credits or qualification. */
    ready(); for(unsigned i=0;i<1000;++i) budget_sample(&s,now,5);
    assert(s.admissions==0 && s.tokens==4);
    /* Deterministic hostile activity preserves both accounting equations. */
    ready(); unsigned rng=1234567;
    for(unsigned i=0;i<100000;++i) {
        rng=rng*1664525u+1013904223u;
        ticks(1,(rng>>24)&15);
        if((rng&1023)==0) { now+=5; budget_sample(&s,now,4); }
        assert(budget_valid(&s));
    }
    puts("PASS budget policy, qualification, gaps, wrap, exhaustion, encoder bounds");
}
