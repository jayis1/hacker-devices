/* Author: jayis1. Synthetic software fixture, never a physical capture. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static void send(char *out,size_t n) { assert(n); assert(fwrite(out,1,n,stdout)==n); }
int main(int argc, char **argv)
{
    (void)argv;
    Ledger s; ledger_init(&s); char out[768];
    send(out,wire_policy(out,sizeof out));
    if(argc>1) {
        for(unsigned t=1;t<=400;++t) {
            unsigned raw=t<50?0:t<100?3:t<150?7:t<250?3:11;
            if(t==350) ledger_halt(&s,CT_WATCHDOG);
            ledger_sample(&s,t+(t>=200?10:0),raw);
            if(t%20==0) send(out,wire_status(out,sizeof out,&s));
        }
        return 0;
    }
    for(unsigned t=1;t<=800;++t) {
        unsigned phase=(t-1)/50;
        const unsigned raw[]={0,3,7,3,7,3,11,3,15,3,11,3,8,0,3,7};
        ledger_sample(&s,t,raw[phase]);
        if(t%20==0) send(out,wire_status(out,sizeof out,&s));
    }
    return 0;
}
