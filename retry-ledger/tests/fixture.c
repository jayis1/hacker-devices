/* Author: jayis1. Synthetic stream, not physical measurements. */
#include "budget.h"
#include "wire.h"
#include <stdio.h>
static void output(Budget *b)
{
    char text[384];
    if (wire_status(text,sizeof text,b,0)) fputs(text,stdout);
    Event e;
    while (budget_pop(b,&e)) {
        if (wire_event(text,sizeof text,&e)) fputs(text,stdout);
    }
}
int main(void)
{
    Budget b;
    char text[384];
    budget_init(&b,0);
    wire_boot(text,sizeof text,0); fputs(text,stdout);
    wire_policy(text,sizeof text); fputs(text,stdout);
    for (uint32_t t=1;t<=5500;++t) {
        uint8_t levels=0;
        if ((t>=100 && t<110)||(t>=200 && t<210)||(t>=300 && t<310)) levels=INPUT_FAIL;
        if (t>=320 && t<1000) levels|=INPUT_LOCK;
        if (t>=600 && t<700) levels|=INPUT_ACCESS;
        budget_tick(&b,t,levels);
        if (t%100==0) output(&b);
    }
}
