/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <stdio.h>
int main(void)
{
    Drain d; char out[384];
    drain_init(&d);
    wire_boot(out,sizeof out,0); fputs(out,stdout);
    wire_policy(out,sizeof out); fputs(out,stdout);
    for(uint32_t t=0;t<90;++t) {
        uint8_t levels=t<10?0:t<30?RD_REVOKE|RD_BUSY:t<55?RD_REVOKE:RD_REVOKE|RD_ACK;
        drain_sample(&d,t,levels);
        if(t%5==0) { wire_status(out,sizeof out,&d); fputs(out,stdout); }
    }
}
