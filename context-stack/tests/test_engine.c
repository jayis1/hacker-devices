/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Stack s;
static uint32_t clock_ms;
static void ticks(unsigned n, unsigned raw)
{
    while (n--) {
        stack_sample(&s, ++clock_ms, raw);
        assert(stack_valid(&s));
    }
}
static void start(void)
{
    stack_init(&s);
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
    assert(s.depth == 2 && s.top == 2);
    operation(2, 8); operation(1, 8);
    assert(s.depth == 0 && s.matched == 2 && s.faults == 0);
    start(); operation(1, 4); operation(2, 8);
    assert(s.mismatch == 1 && s.abandoned == 1 && !s.ready);
    ticks(100, 0); assert(!s.ready); /* Never infer empty from silence. */
    start(); operation(1, 8);
    assert(s.underflow == 1 && !s.ready);
    start();
    for (unsigned i=0;i<8;i++) operation(i%4,4);
    assert(s.depth==8);
    operation(0,4);
    assert(s.overflow==1 && s.pushes==9 && s.abandoned==9 && !s.ready);
    start(); operation(1,4); ticks(12,2); ticks(8,14);
    assert(s.collision==1 && s.pushes==2 && s.pops==1 && s.abandoned==2);
    start(); operation(1,4);
    clock_ms+=10; stack_sample(&s,++clock_ms,0);
    assert(s.gaps==1 && s.abandoned==1 && !s.ready);
    ticks(40,0); assert(!s.ready);
    start(); ticks(8,5); /* tag and push together lack setup */
    assert(s.unstable==1 && s.abandoned==1);
    start(); ticks(2,4); ticks(10,0); assert(s.pushes==0);
    start(); operation(3,4); operation(3,4); operation(3,8); operation(3,8);
    assert(s.matched==2); /* Reentrant equal tags are legitimate. */
    start(); s.pushes=UINT32_MAX; s.matched=UINT32_MAX; s.pops=UINT32_MAX;
    operation(1,4);
    assert(s.halted && s.depth==0 && stack_valid(&s));
    assert(wire_status(frame,sizeof frame,&s));
    assert(!wire_status(frame,1,&s));
    start(); clock_ms=UINT32_MAX-20; s.now=clock_ms;
    ticks(40,0); assert(!s.gaps);
    start();
    unsigned random=123;
    for(unsigned i=0;i<100000;i++) {
        random=random*1664525u+1013904223u;
        ticks(1,random>>28);
    }
    assert(stack_valid(&s));
    puts("engine: nested returns, mismatch, bounds, gaps, setup, wrap, exhaustion and random invariants passed");
}
