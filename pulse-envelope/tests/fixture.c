/* Author: jayis1. Synthetic stream from production engine and encoder. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
int main(void)
{
    Witness w; char out[768]; witness_init(&w);
    assert(wire_boot(out,sizeof out,0)); fputs(out,stdout);
    assert(wire_policy(out,sizeof out)); fputs(out,stdout);
    for(uint32_t t=0;t<=1000;++t) {
        uint8_t raw=0;
        if(t>=100 && t<120) raw|=1;
        if(t>=200 && t<205) raw|=2;
        if(t>=300 && t<550) raw|=4;
        if((t>=600 && t<620)||(t>=640 && t<660)) raw|=8;
        witness_sample(&w,t,raw);
        if(t%100==0) {
            assert(wire_status(out,sizeof out,&w));
            fputs(out,stdout);
        }
    }
}
