/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic diagnostic observations, not captured physical traffic. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static uint32_t now;
static char output[768];
static void snapshot(void)
{
    size_t n=wire_status(output,sizeof output,&s);
    assert(n); fwrite(output,1,n,stdout);
}
static void hold(unsigned raw,unsigned count)
{
    while(count--) ledger_sample(&s,++now,raw);
}
static void op(unsigned tag,bool alloc)
{
    hold(tag,12); hold(tag|(alloc?4:8),6); hold(tag,6); snapshot();
}
int main(void)
{
    ledger_init(&s);
    size_t n=wire_policy(output,sizeof output); fwrite(output,1,n,stdout);
    hold(0,20); snapshot();
    op(0,true); op(2,true); /* Independent resource lifetimes. */
    op(0,false);
    op(0,true); /* Release does not restore tag zero's spent ticket. */
    assert(s.faults==UQ_QUOTA && s.active==4);
    return 0;
}
