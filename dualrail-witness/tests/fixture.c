/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic vectors, not physical captures. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    Witness w;
    char frame[384];
    witness_init(&w);
    assert(wire_boot(frame,sizeof frame,0)); fputs(frame,stdout);
    assert(wire_policy(frame,sizeof frame)); fputs(frame,stdout);
    for (uint32_t t=0;t<=1000;++t) {
        uint8_t raw=t<200?2:t<400?1:t<600?5:t<800?7:15;
        witness_sample(&w,t,raw);
        if(t%100==0) {
            assert(wire_status(frame,sizeof frame,&w));
            fputs(frame,stdout);
        }
    }
    return 0;
}
