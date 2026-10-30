/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static void hold(Ledger *s, unsigned raw, unsigned ticks)
{
    while (ticks--) {
        ledger_sample(s, s->now + 1, raw);
        assert(ledger_valid(s));
    }
}
static Ledger fresh(void)
{
    Ledger s;
    ledger_init(&s);
    hold(&s, 0, 20);
    assert(s.phase == QO_TRACK);
    return s;
}
static void operation(Ledger *s, unsigned tag, bool enqueue)
{
    hold(s, tag, 20);
    hold(s, tag | (enqueue ? 4u : 8u), 20);
    hold(s, tag, 20);
}
/* Independent list oracle: all operation words through length eight, including
 * repeated tags. Halted prefixes do not need extension: evidence must freeze. */
static unsigned long words;
static void enumerate(Ledger s, const unsigned *queue, unsigned length, unsigned left)
{
    if (!left) return;
    for (unsigned op=0; op<8; ++op) {
        unsigned tag=op&3, next[4], n=length, reason=0;
        memcpy(next,queue,length*sizeof *queue);
        bool present=false;
        for (unsigned i=0;i<length;++i) present |= queue[i]==tag;
        if (op<4) {
            if (present) reason=QO_DUPLICATE;
            else next[n++]=tag;
        } else if (!present) reason=QO_ORPHAN;
        else if (queue[0]!=tag) reason=QO_OVERTAKE;
        else {
            --n;
            memmove(next,next+1,n*sizeof *next);
        }
        Ledger t=s;
        operation(&t,tag,op<4);
        ++words;
        assert(t.faults==reason);
        if (reason) {
            assert(t.active==s.active && t.order==s.order);
            Ledger stopped=t;
            hold(&t,15,10);
            assert(t.attempts==stopped.attempts && t.order==stopped.order);
        } else {
            unsigned mask=0, packed=0;
            for (unsigned i=0;i<n;++i) { mask|=1u<<next[i]; packed|=next[i]<<(2*i); }
            assert(t.active==mask && t.order==packed);
            enumerate(t,next,n,left-1);
        }
    }
}
static void edge_cases(void)
{
    Ledger s=fresh();
    operation(&s,2,true); operation(&s,0,true);
    operation(&s,2,false); operation(&s,2,true);
    assert(s.active==5 && s.order==8 && s.queued==2);
    operation(&s,0,false); operation(&s,2,false);
    assert(s.rounds==1 && !s.active && !s.order && s.peak==2);
    s=fresh(); hold(&s,12,20);
    assert(s.faults==QO_INTERFACE && s.rejected==2);
    s=fresh(); hold(&s,4,3); hold(&s,12,1);
    assert(s.faults==QO_INTERFACE && s.rejected==1);
    s=fresh(); hold(&s,7,4);
    assert(s.faults==QO_INTERFACE); /* tag changes with enqueue */
    s=fresh(); hold(&s,4,3); hold(&s,0,10);
    assert(!s.attempts); /* subqualification pulse is deliberately invisible */
    s=fresh(); hold(&s,4,100);
    assert(s.launched==1 && s.attempts==1); /* held strobe is not repeated */
    for (unsigned setup=QO_SETUP-1; setup<=QO_SETUP; ++setup) {
        s=fresh(); s.stable=setup; s.candidates=4; s.runs[1]=2;
        ledger_sample(&s,s.now+1,4);
        assert(s.faults==(setup<QO_SETUP ? QO_INTERFACE : 0));
        assert(ledger_valid(&s));
    }
    s=fresh(); operation(&s,1,true);
    ledger_sample(&s,s.now+2,0);
    assert(s.faults==QO_GAP && s.missed==1 && s.active==2);
    assert(ledger_valid(&s));
    s=fresh(); uint32_t before=s.observed;
    ledger_sample(&s,s.now,15); assert(s.observed==before);
    s.now=UINT32_MAX; hold(&s,0,1); assert(!s.halted && s.now==0);
    s=fresh(); s.observed=UINT32_MAX;
    hold(&s,0,1); assert(s.faults==QO_EXHAUSTED);
    /* Exhaustion on closing a nonempty queue must roll back every field. */
    s=fresh(); operation(&s,0,true);
    s.launched=UINT32_MAX/2+1; s.completed=s.launched-1;
    s.attempts=UINT32_MAX; s.rounds=s.completed; s.queued=0;
    s.observed=UINT32_MAX-1000; assert(ledger_valid(&s));
    operation(&s,0,false);
    assert(s.faults==QO_EXHAUSTED && s.active==1 && s.completed==UINT32_MAX/2);
    s=fresh(); s.active=3; s.order=0; s.launched=2; s.attempts=2;
    s.queued=1; s.peak=2; assert(!ledger_valid(&s)); /* repeated packed identity */
    s=fresh(); s.launched=2; s.completed=2; s.attempts=4;
    s.rounds=1; s.queued=1; s.peak=1;
    assert(!ledger_valid(&s)); /* joining pending work necessarily reaches two */
}
static void encoding(void)
{
    Ledger s=fresh(); char frame[768]; uint16_t crc;
    assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1);
    assert(!crc16_buffer(NULL,1,&crc));
    assert(wire_status(frame,sizeof frame,&s)>0);
    assert(!wire_status(frame,3,&s));
    assert(!wire_status(NULL,768,&s));
    s.launched=1; assert(!wire_status(frame,sizeof frame,&s));
}
int main(void)
{
    unsigned queue[4]={0};
    enumerate(fresh(),queue,0,8);
    edge_cases(); encoding();
    printf("engine: %lu independently checked operation prefixes; boundaries and encoding PASS\n",words);
    return 0;
}
