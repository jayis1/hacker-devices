/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Ledger s;
static unsigned t;
static void step(unsigned raw)
{
    ledger_sample(&s,++t,raw);
    assert(ledger_valid(&s));
}
static void hold(unsigned raw,unsigned n) { while(n--) step(raw); }
static void init(void) { ledger_init(&s); t=0; }
int main(void)
{
    init(); hold(0,3); assert(!s.ready); step(0); assert(s.ready);
    hold(1,4); hold(5,3); assert(!s.starts); step(5); assert(s.accepted==1);
    /* Startup collision must be visible even with an unqualified request. */
    init(); for(unsigned i=0;i<30;++i) step(12u|(i&1u));
    assert(!s.ready && s.overlaps==1 && !s.starts && s.collision_active);
    hold(0,4); assert(!s.collision_active && s.ready);
    /* Request chatter must not hide a grant's independent qualification. */
    init(); hold(1,8);
    for(unsigned i=0;i<30;++i) step(5u|((i&1u)<<1));
    assert(s.accepted==1 && s.starts==1);
    /* Simultaneous high arrival or withdrawal both veto low selection. */
    init(); hold(2,8); hold(11,8); assert(s.inversions==1);
    init(); hold(3,8); hold(10,8); assert(s.inversions==1);
    /* Prior low request withdrawal at admission is an orphan first. */
    init(); hold(3,8); hold(9,8); assert(s.orphans==1 && !s.inversions);
    /* Request withdrawal after admission and same-side reacquisition. */
    init(); hold(2,8); hold(10,8); hold(8,8); assert(s.accepted==1 && !s.orphans);
    hold(2,8); hold(10,8); assert(s.accepted==2);
    /* Qualifying a post-gap held grant cannot manufacture admission. */
    init(); hold(1,8); hold(5,8);
    ledger_sample(&s,t+50,5); t+=50; hold(5,8);
    assert(!s.ready && s.starts==1 && s.gaps==1 && s.missed==49);
    hold(1,8); hold(5,8); assert(s.baselines==2 && s.accepted==2);
    /* Random raw samples, duplicate calls and gaps preserve every invariant. */
    init(); unsigned rng=1234567u;
    for(unsigned i=0;i<100000;++i) {
        rng=rng*1664525u+1013904223u;
        if(i%503u==0) t+=2;
        step((rng>>24)&15u);
        if(i%17u==0) ledger_sample(&s,t,15);
        assert(ledger_valid(&s));
    }
    uint16_t crc;
    assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1u);
    assert(crc16_verify("123456789",9,0x29b1u));
    assert(!crc16_buffer(NULL,1,&crc) && !crc16_buffer("x",1,NULL));
    assert(!wire_status(NULL,768,&s));
    char frame[768]; size_t n=wire_status(frame,sizeof frame,&s);
    assert(n && frame[n-1]=='\n' && !memcmp(frame,"AP1,S,",6));
    assert(wire_status(frame,n,&s)==n && !wire_status(frame,n-1,&s));
    Ledger bad=s; bad.starts++; assert(!ledger_valid(&bad));
    bad=s; bad.inversions=bad.low+1; assert(!ledger_valid(&bad));
    bad=s; bad.ready=2; assert(!ledger_valid(&bad));
    puts("edges: exact qualification, startup overlap, simultaneous boundary rules, 100000-sample fuzz passed");
}
