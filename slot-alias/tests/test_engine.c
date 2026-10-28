/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Slots s;
static uint32_t clock_ms;
static void ticks(unsigned n, unsigned raw)
{
    while (n--) {
        slots_sample(&s, ++clock_ms, raw);
        assert(slots_valid(&s));
    }
}
static void start(void)
{
    slots_init(&s);
    clock_ms = 0;
    ticks(30, 0);
    assert(s.ready);
}
static void operation(unsigned tag, unsigned signal)
{
    ticks(12, tag);
    ticks(8, tag | signal);
    ticks(8, tag);
}
int main(void)
{
    char frame[768];
    start();
    operation(1, 4); operation(2, 4);
    assert(s.pending == 2 && s.occupied == 6);
    operation(1, 8); operation(2, 8); /* FIFO, not LIFO, is allowed. */
    assert(s.pending == 0 && s.retired == 2 && s.faults == 0);
    operation(1,4); operation(1,8);
    assert(s.reuse == 1 && s.retired == 3);
    start(); operation(1,4); operation(1,4);
    assert(s.duplicate == 1 && s.abandoned == 2 && !s.ready);
    ticks(100,0); assert(!s.ready);
    start(); operation(1,8);
    assert(s.orphan == 1 && !s.ready);
    start(); operation(1,4); operation(2,8);
    assert(s.orphan == 1 && s.abandoned == 1);
    start();
    for(unsigned i=0;i<4;i++) operation(i,4);
    assert(s.pending==4 && s.occupied==15);
    operation(2,4);
    assert(s.duplicate==1 && s.allocations==5 && s.abandoned==5);
    start(); operation(1,4); ticks(12,2); ticks(8,14);
    assert(s.collision==1 && s.allocations==2 && s.releases==1 && s.abandoned==2);
    start(); operation(1,4);
    clock_ms+=10; slots_sample(&s,++clock_ms,0);
    assert(s.gaps==1 && s.abandoned==1 && !s.ready);
    ticks(40,0); assert(!s.ready);
    start(); ticks(8,5);
    assert(s.unstable==1 && s.abandoned==1);
    start(); ticks(2,4); ticks(10,0); assert(s.allocations==0);
    /* Initial tag change qualifies on tick four; setup starts next tick. */
    start(); ticks(4,1); ticks(1,1); ticks(4,5);
    assert(s.ready && s.allocations==1 && s.setup==5);
    start(); ticks(4,1); ticks(4,5);
    assert(s.unstable==1 && !s.ready); /* Only four stable intervals. */
    /* Unqualified raw excursion still invalidates setup. */
    start(); ticks(1,1); ticks(4,4);
    assert(s.unstable==1);
    /* FREE qualifies despite tag chatter, but setup correctly fails. */
    start();
    for(unsigned i=0;i<4;i++) ticks(1,8|(i&1));
    assert(s.releases==1 && s.unstable==1);
    /* A held opposite strobe cannot be interpreted as free-and-replace. */
    start(); ticks(8,4); ticks(8,12);
    assert(s.allocations==1 && s.releases==1 && s.collision==1);
    start(); s.allocations=UINT32_MAX; s.retired=UINT32_MAX; s.releases=UINT32_MAX;
    s.seen=1; s.reuse=UINT32_MAX-1; s.peak=1;
    operation(1,4);
    assert(s.halted && s.pending==0 && slots_valid(&s));
    assert(wire_status(frame,sizeof frame,&s));
    assert(!wire_status(frame,1,&s));
    start(); clock_ms=UINT32_MAX-20; s.now=clock_ms;
    ticks(40,0); assert(!s.gaps);
    /* Exhaustive release permutations: no ordering policy is imposed. */
    for(unsigned a=0;a<4;a++) for(unsigned b=0;b<4;b++)
    for(unsigned c=0;c<4;c++) for(unsigned d=0;d<4;d++) {
        if(a==b||a==c||a==d||b==c||b==d||c==d) continue;
        start(); for(unsigned i=0;i<4;i++) operation(i,4);
        operation(a,8); operation(b,8); operation(c,8); operation(d,8);
        assert(s.retired==4 && s.faults==0 && s.occupied==0);
    }
    /* Independent reference bitset over long valid allocation/release traffic. */
    start(); unsigned random=123, mask=0;
    for(unsigned i=0;i<10000;i++) {
        random=random*1664525u+1013904223u;
        unsigned tag=(random>>28)&3, bit=1u<<tag;
        operation(tag,mask&bit ? 8 : 4); mask^=bit;
        assert(s.occupied==mask && !s.faults);
    }
    Slots bad=s; bad.occupied^=1;
    assert(!slots_valid(&bad));
    start();
    for(unsigned i=0;i<100000;i++) {
        random=random*1664525u+1013904223u;
        ticks(1,random>>28);
    }
    puts("engine: alias detection, all release permutations, reuse, gaps, setup, wrap, exhaustion and independent oracle passed");
}
