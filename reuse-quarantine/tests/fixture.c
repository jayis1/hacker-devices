/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic device encoder fixture, not a physical measurement. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Ledger s;
static void hold(unsigned raw,unsigned n)
{
    while(n--) ledger_sample(&s,s.now+1,(uint8_t)raw);
    char b[768]; size_t length=wire_status(b,sizeof b,&s);
    assert(length); fwrite(b,1,length,stdout);
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
    op(0,4); op(1,4); op(0,8); op(3,8);
    hold(0,250); /* first timer expires, second remains quarantined */
    op(0,4); op(0,8);
    op(1,4); /* long-class resource reused too soon */
    assert(s.faults==OB_EARLY && !s.active && s.cooling==3);
    return 0;
}
