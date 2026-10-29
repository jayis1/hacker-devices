/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, tick++, raw);
        assert(ledger_valid(s));
    }
}
static void baseline(Ledger *s)
{
    ledger_init(s);
    tick=0;
    hold(s,0,5);
    assert(s->ready && s->last==4);
}
int main(void)
{
    Ledger s;
    char output[768];
    /* No correlation is invented by attaching to an already owned resource. */
    ledger_init(&s); tick=0; hold(&s,1,10);
    assert(!s.ready && !s.anchors);
    hold(&s,0,5); hold(&s,1,5);
    assert(s.anchors==1 && s.last==0);
    hold(&s,2,5); hold(&s,4,5); hold(&s,8,5); hold(&s,1,5);
    assert(s.forward==4 && !s.skip && !s.reverse && !s.repeat);
    hold(&s,0,5); hold(&s,1,5);
    assert(s.repeat==1 && (s.faults&BO_REPEAT));
    hold(&s,4,5); assert(s.skip==1);
    hold(&s,2,5); assert(s.reverse==1);
    assert(s.matrix[0]==1 && s.matrix[2]==1 && s.matrix[9]==1);
    /* Every possible directed transfer, including a repeated admission. */
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<4;++j) {
        baseline(&s); hold(&s,1u<<i,5); hold(&s,0,5); hold(&s,1u<<j,5);
        assert(s.anchors==1 && s.matrix[i*4+j]==1);
        assert(s.forward==((j-i+4)%4==1));
        assert(s.skip==((j-i+4)%4==2));
        assert(s.reverse==((j-i+4)%4==3));
        assert(s.repeat==(j==i));
    }
    /* Multi-hot invalidity persists across different invalid codes. */
    baseline(&s); hold(&s,1,5); hold(&s,3,5); hold(&s,6,5);
    assert(s.collisions==1 && !s.ready && s.last==4);
    hold(&s,8,5); assert(!s.ready && s.anchors==1);
    hold(&s,0,5); hold(&s,4,5); assert(s.anchors==2);
    /* Gaps drop phase but preserve established totals; zero reanchors. */
    tick+=8; hold(&s,8,5);
    assert(s.gaps==1 && s.missed==8 && !s.ready && !s.forward);
    hold(&s,0,5); hold(&s,8,5); assert(s.anchors==3);
    /* All masks and held-input immunity. */
    for(unsigned mask=0;mask<16;++mask) {
        baseline(&s); hold(&s,mask,25);
        unsigned ones=0; for(unsigned i=0;i<4;++i) ones+=(mask>>i)&1;
        assert(s.collisions==(ones>1));
        assert(s.anchors==(ones==1));
        assert(!s.forward && !s.repeat);
    }
    /* Qualification rejects a one-tick excursion. */
    baseline(&s); hold(&s,1,5); hold(&s,0,1); hold(&s,1,5);
    assert(!s.repeat && s.anchors==1);
    /* A repeated timestamp cannot advance a run. */
    Ledger before=s; ledger_sample(&s,s.now,15);
    assert(!memcmp(&s,&before,sizeof s));
    /* Rollover is continuous, not an acquisition gap. */
    baseline(&s); tick=UINT32_MAX-2; s.now=tick-1;
    hold(&s,0,8); assert(!s.gaps);
    /* Transactional overflow rolls back the matrix and aggregate together. */
    baseline(&s); hold(&s,1,5);
    s.forward=UINT32_MAX; s.matrix[1]=UINT32_MAX;
    assert(ledger_valid(&s)); hold(&s,2,5);
    assert(s.halted && s.forward==UINT32_MAX && s.matrix[1]==UINT32_MAX);
    assert(s.faults&BO_EXHAUSTED);
    assert(wire_status(output,sizeof output,&s)>0);
    assert(!wire_status(output,2,&s));
    s.matrix[1]--; assert(!ledger_valid(&s));
    assert(!wire_status(output,sizeof output,&s));
    /* Independent low channel chatter cannot hide a qualified collision. */
    baseline(&s);
    for(unsigned i=0;i<40;++i) hold(&s,3|((i&1)<<3),1);
    assert(s.collisions==1);
    /* Startup chatter prevents one channel qualifying; independent collision
       episodes must still close and reopen without an invalid snapshot. */
    ledger_init(&s); tick=0;
    for(unsigned phase=0;phase<3;++phase) {
        for(unsigned i=0;i<20;++i)
            hold(&s,(phase==1 ? 1u : 3u)|((i&1u)<<3),1);
        assert(s.valid==7);
        assert(s.collisions==(phase==2 ? 2u : 1u));
        assert(s.collision_active==(phase!=1));
    }
    /* Arbitrary raw chatter and gaps must never invalidate serialization. */
    baseline(&s); uint32_t noise=29;
    for(unsigned k=0;k<50000;++k) {
        noise=noise*1664525u+1013904223u;
        if((noise&255u)==0) tick+=3;
        hold(&s,noise>>28,1);
        assert(wire_status(output,sizeof output,&s)>0);
    }
    /* Deterministic oracle over fully settled transfers. */
    baseline(&s); unsigned previous=0; hold(&s,1,5);
    uint32_t expected[16]={0}, rng=7;
    for(unsigned k=0;k<10000;++k) {
        rng=rng*1664525u+1013904223u;
        unsigned owner=rng>>30;
        hold(&s,0,5); hold(&s,1u<<owner,5);
        ++expected[previous*4+owner]; previous=owner;
        for(unsigned i=0;i<16;++i) assert(s.matrix[i]==expected[i]);
    }
    puts("engine: all 16 transfers, masks, qualification, gaps, overflow, wrap and 10000-turn oracle passed");
}
