/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while(n--) { ledger_sample(s,s->now+1,(uint8_t)raw); assert(ledger_valid(s)); }
}
int main(void)
{
    Ledger s; char out[768];
    ledger_init(&s);
    hold(&s,1,20); assert(s.state==0); /* mid-band never invents memory */
    hold(&s,0,20); assert(s.state==1 && !s.mismatch);
    hold(&s,1,20); assert(s.state==1 && !s.mismatch);
    hold(&s,7,20); assert(s.state==2 && !s.mismatch);
    hold(&s,5,20); assert(s.state==2 && !s.mismatch);
    hold(&s,1,20); assert(s.state==2 && s.mismatch && s.bad_high);
    hold(&s,5,20); assert(!s.mismatch && s.episodes==1);
    hold(&s,0,20); assert(s.state==1 && !s.mismatch);
    hold(&s,4,20); assert(s.bad_low && s.episodes==2);
    hold(&s,8,20); assert(s.state==0); /* inhibit disclaims evidence */
    hold(&s,1,20); assert(s.state==0);
    hold(&s,3,4); assert(s.state==2 && s.settle==10);
    hold(&s,3,9); assert(!s.mismatch);
    hold(&s,3,1); assert(s.mismatch);
    unsigned old=s.episodes;
    ledger_sample(&s,s.now+3,1); assert(s.gaps==1 && s.state==0);
    hold(&s,1,30); assert(!s.state && s.episodes==old);
    assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,2,&s));
    s.bad_high=UINT32_MAX; assert(!ledger_valid(&s));
    ledger_init(&s); s.now=UINT32_MAX-8;
    hold(&s,0,20); assert(s.state==1 && !s.gaps);
    s.coverage=UINT32_MAX; s.good=UINT32_MAX;
    hold(&s,0,1); assert(s.halted && (s.faults&TM_EXHAUSTED));
    assert(wire_status(out,sizeof out,&s));
    /* Independent oracle for plateau sequences, including all four levels. */
    for(unsigned pattern=0;pattern<256;++pattern) {
        ledger_init(&s); unsigned expected=0;
        for(unsigned i=0;i<4;++i) {
            unsigned level=(pattern>>(2*i))&3;
            if(level==0) expected=1;
            if(level==3) expected=2;
            hold(&s,level | (expected==2?4:0),20);
            assert(s.state==expected && !s.mismatch);
        }
    }
    /* Output chatter must not postpone qualification of the level bus. */
    ledger_init(&s); hold(&s,0,20);
    for(unsigned i=0;i<30;++i) hold(&s,3|((i&1)?4:0),1);
    assert(s.state==2 && s.bad_high);
    /* Deterministic arbitrary-signal stress with sparse acquisition gaps. */
    uint32_t rng=17;
    ledger_init(&s);
    for(unsigned i=0;i<100000;++i) {
        rng=rng*1664525u+1013904223u;
        ledger_sample(&s,s.now+((i%997)==0?4:1),(uint8_t)(rng>>28));
        assert(ledger_valid(&s));
    }
    puts("engine: boundaries, 256 sequences, 100000 stress samples passed");
}
