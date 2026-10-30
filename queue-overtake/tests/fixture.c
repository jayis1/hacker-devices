/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic observations through the production encoder, not hardware data. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static char output[768];
static void emit(size_t n) { assert(n); assert(fwrite(output,1,n,stdout)==n); }
static void hold(unsigned raw,unsigned n)
{
    while(n--) ledger_sample(&s,s.now+1,raw);
}
static void operation(unsigned tag,bool add)
{
    hold(tag,20); hold(tag|(add?4:8),20); hold(tag,20);
    emit(wire_status(output,sizeof output,&s));
}
int main(void)
{
    ledger_init(&s);
    emit(wire_boot(output,sizeof output,0));
    emit(wire_policy(output,sizeof output));
    hold(0,20); emit(wire_status(output,sizeof output,&s));
    operation(2,true); operation(0,true); operation(2,false);
    operation(2,true); operation(0,false); operation(2,false);
    operation(3,true); operation(1,true); operation(1,false);
    assert(s.faults==QO_OVERTAKE && s.rounds==1);
    return 0;
}
