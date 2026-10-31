/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic encoder fixture, not physical target evidence. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Ledger s;
static void hold(unsigned raw,unsigned count)
{
    while(count--) ledger_sample(&s,s.now+1,(uint8_t)raw);
    char b[768]; size_t n=wire_status(b,sizeof b,&s);
    assert(n); fwrite(b,1,n,stdout);
}
static void op(unsigned tag,unsigned edge)
{
    hold(tag,20); hold(tag|edge,20); hold(tag,20);
}
int main(void)
{
    char b[768]; ledger_init(&s);
    size_t n=wire_boot(b,sizeof b,0); fwrite(b,1,n,stdout);
    n=wire_policy(b,sizeof b); fwrite(b,1,n,stdout);
    hold(0,20);
    op(2,8); op(3,8); /* two independent scrub completions */
    op(0,4); op(1,4); /* consume both */
    op(0,8); op(2,8); op(0,4); /* recycle while peer is live */
    op(1,8); op(0,8); /* drain both */
    assert(!s.halted && s.rounds==1 && s.scrubbed==3);
    op(1,4); /* a return alone did not replenish slot one's credit */
    assert(s.faults==OB_CREDIT && !s.active && !s.clean);
    return 0;
}
