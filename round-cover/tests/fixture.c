/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic diagnostic observations, not captured physical traffic. */
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
    if (argc==2 && !strcmp(argv[1],"rounds")) {
        /* Two shuffled rounds without a pool-wide drain. */
        const unsigned first[4]={2,0,3,1}, second[4]={1,3,0,2};
        for(unsigned i=0;i<4;++i) op(first[i],true);
        for(unsigned i=0;i<4;++i) { op(second[i],false); op(second[i],true); }
        assert(!s.halted && s.launched==8 && s.active==15);
        return 0;
    }
    if (argc==2 && !strcmp(argv[1],"busy")) {
        for(unsigned i=0;i<4;++i) op(i,true);
        for(unsigned i=0;i<3;++i) { op(i,false); op(i,true); op(i,false); }
        assert(!s.halted && s.active==8 && s.uses[3]==1);
        return 0;
    }
    op(0,true); op(2,true); /* Partial coverage: peers 1 and 3 are missing. */
    op(0,false);
    op(0,true); /* Release does not erase partial-round membership. */
    assert(s.faults==UQ_COVERAGE && s.active==4);
    return 0;
}
