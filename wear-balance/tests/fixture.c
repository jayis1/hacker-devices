/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic observations emitted by the real production encoder. */
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
    op(0,true); op(0,false);
    op(1,true); op(1,false);
    op(0,true); /* Tag two and three are still unused: bias finding. */
    assert(s.faults==WB_BIAS);
    return 0;
}
