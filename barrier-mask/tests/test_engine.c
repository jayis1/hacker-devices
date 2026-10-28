/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Barrier *s, unsigned raw, unsigned n)
{
    while (n--) {
        barrier_sample(s, ++tick, raw);
        assert(barrier_valid(s));
    }
}
static void start(Barrier *s)
{
    tick=0; barrier_init(s); hold(s,0,30); assert(s->ready);
}
static void arrive(Barrier *s,unsigned tag)
{
    hold(s,tag,12); hold(s,tag|4,8); hold(s,tag,8);
}
static void release_round(Barrier *s)
{
    hold(s,8,8); hold(s,0,30);
}
static void permutations(void)
{
    for(unsigned a=0;a<4;++a) for(unsigned b=0;b<4;++b)
    for(unsigned c=0;c<4;++c) for(unsigned d=0;d<4;++d) {
        if(a==b||a==c||a==d||b==c||b==d||c==d) continue;
        Barrier s; start(&s);
        unsigned tags[]={a,b,c,d};
        for(unsigned round=0;round<3;++round) {
            unsigned oracle=0;
            for(unsigned i=0;i<4;++i) {
                arrive(&s,tags[i]); oracle|=1u<<tags[i];
                assert(s.mask==oracle && s.active && s.ready);
            }
            release_round(&s);
            assert(s.completed==round+1 && !s.mask && s.ready);
            assert(s.accepted==4*(round+1) && !s.faults);
        }
    }
}
static void errors(void)
{
    Barrier s; start(&s); arrive(&s,2); arrive(&s,2);
    assert(s.duplicate==1 && s.arrivals==2 && s.accepted==1);
    assert(s.abandoned==1 && s.discarded==1 && !s.ready);
    hold(&s,0,100); assert(!s.ready);
    start(&s); release_round(&s); assert(s.early==1 && s.releases==1);
    start(&s); arrive(&s,0); arrive(&s,1); arrive(&s,2);
    hold(&s,3,12); hold(&s,15,8);
    assert(!s.completed && s.collision==1 && s.arrivals==4 && s.releases==1);
    start(&s); arrive(&s,0); arrive(&s,1); arrive(&s,2);
    hold(&s,3,12); hold(&s,7,8); hold(&s,15,8);
    assert(s.collision==1 && !s.completed); /* Held arrival overlaps release. */
    start(&s); hold(&s,5,4); assert(s.unstable==1);
    start(&s); arrive(&s,0); tick+=4; hold(&s,0,1);
    assert(s.gaps==1 && s.missed==4 && s.discarded==1 && !s.ready);
}
static void deadlines(void)
{
    Barrier s;
    for(unsigned age=999;age<=1001;++age) {
        start(&s);
        for(unsigned i=0;i<4;++i) arrive(&s,i);
        while(s.age<age-4) hold(&s,0,1);
        hold(&s,8,4);
        assert(s.age==age);
        assert(s.completed==(age<=1000));
        assert(s.timeouts==(age>1000));
        assert(s.releases==1); /* Fault-tick edges are not silently omitted. */
    }
    start(&s); arrive(&s,0);
    while(s.age<1001) hold(&s,0,1);
    assert(s.timeouts==1 && s.abandoned==1);
}
static void noise_and_limits(void)
{
    Barrier s; start(&s);
    for(unsigned stable=4;stable<=6;++stable) {
        start(&s);
        hold(&s,1,4); /* Tag qualifies on fourth observation. */
        hold(&s,1,stable-4);
        hold(&s,5,4);
        assert(s.accepted==(stable>=5));
        assert(s.unstable==(stable<5));
    }
    start(&s); arrive(&s,0);
    s.arrivals=UINT32_MAX;
    hold(&s,1,12); hold(&s,5,4);
    assert(s.halted && s.abandoned==1 && s.discarded==1);
    assert(s.accepted==1 && s.rounds==1 && !s.active);
    start(&s);
    hold(&s,4,3); hold(&s,0,8); assert(!s.arrivals);
    arrive(&s,0); hold(&s,4,80); assert(s.duplicate==1);
    start(&s); tick=UINT32_MAX-15; s.now=tick;
    hold(&s,0,30); assert(s.ready && !s.gaps);
    s.arrivals=UINT32_MAX;
    /* Counter exhaustion rolls back the entire sample. */
    hold(&s,4,4); assert(s.halted && (s.faults&BM_EXHAUSTED));
    assert(s.arrivals==UINT32_MAX && !s.active && !s.rounds);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,5,&s)); assert(!wire_status(NULL,0,&s));
    start(&s); s.mask=1; assert(!barrier_valid(&s));
    start(&s); s.completed=1; assert(!wire_status(out,sizeof out,&s));
    start(&s); s.faults=BM_EARLY; assert(!barrier_valid(&s));
    uint32_t rng=1;
    start(&s);
    for(unsigned i=0;i<20000;++i) {
        rng=rng*1664525u+1013904223u;
        hold(&s,(rng>>24)&15,1);
    }
}
int main(void)
{
    permutations(); errors(); deadlines(); noise_and_limits();
    puts("engine: permutations, repeated rounds, faults, deadlines, conservation, wrap and noise passed");
}
