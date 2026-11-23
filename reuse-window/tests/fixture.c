/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic waveform through production engine/encoder, not hardware. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Ledger state;
static bool dense;
static void emit(void)
{
    char out[768];
    size_t n=wire_status(out,sizeof out,&state);
    assert(n); fwrite(out,1,n,stdout);
}
static void hold(unsigned raw,unsigned count)
{
    while(count--) {
        ledger_sample(&state,state.now+1u,raw);
        if(dense) emit();
    }
}
static void op(unsigned tag,bool allocation)
{
    hold(tag,12); hold(tag|(allocation?4u:8u),6); hold(tag,6);
    if(!dense) emit();
}
int main(int argc,char **argv)
{
    char out[768];
    dense=argc>1 && !strcmp(argv[1],"dense");
    size_t n=wire_policy(out,sizeof out); fwrite(out,1,n,stdout);
    ledger_init(&state); hold(0,20); if(!dense) emit();
    for(unsigned i=0;i<4;i++) { op(i,true); op(i,false); }
    for(unsigned i=0;i<4;i++) { op(i,true); op(i,false); }
    if(argc>1 && !strcmp(argv[1],"legal")) {
        hold(0,1000); emit(); return 0;
    }
    if(dense) {
        hold(0,1000); /* Replenishment, new admissions and old deadline shift. */
        op(0,true); op(0,false); op(0,true); op(0,false);
        hold(0,state.remaining[0][0]-16u); op(0,true); op(0,false);
        ledger_sample(&state,state.now+2u,0); emit();
        hold(0,5); return 0;
    }
    op(0,true); /* Third admission is a terminal window finding. */
    return 0;
}
