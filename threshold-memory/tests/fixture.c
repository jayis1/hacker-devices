/* Author: jayis1. Synthetic evidence, not physical measurements. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static void emit(Ledger *s)
{
    char out[768]; size_t n=wire_status(out,sizeof out,s);
    fwrite(out,1,n,stdout);
}
int main(void)
{
    Ledger s; char out[768]; ledger_init(&s);
    size_t n=wire_policy(out,sizeof out); fwrite(out,1,n,stdout);
    for(unsigned t=1;t<=800;++t) {
        unsigned raw=t<100?0:t<200?7:t<300?5:t<400?1:t<500?5:t<600?0:t<700?4:8;
        ledger_sample(&s,t,(uint8_t)raw);
        if(t%20==0) emit(&s);
    }
    return 0;
}
