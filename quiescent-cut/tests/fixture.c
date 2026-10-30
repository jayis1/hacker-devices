/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic production-encoder data; never physical measurements. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static char buffer[768];
static void emit(size_t size)
{
    assert(size); assert(fwrite(buffer,1,size,stdout)==size);
}
static void run(unsigned mask,unsigned ticks)
{
    while(ticks--) {
        ledger_sample(&s,s.now+1,mask);
        if(s.now%10==0) emit(wire_status(buffer,sizeof buffer,&s));
    }
}
int main(void)
{
    ledger_init(&s);
    emit(wire_boot(buffer,sizeof buffer,42));
    emit(wire_policy(buffer,sizeof buffer));
    run(3,20); run(7,20); run(2,20); run(3,20);
    run(1,20); run(9,20); run(1,20);
    run(5,20); run(9,20); run(1,20);
    run(5,20); ledger_sample(&s,s.now+3,1);
    emit(wire_status(buffer,sizeof buffer,&s));
    return 0;
}
