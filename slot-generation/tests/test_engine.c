/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while(n--) {
        ledger_sample(s,s->now+1,(uint8_t)raw);
        assert(ledger_valid(s));
    }
}
static void operation(Ledger *s,unsigned slot,unsigned generation,bool allocate)
{
    unsigned tag=slot|(generation<<1);
    hold(s,tag,12);
    hold(s,tag|(allocate?4:8),8);
    hold(s,tag,8);
}
static Ledger baseline(void)
{
    Ledger s;
    ledger_init(&s);
    hold(&s,0,20);
    assert(s.phase==SG_TRACK);
    return s;
}
/* Independent oracle uses arrays, not the packed evidence representation. */
static unsigned paths;
static void explore(Ledger s, bool live[2], int last[2], unsigned depth)
{
    if(!depth) return;
    for(unsigned slot=0;slot<2;slot++)
    for(unsigned gen=0;gen<2;gen++)
    for(unsigned alloc=0;alloc<2;alloc++) {
        Ledger next=s;
        unsigned fault=0;
        if(alloc) {
            if(live[slot]) fault=SG_DUPLICATE;
            else if(last[slot]==(int)gen) fault=SG_GENERATION;
        } else {
            if(!live[slot]) fault=SG_ORPHAN;
            else if(last[slot]!=(int)gen) fault=SG_GENERATION;
        }
        operation(&next,slot,gen,alloc);
        assert(next.faults==fault);
        ++paths;
        if(!fault) {
            bool l[2]={live[0],live[1]};
            int g[2]={last[0],last[1]};
            l[slot]=alloc;
            if(alloc) g[slot]=(int)gen;
            for(unsigned i=0;i<2;i++) {
                assert(!!(next.active&(1u<<i))==l[i]);
                assert(!!(next.order&(4u<<i))==(g[i]>=0));
                if(g[i]>=0) assert(((next.order>>i)&1u)==(unsigned)g[i]);
            }
            explore(next,l,g,depth-1);
        } else {
            assert(next.active==s.active && next.order==s.order);
        }
    }
}
int main(void)
{
    Ledger s=baseline();
    bool live[2]={false,false}; int last[2]={-1,-1};
    explore(s,live,last,6);
    operation(&s,0,0,true); operation(&s,0,0,false);
    operation(&s,0,1,true); operation(&s,0,0,false);
    assert(s.faults==SG_GENERATION && s.active==1 && s.completed==1);
    Ledger frozen=s; hold(&s,15,50);
    frozen.now=s.now; assert(!memcmp(&s,&frozen,sizeof s));
    s=baseline(); operation(&s,1,1,true); operation(&s,1,1,false);
    operation(&s,1,1,true); assert(s.faults==SG_GENERATION);
    s=baseline(); operation(&s,0,0,true); operation(&s,1,1,true);
    operation(&s,0,0,false); operation(&s,1,1,false);
    assert(!s.faults && s.rounds==1 && s.queued==1 && s.peak==2);
    s=baseline(); hold(&s,12,8); assert(s.faults==SG_INTERFACE && s.rejected==2);
    s=baseline(); ledger_sample(&s,s.now+3,0);
    assert(s.faults==SG_GAP && s.missed==2 && ledger_valid(&s));
    s=baseline(); Ledger same=s; ledger_sample(&s,s.now,15);
    assert(!memcmp(&s,&same,sizeof s));
    s.now=UINT32_MAX; ledger_sample(&s,0,0); assert(!s.faults);
    s=baseline(); s.observed=UINT32_MAX; ledger_sample(&s,s.now+1,0);
    assert(s.faults==SG_EXHAUSTED && s.observed==UINT32_MAX);
    s=baseline(); hold(&s,4,2); hold(&s,0,10); assert(!s.attempts);
    hold(&s,4,8); assert(s.launched==1); hold(&s,4,30); assert(s.launched==1);
    s=baseline(); hold(&s,6,8); assert(s.faults==SG_INTERFACE);
    char out[768]; size_t n=wire_status(out,sizeof out,&s); assert(n);
    assert(!wire_status(out,n-1,&s)); assert(!wire_status(NULL,sizeof out,&s));
    assert(wire_boot(out,sizeof out,0)); assert(wire_policy(out,sizeof out));
    uint16_t crc; assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1);
    assert(!crc16_buffer(NULL,1,&crc)); assert(!crc16_buffer("",0,NULL));
    s=baseline(); s.active=1; assert(!ledger_valid(&s));
    s=baseline(); s.order=1; assert(!ledger_valid(&s));
    printf("engine: %u oracle branches, stale generation, wrap, gaps, bounds PASS\n",paths);
}
