/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic fixture from the actual engine and encoder, not hardware data. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void emit(const char *p,size_t n) { assert(n); assert(fwrite(p,1,n,stdout)==n); }
int main(void)
{
    Ledger s; char buffer[768]; ledger_init(&s);
    emit(buffer,wire_boot(buffer,sizeof buffer,0));
    emit(buffer,wire_policy(buffer,sizeof buffer));
    const unsigned inputs[]={0,1,2,4,8,1,0,1,4,2,3,0,8};
    for(unsigned t=0;t<1300;++t) {
        ledger_sample(&s,t,inputs[t/100]);
        if(t%100==99) emit(buffer,wire_status(buffer,sizeof buffer,&s));
    }
}
