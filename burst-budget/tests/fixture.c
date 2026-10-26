/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic observations, never presented as physical measurements. */
#include "budget.h"
#include "wire.h"
#include <stdio.h>
int main(void)
{
    Budget s;
    char frame[384];
    budget_init(&s);
    wire_boot(frame,sizeof frame,0); fputs(frame,stdout);
    wire_policy(frame,sizeof frame); fputs(frame,stdout);
    for(unsigned t=0;t<=4500;++t) {
        unsigned raw=4;
        if(t>=4100 && t<4220 && ((t-4100)%20)<10) raw=5;
        budget_sample(&s,t,raw);
        if(t%100==0) {
            if(!wire_status(frame,sizeof frame,&s)) return 1;
            fputs(frame,stdout);
        }
    }
    return 0;
}
