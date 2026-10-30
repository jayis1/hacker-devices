/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static Ledger s;
static uint32_t clock_ms;
static void hold(unsigned raw, unsigned ticks)
{
    while (ticks--) {
        ledger_sample(&s, clock_ms++, (uint8_t)raw);
        assert(ledger_valid(&s));
    }
}
static void init(void)
{
    ledger_init(&s); clock_ms=0; hold(0,20);
    assert(s.phase==RS_IDLE);
}
static void session(void)
{
    hold(8,20); assert(s.phase==RS_SESSION);
}
static void admit(unsigned tag)
{
    hold(8|tag,12); hold(12|tag,12); hold(8|tag,12);
}
static void permutations(unsigned mask, unsigned order[4], unsigned depth)
{
    if(depth==4) {
        init(); session();
        for(unsigned i=0;i<4;i++) admit(order[i]);
        assert(s.fresh==4 && s.seen==15 && !s.duplicate);
        for(unsigned i=0;i<4;i++) admit(order[i]);
        assert(s.fresh==4 && s.duplicate==4 && s.admissions==8);
        hold(0,20); session(); admit(order[0]);
        assert(s.fresh==5 && s.sessions==2 && !s.halted);
        return;
    }
    for(unsigned i=0;i<4;i++) if(!(mask&(1u<<i))) {
        order[depth]=i; permutations(mask|(1u<<i),order,depth+1);
    }
}
int main(void)
{
    unsigned order[4]; permutations(0,order,0);
    /* No session, same-tick session, and unstable identity are terminal. */
    init(); hold(4,10); assert(s.invalid==1 && s.halted);
    init(); hold(12,10); assert(s.invalid==1 && s.halted && !s.sessions);
    init(); session(); hold(13,4); assert(s.invalid==1 && s.halted);
    /* Accepted strobe held high counts once, even if the tag later changes. */
    init(); session(); hold(12,40); hold(13,40);
    assert(s.admissions==1 && s.fresh==1 && !s.halted);
    /* Raw session withdrawal on the acceptance endpoint invalidates it. */
    init(); session(); hold(12,3); hold(4,1);
    assert(s.invalid==1 && s.halted);
    /* Ignored short session-low glitches do not clear the set. */
    init(); session(); admit(2); hold(2,2); hold(10,10); admit(2);
    assert(s.duplicate==1 && s.sessions==1);
    /* Startup in an active session must wait for a real inactive baseline. */
    ledger_init(&s); clock_ms=0; hold(12,20);
    assert(s.phase==RS_WAIT && !s.admissions); hold(0,20); session(); admit(1);
    assert(s.fresh==1);
    init(); session(); admit(3);
    clock_ms+=5; hold(8,1); assert(s.gaps==1 && s.missed==5 && s.halted);
    unsigned before=s.admissions; hold(12,20); assert(s.admissions==before);
    /* Timer wrap is ordinary continuity, not a fresh session. */
    init(); s.now=UINT32_MAX-2; clock_ms=UINT32_MAX-1;
    hold(0,5); assert(!s.halted);
    /* Transactional overflow cannot erase previously committed membership. */
    init(); session(); admit(1); s.observed=UINT32_MAX;
    hold(8,1); assert(s.halted && (s.faults&RS_EXHAUSTED));
    assert(s.fresh==1 && s.admissions==1 && s.seen==2);
    char buffer[768]; assert(wire_status(buffer,sizeof buffer,&s));
    assert(!wire_status(buffer,4,&s)); assert(!wire_status(NULL,768,&s));
    Ledger bad=s; bad.fresh++; assert(!ledger_valid(&bad));
    /* Every long paced stream agrees with an independent boolean set oracle. */
    init(); session(); unsigned seen=0, fresh=0, duplicate=0, seed=17;
    for(unsigned i=0;i<2000;i++) {
        seed=seed*1664525u+1013904223u; unsigned tag=seed>>30;
        if(seen&(1u<<tag)) duplicate++; else { fresh++; seen|=1u<<tag; }
        admit(tag);
        assert(s.fresh==fresh && s.duplicate==duplicate && s.seen==seen);
    }
    puts("engine: permutations, replay, session edges, gaps, overflow, oracle PASS");
}
