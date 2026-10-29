/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic evidence from the production engine and encoder, not hardware. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void emit(char *out, size_t n)
{
    assert(n);
    assert(fwrite(out,1,n,stdout)==n);
}
int main(void)
{
    Ledger s;
    char out[768];
    const unsigned levels[]={0,9,8,0,4,6,12,4,6,5,15,14,12};
    ledger_init(&s);
    emit(out,wire_boot(out,sizeof out,0));
    emit(out,wire_policy(out,sizeof out));
    for(unsigned t=0;t<1300;++t) {
        ledger_sample(&s,t,(uint8_t)levels[t/100]);
        if(t%100==99)
            emit(out,wire_status(out,sizeof out,&s));
    }
    return 0;
}
