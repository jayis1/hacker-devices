/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static uint32_t clock_ms;
static void hold(Ledger *s,unsigned raw,unsigned n)
{
    while(n--) {
        ledger_sample(s,++clock_ms,(uint8_t)raw);
        assert(ledger_valid(s));
    }
}
static void baseline(Ledger *s)
{
    clock_ms=0; ledger_init(s); hold(s,0,20);
    assert(s->phase==RS_TRACK);
}
static void event(Ledger *s,unsigned tag,unsigned edge)
{
    hold(s,tag,12); hold(s,tag|edge,5); hold(s,tag,5);
}
/* All four-event words (including repeated identities), with a separate
 * ownership oracle. Stops at the first fault; does not use engine helpers. */
static unsigned cases;
static void enumerate(unsigned code)
{
    Ledger s; baseline(&s);
    unsigned owners=0,rounds=0;
    for(unsigned step=0;step<4;step++) {
        unsigned op=code%8; code/=8;
        unsigned tag=op%4,bit=1u<<tag,reason=0;
        if(op<4) {
            if(owners&bit) reason=RS_DUPLICATE;
            else if((tag==3 && owners) || (tag!=3 && (owners&8))) reason=RS_CONFLICT;
            else owners|=bit;
        } else {
            if(!(owners&bit)) reason=RS_ORPHAN;
            else { owners&=~bit; if(!owners) rounds++; }
        }
        event(&s,tag,op<4?4:8);
        assert(s.faults==reason);
        if(reason) break;
        assert(s.active==owners && s.rounds==rounds);
    }
    cases++;
}
int main(void)
{
    for(unsigned code=0;code<4096;code++) enumerate(code);
    Ledger s; baseline(&s);
    event(&s,0,4); event(&s,1,4); event(&s,2,4);
    assert(s.active==7 && !s.halted);
    event(&s,0,8); event(&s,0,4); /* legal reentry while peers remain */
    event(&s,1,8); event(&s,0,8); event(&s,2,8);
    assert(s.rounds==1 && s.launched==4 && s.completed==4 && s.seen==0);
    assert(s.shared==3 && s.readers==4 && s.writers==0);
    event(&s,3,4); event(&s,3,8); assert(s.rounds==2 && s.writers==1);
    baseline(&s); event(&s,3,4); event(&s,1,4); assert(s.faults==RS_CONFLICT && s.active==8);
    baseline(&s); event(&s,2,4); event(&s,3,4); assert(s.faults==RS_CONFLICT && s.active==4);
    baseline(&s); event(&s,3,4); event(&s,3,4); assert(s.faults==RS_DUPLICATE);
    baseline(&s); hold(&s,12,5); assert(s.rejected==2 && s.faults==RS_INTERFACE);
    baseline(&s); hold(&s,4,3); hold(&s,12,1); assert(s.faults==RS_INTERFACE);
    baseline(&s); hold(&s,1,3); hold(&s,5,4); assert(s.faults==RS_INTERFACE);
    baseline(&s); hold(&s,4,30); assert(s.launched==1 && !s.halted);
    baseline(&s); for(unsigned i=0;i<4;i++) hold(&s,4|(i&1),1);
    assert(s.faults==RS_INTERFACE);
    baseline(&s); event(&s,0,4); ledger_sample(&s,clock_ms+2,0);
    assert(s.faults==RS_GAP && s.active==1 && s.missed==1);
    Ledger retained=s; hold(&s,15,10); assert(s.active==retained.active && s.attempts==retained.attempts);
    ledger_init(&s); clock_ms=UINT32_MAX-10; hold(&s,0,20); assert(!s.halted);
    baseline(&s); s.rounds=(UINT32_MAX-1u)/2u; s.launched=s.rounds;
    s.completed=s.rounds; s.readers=s.rounds; s.attempts=2*s.rounds;
    s.observed=s.rounds; assert(ledger_valid(&s));
    event(&s,0,4); assert(s.attempts==UINT32_MAX);
    event(&s,0,8); assert(s.faults==RS_EXHAUSTED && s.active==1);
    assert(ledger_valid(&s));
    char output[768]; assert(wire_status(output,sizeof output,&s));
    assert(!wire_status(output,2,&s)); assert(!wire_status(NULL,768,&s));
    baseline(&s); s.active=9; s.seen=9; s.launched=2; s.attempts=2; s.readers=1; s.writers=1;
    assert(!ledger_valid(&s));
    printf("engine: %u four-event words, sharing/reentry/exclusion, raw guards, wrap, gap, overflow PASS\n",cases);
}
