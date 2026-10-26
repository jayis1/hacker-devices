/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "isolation.h"
#include <assert.h>
#include <stdio.h>
static uint32_t now;
static void tick(Isolation *s,unsigned levels,unsigned count)
{
    while(count--) isolation_sample(s,++now,levels);
}
int main(void)
{
    Isolation s; isolation_init(&s); now=0;
    isolation_sample(&s,now,0);
    tick(&s,1,1); tick(&s,1,99);
    assert(s.wait[0]==99 && s.starved[0]==0);
    tick(&s,1,1);
    assert(s.starved[0]==1 && (s.faults & IW_STARVED_A));
    tick(&s,1,5); assert(s.starved[0]==1);
    tick(&s,5,1); assert(s.wait[0]==0 && s.services[0]==1);
    tick(&s,3,1); /* A and B pending, no enables */
    for (unsigned i=0;i<3;++i) { tick(&s,11,1); tick(&s,3,1); }
    assert(s.bypass[0]==3 && s.starved[0]==2 && s.services[1]==3);
    tick(&s,1,1); assert(s.wait[1]==0 && s.bypass[1]==0);
    tick(&s,0,1); assert(s.wait[0]==0 && s.bypass[0]==0);
    tick(&s,11,1); /* simultaneous arrival must not invent prior waiting */
    assert(s.bypass[0]==0);
    tick(&s,3,1); tick(&s,7,1); /* B bypassed by A */
    assert(s.bypass[1]==1);
    now+=4; isolation_sample(&s,now,3);
    assert(s.wait[0]==0 && s.wait[1]==0 && s.bypass[1]==0);
    tick(&s,3,100);
    assert(s.starved[0]==3 && s.starved[1]==1);
    assert(isolation_valid(&s));
    puts("PASS starvation exact deadline, bypass budget, both directions, gap and release");
}
