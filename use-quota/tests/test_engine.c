/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static unsigned words;
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, ++clock_ms, raw);
        assert(ledger_valid(s));
    }
}
static void start(Ledger *s)
{
    clock_ms=0;
    ledger_init(s);
    hold(s,0,20);
}
static void operation(Ledger *s,unsigned tag,bool alloc)
{
    hold(s,tag,12);
    hold(s,tag|(alloc?4u:8u),6);
    hold(s,tag,6);
}
/* Independent remaining-ticket oracle: implementation stores used counts. */
static void enumerate(Ledger state, unsigned remaining[4], bool busy[4], unsigned depth)
{
    if (!depth || state.halted) return;
    for (unsigned op=0;op<8;++op) {
        Ledger next=state;
        unsigned tickets[4]; bool occupied[4];
        memcpy(tickets,remaining,sizeof tickets);
        memcpy(occupied,busy,sizeof occupied);
        unsigned tag=op%4, reason=0;
        if (op<4) {
            if (occupied[tag]) reason=UQ_DUPLICATE;
            else if (!tickets[tag]) reason=UQ_QUOTA;
            else { occupied[tag]=true; --tickets[tag]; }
        } else {
            if(!occupied[tag]) reason=UQ_ORPHAN;
            else occupied[tag]=false;
        }
        clock_ms=next.now;
        operation(&next,tag,op<4);
        assert(next.faults==reason);
        for(unsigned j=0;j<4;++j) {
            assert(next.uses[j]==j+1-tickets[j]);
            assert(!!(next.active&(1u<<j))==occupied[j]);
        }
        ++words;
        enumerate(next,tickets,occupied,depth-1);
    }
}
int main(void)
{
    Ledger s; start(&s);
    unsigned tickets[4]={1,2,3,4}; bool busy[4]={0};
    enumerate(s,tickets,busy,8);
    /* All quotas are inclusive; release and drain never refund tickets. */
    for(unsigned tag=0;tag<4;++tag) {
        start(&s);
        for(unsigned n=0;n<=tag;++n) {
            operation(&s,tag,true);
            assert(!s.halted && s.uses[tag]==n+1);
            operation(&s,tag,false);
            assert(!s.halted && s.active==0 && s.uses[tag]==n+1);
        }
        hold(&s,0,100);
        operation(&s,tag,true);
        assert(s.faults==UQ_QUOTA && !s.active && s.uses[tag]==tag+1);
        Ledger saved=s; operation(&s,3,true);
        saved.now=s.now; assert(!memcmp(&s,&saved,sizeof s));
    }
    /* Arbitrary pair overlap and arbitrary retirement, including all four. */
    for(unsigned a=0;a<4;++a) for(unsigned b=0;b<4;++b) {
        start(&s); operation(&s,a,true); operation(&s,b,true);
        assert(s.faults==(a==b?UQ_DUPLICATE:0));
    }
    start(&s);
    for(unsigned i=0;i<4;++i) operation(&s,i,true);
    assert(s.active==15 && s.peak==4);
    for(unsigned i=4;i--;) operation(&s,i,false);
    assert(!s.halted && !s.active && s.rounds==1);
    /* Duplicate has precedence even when the incumbent spent its last ticket. */
    start(&s); operation(&s,0,true); operation(&s,0,true);
    assert(s.faults==UQ_DUPLICATE);
    start(&s); hold(&s,12,6); assert(s.faults==UQ_INTERFACE && s.rejected==2);
    start(&s); operation(&s,2,false); assert(s.faults==UQ_ORPHAN);
    start(&s); ledger_sample(&s,s.now+2,0); assert(s.faults==UQ_GAP && ledger_valid(&s));
    start(&s); s.observed=UINT32_MAX; hold(&s,0,1);
    assert(s.faults==UQ_EXHAUSTED && s.launched==0);
    start(&s); hold(&s,0,12); hold(&s,4,3); s.observed=UINT32_MAX;
    hold(&s,4,1);
    assert(s.faults==UQ_EXHAUSTED && !s.active && !s.uses[0] && !s.attempts);
    start(&s); hold(&s,4,3); hold(&s,12,1);
    assert(s.faults==UQ_INTERFACE && !s.launched);
    start(&s); hold(&s,4,3); hold(&s,5,1);
    assert(s.faults==UQ_INTERFACE && !s.launched);
    uint16_t checksum=0;
    assert(crc16_buffer("123456789",9,&checksum) && checksum==0x29b1);
    assert(!crc16_buffer(NULL,1,&checksum));
    start(&s); Ledger old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    s.now=UINT32_MAX; clock_ms=UINT32_MAX; hold(&s,0,1); assert(!s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s)); assert(!wire_status(NULL,100,&s));
    start(&s); operation(&s,0,true); operation(&s,0,false);
    /* Balanced accounting cannot excuse a quota overflow in a forged snapshot. */
    s.uses[0]=2; s.launched=2; s.completed=2; s.rounds=2; s.attempts=4;
    assert(!ledger_valid(&s) && !wire_status(out,sizeof out,&s));
    assert(ledger_quota(4)==0);
    printf("engine: %u ticket-oracle transitions; quota boundaries and faults PASS\n",words);
}
