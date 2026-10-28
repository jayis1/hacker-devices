/* Author: jayis1. Synthetic software fixture, not a physical capture. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static char frame[768];
static void emit(size_t n) { assert(n); assert(fwrite(frame,1,n,stdout)==n); }
int main(void)
{
    Ledger s; ledger_init(&s);
    emit(wire_boot(frame,sizeof frame,0));
    emit(wire_policy(frame,sizeof frame));
    for(unsigned t=0;t<=400;++t) {
        unsigned raw=t<100?0:t<200?3:t<300?7:0;
        ledger_sample(&s,t,raw);
        if(t%100==0) emit(wire_status(frame,sizeof frame,&s));
    }
}
