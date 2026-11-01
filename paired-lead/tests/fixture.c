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
    if(argc==2 && !strcmp(argv[1],"independent")) {
        for(unsigned i=0;i<6;++i) {
            op(0,true); op(0,false); op(1,true); op(1,false);
        }
        assert(!s.halted && s.uses[0]==6 && !s.uses[2]);
        return 0;
    }
    if(argc==2 && !strcmp(argv[1],"recovery")) {
        op(0,true); op(0,false); op(0,true); op(0,false);
        op(1,true); op(0,true);
        assert(!s.halted && s.uses[0]==3 && s.active==3);
        return 0;
    }
    op(2,true);
    op(0,true); op(0,false); op(0,true); op(0,false);
    op(0,true); /* Third lead is rejected despite complete release of zero. */
    assert(s.faults==UQ_LEAD && s.active==4 && s.uses[0]==2);
    return 0;
}
