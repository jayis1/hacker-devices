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
    for(unsigned j=0;j<2;j++) for(unsigned tag=0;tag<3;tag++) {
        op(tag,true); op(tag,false);
    }
    assert(!s.halted && s.launched==6 && !s.uses[3]);
    if(argc==2 && !strcmp(argv[1],"legal")) return 0;
    op(2,true); /* Release and complete drain cannot erase the last ordinal. */
    assert(s.faults==UQ_DISTANCE && !s.active && s.uses[2]==2);
    return 0;
}
