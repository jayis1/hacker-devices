/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "isolation.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void tick(Isolation *s, unsigned levels, unsigned count)
{
    while (count--) isolation_sample(s, ++now, levels);
}
static void ready(Isolation *s)
{
    now=0;
    isolation_init(s);
    isolation_sample(s,now,0);
    tick(s,0,IW_DEAD_MS);
    assert(s->state==IW_READY);
}
int main(void)
{
    Isolation s;
    ready(&s);
    tick(&s,5,1); /* request A + enable A */
    tick(&s,0,1);
    tick(&s,0,20);
    tick(&s,10,1);
    assert(s.switches==1 && s.early==0 && s.minimum==20);
    ready(&s);
    tick(&s,5,1);
    tick(&s,0,1);
    tick(&s,0,19);
    tick(&s,10,1);
    assert(s.switches==1 && s.early==1 && s.minimum==19);
    ready(&s);
    tick(&s,5,1);
    tick(&s,10,1);
    assert(s.early==1 && s.minimum==0);
    tick(&s,15,4);
    assert(s.overlaps==1 && s.overlap_ms==3);
    tick(&s,15,2);
    assert(s.overlaps==1 && s.overlap_ms==5);
    tick(&s,12,1);
    assert(s.unrequested==2);
    tick(&s,12,5);
    assert(s.unrequested==2);
    tick(&s,15,1);
    tick(&s,12,1);
    assert(s.unrequested==4);
    uint32_t saved=s.switches;
    isolation_sample(&s,now+3,10); now+=3;
    assert(s.gaps==1 && s.state==IW_BASELINE && s.switches==saved);
    tick(&s,10,30);
    assert(s.state==IW_BASELINE);
    tick(&s,0,21);
    assert(s.state==IW_READY);
    tick(&s,5,1);
    assert(s.switches==saved); /* unknown past is not a switch */
    ready(&s);
    s.now=UINT32_MAX; now=UINT32_MAX;
    tick(&s,5,1);
    assert(s.gaps==0);
    isolation_sample(&s,now,15);
    assert(s.levels==5); /* same-tick port changes wait for next sample */
    ready(&s);
    s.overlaps=UINT32_MAX;
    tick(&s,15,1);
    assert(s.state==IW_HALTED && (s.faults & IW_EXHAUSTED));
    ready(&s);
    tick(&s,4,1);
    assert(s.unrequested==1 && (s.faults & IW_UNREQUESTED));
    char frame[384];
    assert(wire_status(frame,sizeof frame,&s)>0);
    assert(strncmp(frame,"SW1,S,",6)==0);
    assert(wire_status(frame,4,&s)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(isolation_valid(&s));
    s.switches=1; s.histogram[3]=1; s.minimum=20;
    assert(!isolation_valid(&s));
    assert(wire_status(frame,sizeof frame,&s)==0);
    s.minimum=100;
    assert(isolation_valid(&s));
    s.quiet=UINT32_MAX-1u;
    s.levels=0; s.last_owner=1;
    tick(&s,0,1);
    assert(s.quiet==UINT32_MAX-1u);
    tick(&s,10,1);
    assert(s.switches==2 && isolation_valid(&s));
    /* Deterministic mixed activity checks invariants on every sample. */
    ready(&s);
    uint32_t random=1234567;
    for (unsigned i=0;i<100000;++i) {
        random=random*1664525u+1013904223u;
        now += (random & 255u)==0 ? 3u : 1u;
        isolation_sample(&s,now,(uint8_t)(random >> 28));
        assert(isolation_valid(&s));
    }
    puts("PASS boundary, overlap, request, gap, wrap, saturation, encoder");
    return 0;
}
