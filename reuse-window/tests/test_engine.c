/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static unsigned transitions;
static void hold(Ledger *s,unsigned raw,unsigned n)
{
    while(n--) {
        ledger_sample(s,s->now+1u,raw);
        assert(ledger_valid(s));
    }
}
static void start(Ledger *s)
{
    ledger_init(s); hold(s,0,20);
}
static void op(Ledger *s,unsigned tag,bool alloc)
{
    hold(s,tag,12); hold(s,tag|(alloc?4u:8u),6); hold(s,tag,6);
}
/* Independent oracle stores absolute admission timestamps, not countdowns.
 * It counts events within the half-open window and keeps full short history. */
typedef struct {
    bool live[4];
    unsigned count[4], times[4][16];
} Oracle;
static unsigned recent(const Oracle *o,unsigned tag,unsigned now)
{
    unsigned n=0;
    for(unsigned i=0;i<o->count[tag];i++)
        n+=now-o->times[tag][i]<1000u;
    return n;
}
static void enumerate(Ledger state,Oracle oracle,unsigned depth)
{
    if(!depth || state.halted) return;
    for(unsigned action=0;action<8;action++) {
        Ledger next=state; Oracle expected=oracle;
        unsigned tag=action%4,reason=0;
        unsigned endpoint=next.now+16u;
        if(action<4) {
            if(oracle.live[tag]) reason=4;
            else if(recent(&oracle,tag,endpoint)>=2) reason=1;
            else {
                expected.live[tag]=true;
                expected.times[tag][expected.count[tag]++]=endpoint;
            }
        } else {
            if(!oracle.live[tag]) reason=2;
            else expected.live[tag]=false;
        }
        op(&next,tag,action<4);
        assert(next.faults==reason);
        for(unsigned i=0;i<4;i++) {
            assert(!!(next.active&(1u<<i))==expected.live[i]);
            assert(next.uses[i]==expected.count[i]);
            if(!reason)
                assert(ledger_eligible(&next,i)==(!expected.live[i] && recent(&expected,i,next.now)<2));
        }
        transitions++; enumerate(next,expected,depth-1);
    }
}
int main(void)
{
    Ledger s;
    start(&s); Oracle empty={0}; enumerate(s,empty,7);
    for(unsigned tag=0;tag<4;tag++) {
        for(int offset=-1;offset<=1;offset++) {
            start(&s); op(&s,tag,true); op(&s,tag,false);
            op(&s,tag,true); op(&s,tag,false);
            unsigned wait=s.remaining[tag][0]-16u;
            hold(&s,tag,(unsigned)((int)wait+offset));
            op(&s,tag,true);
            assert(s.faults==(offset<0?1u:0u));
            assert(s.uses[tag]==(offset<0?2u:3u));
        }
    }
    /* A fixed calendar reset would wrongly accept this closely spaced third. */
    start(&s); hold(&s,0,850); op(&s,0,true); op(&s,0,false);
    op(&s,0,true); op(&s,0,false); hold(&s,0,50); op(&s,0,true);
    assert(s.faults==1);
    /* Other tags have independent capacity; release and full drain refund none. */
    start(&s);
    for(unsigned i=0;i<4;i++) {
        op(&s,i,true); op(&s,i,false); op(&s,i,true); op(&s,i,false);
    }
    assert(s.launched==8 && !s.active && !s.halted);
    hold(&s,0,1000);
    for(unsigned i=0;i<4;i++) {
        assert(!s.remaining[i][0] && !s.remaining[i][1]);
        op(&s,i,true);
    }
    assert(s.depth==4 && s.launched==12 && !s.halted);
    /* Chatter does not pause time, even on the tag bundle. */
    start(&s); op(&s,0,true);
    for(unsigned i=0;i<1000;i++) hold(&s,i%2,1);
    assert(!s.remaining[0][1] && !s.halted);
    /* Held occupancy may outlive both historical deadlines. */
    op(&s,0,true); assert(s.faults==4);
    start(&s); op(&s,2,false); assert(s.faults==2);
    start(&s); hold(&s,12,6); assert(s.faults==8 && s.rejected==2);
    start(&s); op(&s,0,true); Ledger old=s;
    ledger_sample(&s,s.now+2,0);
    assert(s.faults==16 && ledger_valid(&s));
    assert(!memcmp(s.remaining,old.remaining,sizeof s.remaining));
    old=s; hold(&s,15,1100); old.now=s.now;
    assert(!memcmp(&s,&old,sizeof s));
    /* Exhaustion rolls back both expiration and a simultaneous admission. */
    start(&s); op(&s,0,true); op(&s,0,false);
    hold(&s,0,12); hold(&s,4,3); s.observed=UINT32_MAX;
    old=s; hold(&s,4,1); assert(s.faults==32);
    assert(s.launched==old.launched && s.attempts==old.attempts);
    assert(!memcmp(s.remaining,old.remaining,sizeof s.remaining));
    /* Last permitted observation increment is still accepted. */
    start(&s); s.observed=UINT32_MAX-1; hold(&s,0,1);
    assert(s.observed==UINT32_MAX && !s.halted);
    hold(&s,0,1); assert(s.faults==32);
    start(&s); hold(&s,4,3); hold(&s,12,1); assert(s.faults==8);
    start(&s); hold(&s,4,3); hold(&s,5,1); assert(s.faults==8);
    start(&s); old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    /* Countdown decrements correctly across UINT32 timestamp rollover. */
    op(&s,0,true); unsigned remain=s.remaining[0][1]; s.now=UINT32_MAX;
    hold(&s,0,1); assert(s.now==0 && s.remaining[0][1]==remain-1 && !s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s) && !wire_status(NULL,100,&s));
    s.remaining[0][0]=1001; assert(!ledger_valid(&s));
    start(&s); s.remaining[0][1]=1; assert(!ledger_valid(&s));
    start(&s); s.halted=1; s.valid=0; s.faults=1; s.rejected=1; s.attempts=1;
    assert(!ledger_valid(&s));
    printf("engine: %u independent timestamp-list transitions PASS\n",transitions);
    puts("engine: boundaries, per-tag independence, rollback, gap, wrap, framing PASS");
}
