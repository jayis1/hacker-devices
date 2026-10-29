/* Author: jayis1. Synthetic software fixture, never a physical capture. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static void send(char *out,size_t n) { assert(n); assert(fwrite(out,1,n,stdout)==n); }
int main(void)
{
    Ledger s; ledger_init(&s); char out[768];
    send(out,wire_policy(out,sizeof out));
    for(unsigned t=1;t<=800;++t) {
        unsigned phase=(t-1)/100;
        const unsigned raw[]={0,3,7,3,11,3,15,0};
        ledger_sample(&s,t,raw[phase]);
        if(t%20==0) send(out,wire_status(out,sizeof out,&s));
    }
    return 0;
}
