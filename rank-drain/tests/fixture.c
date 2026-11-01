/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic observations emitted by the real encoder, not physical captures. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
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
int main(int argc, char **argv)
{
    ledger_init(&s);
    size_t n=wire_policy(output,sizeof output); fwrite(output,1,n,stdout);
    hold(0,20); snapshot();
    op(3,true); op(0,true); op(3,false); op(0,false);
    op(0,true); op(0,false); /* Immediate reuse, no distance constraint. */
    op(1,true); op(3,true);
    assert(!s.halted && s.launched==5 && s.completed==3 && s.active==10);
    if(argc==2 && !strcmp(argv[1],"legal")) return 0;
    op(1,false); /* Higher rank three still live: reject and retain both. */
    assert(s.faults==UQ_RANK && s.active==10 && s.completed==3);
    return 0;
}
