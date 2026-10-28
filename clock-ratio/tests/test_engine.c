/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Clock c;
static uint32_t now;
static void hold(unsigned mask, unsigned count)
{
    while (count--) clock_sample(&c, ++now, mask);
    assert(clock_valid(&c));
}
static void fresh(void)
{
    now=0; clock_init(&c); hold(0,10); hold(4,10);
    hold(6,5); hold(4,5); /* First tick anchors, never completes. */
    assert(c.anchored && !c.good && !c.bad);
}
static void refs(unsigned n)
{
    while(n--) { hold(5,5); hold(4,5); }
}
int main(void)
{
    char out[768];
    fresh(); refs(4); hold(6,5); hold(4,5);
    assert(c.good==1 && c.bad==0 && c.intervals==1);
    refs(3); hold(6,5); hold(4,5);
    assert(c.bad==1 && (c.faults & CR_RATIO));
    fresh(); refs(5); assert(c.faults & CR_EXCESS);
    hold(6,5); assert(c.bad==1);
    fresh(); hold(7,5); assert(c.ambiguous==1 && !c.anchored);
    fresh(); hold(4,110); assert(c.timeouts==1 && !c.anchored);
    fresh(); refs(4); c.age=96; hold(6,4);
    assert(c.good==1 && c.timeouts==0); /* inclusive exact deadline */
    fresh(); refs(4); c.age=97; hold(6,4);
    assert(c.good==0 && c.timeouts==1 && c.aborted==1);
    fresh(); c.age=97; hold(7,4);
    assert(c.timeouts==1 && c.ambiguous==0 && c.aborted==1);
    fresh();
    /* RESET chatter does not postpone an independently qualifying TICK. */
    hold(14,1); hold(6,1); hold(14,1); hold(6,1);
    assert(c.outside==0 && !c.anchored && c.aborted==1);
    fresh(); hold(12,5); assert(!c.anchored && c.aborted==1);
    fresh(); clock_sample(&c,now+4,4); now+=4;
    assert(c.gaps==1 && c.missed==3 && !c.anchored);
    hold(6,5); assert(!c.anchored); /* must see tick low first */
    hold(4,5); hold(6,5); assert(c.anchored);
    fresh(); hold(0,5); hold(2,5); assert(c.outside==1);
    fresh(); c.good=UINT32_MAX-1; c.intervals=UINT32_MAX-1; c.anchors=UINT32_MAX;
    refs(4); hold(6,5); assert(c.halted && c.good==UINT32_MAX-1);
    assert(clock_valid(&c));
    fresh(); now=UINT32_MAX-3; c.now=now;
    hold(4,10); assert(c.gaps==0);
    assert(wire_status(out,sizeof out,&c)>0);
    assert(wire_status(out,5,&c)==0);
    c.good++; assert(!clock_valid(&c));
    assert(wire_status(out,sizeof out,&c)==0);
    clock_init(&c); now=0;
    uint32_t random=12345;
    for(unsigned i=0;i<200000;i++) {
        random=random*1664525u+1013904223u;
        now += (random & 511u)==0 ? 3u : 1u;
        clock_sample(&c,now,(uint8_t)(random>>28));
        assert(clock_valid(&c));
        assert(wire_status(out,sizeof out,&c)>0);
    }
    puts("engine: ratio, missing/extra, ambiguous, reset, gap, wrap, overflow passed");
    return 0;
}
