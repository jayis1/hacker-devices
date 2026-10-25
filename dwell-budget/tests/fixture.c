/* Author: jayis1. Synthetic data, not a hardware capture. */
#include "budget.h"
#include "wire.h"
#include <stdio.h>
static Budget b;
static char frame[384];
static void emit(size_t n) { fwrite(frame,1,n,stdout); }
int main(void)
{
    emit(wire_boot(frame,sizeof frame,0));
    emit(wire_policy(frame,sizeof frame));
    budget_init(&b);
    for(uint32_t t=0;t<=12000;++t) {
        budget_sample(&b,t,t<=3000?3:0);
        if(t%100==0) emit(wire_status(frame,sizeof frame,&b));
    }
}
