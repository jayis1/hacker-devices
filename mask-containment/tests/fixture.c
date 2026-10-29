/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic software fixture, never a physical target capture. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void emit(char *p, size_t n)
{
    assert(n);
    assert(fwrite(p,1,n,stdout)==n);
}
int main(void)
{
    Ledger s; ledger_init(&s); char out[768];
    emit(out,wire_boot(out,sizeof out,0));
    emit(out,wire_policy(out,sizeof out));
    unsigned patterns[]={0,1,5,4,0,2,10,8,12,15,0,4,0};
    uint32_t now=0;
    for(unsigned i=0;i<sizeof patterns/sizeof patterns[0];++i) {
        for(unsigned t=0;t<100;++t) ledger_sample(&s,now++,patterns[i]);
        emit(out,wire_status(out,sizeof out,&s));
    }
}
