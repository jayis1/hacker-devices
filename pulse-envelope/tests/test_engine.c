/* Author: jayis1. Synthetic policy regression; not physical evidence. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Witness w;
static uint32_t tick;
static void step(uint8_t raw, unsigned n)
{
    while(n--) {
        witness_sample(&w, ++tick, raw);
        assert(witness_consistent(&w));
    }
}
static void init(void)
{
    tick=0; witness_init(&w); witness_sample(&w,tick,0);
    step(0,60);
}
static void pulse(unsigned width)
{
    step(1,width); step(0,4);
}
int main(void)
{
    init(); pulse(10);
    assert(w.channel[0].completed==1 && w.channel[0].last==10);
    assert(!w.channel[0].shorts && !w.channel[0].longs && !w.channel[0].rapid);
    init(); pulse(9); assert(w.channel[0].shorts==1);
    init(); pulse(200); assert(!w.channel[0].longs);
    init(); pulse(201); assert(w.channel[0].longs==1 && w.channel[0].last==201);
    init(); pulse(1000); assert(w.channel[0].longs==1 && w.channel[0].peak==201);
    init(); step(1,20); step(0,49); step(1,4); assert(w.channel[0].rapid==1);
    init(); step(1,20); step(0,50); step(1,4); assert(!w.channel[0].rapid);
    init(); step(1,20); witness_sample(&w,tick+=2,1);
    assert(w.channel[0].aborted==1 && w.channel[0].phase==0);
    step(1,100); assert(w.channel[0].phase==0);
    step(0,60); pulse(20); assert(w.channel[0].completed==1);
    witness_init(&w); tick=0; witness_sample(&w,0,15); step(15,300);
    for(unsigned i=0;i<4;++i) assert(w.channel[i].starts==0);
    step(0,60); step(15,20); step(0,4);
    for(unsigned i=0;i<4;++i) assert(w.channel[i].completed==1);
    init(); step(1,4);
    for(unsigned i=0;i<250;++i) step((uint8_t)(1 | ((i&1)<<1)),1);
    assert(w.channel[0].longs==1 && w.channel[1].starts==0);
    init(); step(1,2); step(0,10); assert(w.channel[0].starts==0);
    init(); uint32_t old=w.now; witness_sample(&w,old,15);
    assert(w.now==old && !w.channel[0].starts);
    init(); tick=UINT32_MAX-2; w.now=tick; step(0,10); assert(!w.gaps);
    init(); w.channel[0].starts=UINT32_MAX; w.channel[0].completed=UINT32_MAX;
    step(1,4); assert(w.halted && w.channel[0].starts==UINT32_MAX);
    assert(witness_consistent(&w));
    char frame[768]; assert(wire_status(frame,sizeof frame,&w));
    assert(!wire_status(frame,5,&w));
    init(); unsigned seed=47;
    for(unsigned i=0;i<100000;++i) {
        seed=seed*1664525u+1013904223u;
        step((uint8_t)(seed>>28),1);
    }
    assert(wire_status(frame,sizeof frame,&w));
    puts("engine: boundaries, gap conservation, wrap, overflow, chatter and 100000 random ticks passed");
}
