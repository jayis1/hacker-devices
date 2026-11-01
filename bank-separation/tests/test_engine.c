/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
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
/* Independent graph oracle uses an edge list, not firmware bit masks. */
static const unsigned edges[3][2]={{0,1},{1,2},{2,3}};
static bool conflict(unsigned a,unsigned b)
{
    for(unsigned i=0;i<3;++i)
        if((edges[i][0]==a && edges[i][1]==b) ||
           (edges[i][0]==b && edges[i][1]==a)) return true;
    return false;
}
static unsigned words;
static void enumerate(Ledger state, unsigned used[4], bool busy[4], unsigned depth)
{
    if (!depth || state.halted) return;
    for (unsigned op=0;op<8;++op) {
        Ledger next=state;
        unsigned counts[4]; bool occupied[4];
        memcpy(counts,used,sizeof counts);
        memcpy(occupied,busy,sizeof occupied);
        unsigned tag=op%4, reason=0;
        if (op<4) {
            if (occupied[tag]) reason=BS_DUPLICATE;
            else {
                for(unsigned j=0;j<4;++j)
                    if(occupied[j] && conflict(j,tag)) reason=BS_CONFLICT;
                if(!reason) { occupied[tag]=true; ++counts[tag]; }
            }
        } else {
            if(!occupied[tag]) reason=BS_ORPHAN;
            else occupied[tag]=false;
        }
        clock_ms=next.now;
        operation(&next,tag,op<4);
        assert(next.faults==reason);
        for(unsigned j=0;j<4;++j) {
            assert(next.uses[j]==counts[j]);
            assert(!!(next.active&(1u<<j))==occupied[j]);
        }
        ++words;
        enumerate(next,counts,occupied,depth-1);
    }
}
int main(void)
{
    Ledger s; start(&s);
    unsigned used[4]={0}; bool busy[4]={0};
    enumerate(s,used,busy,7);
    /* Exercise every ordered pair, including duplicate precedence. */
    for(unsigned a=0;a<4;++a) for(unsigned b=0;b<4;++b) {
        start(&s); operation(&s,a,true); operation(&s,b,true);
        assert(s.faults==(a==b?BS_DUPLICATE:conflict(a,b)?BS_CONFLICT:0));
    }
    /* Nontransitivity: 0 and 2 may coexist although both conflict with 1. */
    start(&s); operation(&s,0,true); operation(&s,2,true);
    assert(!s.halted && s.active==5 && s.peak==2);
    operation(&s,0,false); operation(&s,0,true);
    assert(!s.halted && s.uses[0]==2); /* No fairness constraint. */
    operation(&s,1,true); assert(s.faults==BS_CONFLICT && s.active==5);
    /* Releasing the actual blocker permits admission; releasing another doesn't. */
    start(&s); operation(&s,0,true); operation(&s,3,true);
    operation(&s,0,false); operation(&s,1,true); assert(!s.halted && s.active==10);
    start(&s); operation(&s,0,true); operation(&s,3,true);
    operation(&s,3,false); operation(&s,1,true); assert(s.faults==BS_CONFLICT);
    start(&s); hold(&s,12,6); assert(s.faults==BS_INTERFACE && s.rejected==2);
    start(&s); operation(&s,2,false); assert(s.faults==BS_ORPHAN);
    start(&s); ledger_sample(&s,s.now+2,0); assert(s.faults==BS_GAP && ledger_valid(&s));
    start(&s); s.observed=UINT32_MAX; hold(&s,0,1);
    assert(s.faults==BS_EXHAUSTED && s.launched==0);
    start(&s); hold(&s,0,12); hold(&s,4,3); s.observed=UINT32_MAX;
    hold(&s,4,1);
    assert(s.faults==BS_EXHAUSTED && !s.active && !s.uses[0] && !s.attempts);
    start(&s); hold(&s,4,3); hold(&s,12,1);
    assert(s.faults==BS_INTERFACE && !s.launched);
    start(&s); hold(&s,4,3); hold(&s,5,1);
    assert(s.faults==BS_INTERFACE && !s.launched);
    uint16_t checksum=0;
    assert(crc16_buffer("123456789",9,&checksum) && checksum==0x29b1);
    assert(crc16_verify("123456789",9,0x29b1));
    assert(!crc16_buffer(NULL,1,&checksum));
    assert(!crc16_buffer("x",1,NULL));
    start(&s); Ledger old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    s.now=UINT32_MAX; clock_ms=UINT32_MAX; hold(&s,0,1); assert(!s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s)); assert(!wire_status(NULL,100,&s));
    s.uses[1]++; assert(!ledger_valid(&s) && !wire_status(out,sizeof out,&s));
    /* Individually consistent counts cannot legalize a forbidden active pair. */
    start(&s); operation(&s,0,true); operation(&s,2,true);
    s.active=3; s.uses[1]=1; s.uses[2]=0;
    assert(!ledger_valid(&s));
    printf("engine: %u graph-oracle transitions; all pairs, reuse, release, faults PASS\n",words);
}
