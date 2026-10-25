/* Author: jayis1. Synthetic stimuli, not physical evidence. */
#include "health.h"
#include "wire.h"
#include <stdio.h>
int main(void)
{
    Health h;
    char frame[384];
    health_init(&h, 0);
    wire_boot(frame, sizeof frame, 0);
    fputs(frame, stdout);
    wire_policy(frame, sizeof frame);
    fputs(frame, stdout);
    for (unsigned i=0;i<128;i++) {
        unsigned bit = i<64 ? i&1u : 1u;
        for (unsigned j=0;j<10;j++) {
            health_tick(&h,h.time+1,(uint8_t)(bit|(j>=5?2:0)|(i>=64?8:0)));
        }
        wire_status(frame,sizeof frame,&h,0);
        fputs(frame,stdout);
        Event e;
        while (health_pop(&h,&e)) {
            wire_event(frame,sizeof frame,&e);
            fputs(frame,stdout);
        }
    }
    return 0;
}
