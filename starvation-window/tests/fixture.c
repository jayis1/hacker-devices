/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic stimulus; not a measurement of physical hardware. */
#include "isolation.h"
#include "wire.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    (void)argv;
    Isolation s;
    char frame[384];
    isolation_init(&s);
    wire_policy(frame,sizeof frame); fputs(frame,stdout);
    for (uint32_t t=0;t<=(argc>1?250u:100u);++t) {
        unsigned levels=t<25?0:t<35?5:t<45?0:t<60?10:t<70?15:0;
        if (argc>1)
            levels=(t==0 || t>=230)?0:(t==20 || t==40 || t==60)?11:3;
        isolation_sample(&s,t,levels);
        if (t%10==0) {
            wire_status(frame,sizeof frame,&s);
            fputs(frame,stdout);
        }
    }
    return 0;
}
