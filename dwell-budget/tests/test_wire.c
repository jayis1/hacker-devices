/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static Budget b;
int main(void)
{
    char out[384], small[2]={'x','y'};
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_policy(small,1)==0 && small[1]=='y');
    assert(wire_boot(NULL,0,0)==0);
    assert(wire_status(out,sizeof out,NULL)==0);
    budget_init(&b);
    size_t n=wire_status(out,sizeof out,&b);
    assert(n>10 && out[n-1]=='\n');
    assert(memcmp(out,"DB1,S,",6)==0);
    for(size_t i=0;i<n;++i) assert(wire_status(out,i,&b)==0);
    assert(wire_status(out,n,&b)==n);
    puts("PASS CRC known answer and bounded encoder");
}
