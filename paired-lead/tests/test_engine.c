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
/* Independent signed walk oracle. No cumulative-count eligibility formula.
 * Even admission moves a pair right, odd admission moves it left.
 * The permitted walk corridor includes both endpoints -2 and +2. */
static void enumerate(Ledger state, int balance[2], bool busy[4], unsigned depth)
{
    if (!depth || state.halted) return;
    for (unsigned op=0;op<8;++op) {
        Ledger next=state;
        bool occupied[4]; memcpy(occupied,busy,sizeof occupied);
        int walk[2]; memcpy(walk,balance,sizeof walk);
        unsigned tag=op%4, reason=0;
        if (op<4) {
            int step=tag%2 ? -1 : 1;
            if (occupied[tag]) reason=4;
            else if (walk[tag/2]+step>2 || walk[tag/2]+step< -2) reason=1;
            else { occupied[tag]=true; walk[tag/2]+=step; }
        } else {
            if (!occupied[tag]) reason=2;
            else occupied[tag]=false;
        }
        clock_ms=next.now;
        operation(&next,tag,op<4);
        assert(next.faults==reason);
        for (unsigned j=0;j<4;++j)
            assert(!!(next.active&(1u<<j))==occupied[j]);
        for (unsigned p=0;p<2;++p)
            assert((int64_t)next.uses[2*p]-(int64_t)next.uses[2*p+1]==walk[p]);
        ++words;
        enumerate(next,walk,occupied,depth-1);
    }
}
int main(void)
{
    Ledger s; start(&s); bool busy[4]={0}; int balance[2]={0};
    /* Second same-tag lifetime is legal, unlike coverage-gated renewal. */
    operation(&s,0,true); operation(&s,0,false); operation(&s,0,true);
    assert(!s.halted && s.uses[0]==2);
    start(&s); enumerate(s,balance,busy,8);
    for(unsigned tag=0;tag<4;++tag) {
        start(&s);
        for(unsigned i=0;i<2;++i) { operation(&s,tag,true); operation(&s,tag,false); }
        hold(&s,0,100); operation(&s,tag,true);
        assert(s.faults==1 && !s.active && s.uses[tag]==2);
        Ledger saved=s; operation(&s,tag^1u,true);
        saved.now=s.now; assert(!memcmp(&s,&saved,sizeof s));
        /* Partner progress opens one more lead position; release does not. */
        start(&s);
        for(unsigned i=0;i<2;++i) { operation(&s,tag,true); operation(&s,tag,false); }
        operation(&s,tag^1u,true); operation(&s,tag,true);
        assert(!s.halted && s.uses[tag]==3);
        operation(&s,tag,false); operation(&s,tag,true);
        assert(s.faults==1); /* Busy partner remains relevant. */
        start(&s);
        for(unsigned i=0;i<10;++i) {
            operation(&s,tag,true); operation(&s,tag,false);
            operation(&s,tag^1u,true); operation(&s,tag^1u,false);
        }
        assert(!s.halted && s.launched==20); /* Other pair may remain unused. */
    }
    start(&s); operation(&s,0,true); operation(&s,0,true);
    assert(s.faults==4);
    start(&s); operation(&s,0,true); operation(&s,0,false);
    operation(&s,0,true); operation(&s,0,true);
    assert(s.faults==4 && s.uses[0]==2); /* Duplicate before lead. */
    Ledger arithmetic={0};
    arithmetic.uses[0]=UINT32_MAX; arithmetic.uses[1]=UINT32_MAX;
    assert(ledger_eligible(&arithmetic,0)); /* Widen partner+2, no wrap. */
    assert(!ledger_eligible(NULL,0) && !ledger_eligible(&arithmetic,4));
    start(&s); hold(&s,12,6); assert(s.faults==8 && s.rejected==2);
    start(&s); operation(&s,2,false); assert(s.faults==2);
    start(&s); ledger_sample(&s,s.now+2,0); assert(s.faults==16 && ledger_valid(&s));
    start(&s); s.observed=UINT32_MAX; hold(&s,0,1);
    assert(s.faults==32 && s.launched==0);
    start(&s); hold(&s,0,12); hold(&s,4,3); s.observed=UINT32_MAX;
    hold(&s,4,1);
    assert(s.faults==32 && !s.active && !s.uses[0] && !s.attempts);
    start(&s); hold(&s,4,3); hold(&s,12,1); assert(s.faults==8);
    start(&s); hold(&s,4,3); hold(&s,5,1); assert(s.faults==8);
    uint16_t checksum=0;
    assert(crc16_buffer("123456789",9,&checksum) && checksum==0x29b1);
    assert(!crc16_buffer(NULL,1,&checksum));
    start(&s); Ledger old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    s.now=UINT32_MAX; clock_ms=UINT32_MAX; hold(&s,0,1); assert(!s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s)); assert(!wire_status(NULL,100,&s));
    start(&s); operation(&s,0,true); operation(&s,0,false);
    s.uses[0]=3; s.launched=3; s.completed=3; s.rounds=3; s.attempts=6;
    assert(!ledger_valid(&s) && !wire_status(out,sizeof out,&s));
    printf("engine: %u independent signed-walk transitions; lead boundaries PASS\n",words);
}
