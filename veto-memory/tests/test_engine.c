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
    for (unsigned i=0; i<n; ++i) {
        ledger_sample(s, clock_ms++, raw);
        assert(ledger_valid(s));
        char out[768];
        assert(wire_status(out,sizeof out,s)>0);
    }
}
static void reset(Ledger *s)
{
    clock_ms=0; ledger_init(s); hold(s,0,8);
    assert(s->phase==VM_READY && s->baselines==1);
}
static void clean_and_veto(void)
{
    Ledger s; reset(&s);
    hold(&s,1,8); hold(&s,3,8); hold(&s,7,8);
    assert(s.accepted==1 && s.phase==VM_DRAIN);
    hold(&s,3,8); hold(&s,7,8);
    assert(s.repeated==1 && s.commits==2 && s.rounds==1);
    hold(&s,0,8); assert(s.phase==VM_READY);
    hold(&s,3,8); hold(&s,11,8); hold(&s,3,8); hold(&s,7,8);
    assert(s.vetoed==1 && s.veto_rounds==1 && s.accepted==1);
    hold(&s,0,8); hold(&s,3,8); hold(&s,1,8); hold(&s,3,8); hold(&s,7,8);
    assert(s.vetoed==2 && s.withdrawals==1);
    hold(&s,0,8); hold(&s,1,8); hold(&s,0,8);
    assert(s.cancelled==1);
}
static void boundaries(void)
{
    Ledger s; reset(&s); hold(&s,7,8);
    assert(s.early==1 && !s.accepted);
    reset(&s); hold(&s,3,8); hold(&s,15,8);
    assert(s.vetoed==1 && !s.accepted);
    reset(&s); hold(&s,3,8);
    /* A raw abort on the commit qualification endpoint vetoes immediately. */
    hold(&s,7,3); hold(&s,15,1);
    assert(s.vetoed==1 && s.veto_rounds==1);
    reset(&s); hold(&s,3,8); hold(&s,7,3); hold(&s,6,1);
    assert(s.early==1 && !s.accepted);
    reset(&s); hold(&s,3,8); hold(&s,2,1); hold(&s,3,8); hold(&s,7,8);
    assert(s.accepted==1); /* rejected raw preparation glitch is not withdrawal */
    ledger_init(&s); clock_ms=0; hold(&s,7,20);
    assert(s.rounds==0 && s.phase==VM_WAIT);
    hold(&s,0,8); hold(&s,3,8); hold(&s,7,8); assert(s.accepted==1);
}
static void masks(void)
{
    for(unsigned prep=0;prep<4;prep++) {
        for(unsigned incoming=4;incoming<16;incoming++) {
            if(!(incoming&4)) continue;
            Ledger s; reset(&s); hold(&s,prep,8); hold(&s,incoming,8);
            assert(s.commits==1);
            bool veto=(incoming&8) || (prep & ~incoming & 3);
            bool good=!veto && prep==3 && (incoming&3)==3;
            assert(s.vetoed==(unsigned)veto);
            assert(s.accepted==(unsigned)good);
            assert(s.early==(unsigned)(!veto && !good));
        }
    }
}
static void continuity(void)
{
    Ledger s; reset(&s); hold(&s,3,8);
    clock_ms+=3; hold(&s,3,1);
    assert(s.halted && s.abandoned==1 && s.gaps==1 && s.missed==3);
    hold(&s,0,8); hold(&s,7,8); assert(s.commits==0);
    reset(&s); uint32_t old=s.observed;
    ledger_sample(&s,s.now,7); assert(s.observed==old);
    ledger_init(&s); clock_ms=UINT32_MAX-4; hold(&s,0,12);
    assert(!s.gaps && s.phase==VM_READY);
    hold(&s,3,8); s.observed=UINT32_MAX;
    hold(&s,7,1); assert(s.halted && s.abandoned==1 && !s.commits);
    ledger_init(&s); ledger_halt(&s,VM_WATCHDOG); assert(ledger_valid(&s));
    s.tx_fail=UINT32_MAX; ledger_tx_failure(&s); assert(s.tx_fail==UINT32_MAX);
}
static void random_walk(void)
{
    Ledger s; reset(&s); uint32_t rng=42;
    for(unsigned i=0;i<20000;i++) {
        rng=rng*1664525u+1013904223u;
        hold(&s,rng>>28,1+(rng&7));
    }
}
static void encoding(void)
{
    Ledger s; reset(&s); char out[768];
    size_t n=wire_status(out,sizeof out,&s); assert(n>5);
    assert(!wire_status(out,1,&s)); assert(!wire_status(NULL,0,&s));
    assert(!wire_status(out,sizeof out,NULL));
    uint16_t crc; assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1);
    s.rounds++; assert(!ledger_valid(&s) && !wire_status(out,sizeof out,&s));
}
int main(void)
{
    clean_and_veto(); boundaries(); masks(); continuity(); random_walk(); encoding();
    puts("engine: veto retention, withdrawal, commit masks, boundary ordering, gaps, overflow, random walks PASS");
}
