/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "grant.h"
#include "wire.h"
#include <stdio.h>
static struct grant g;
static uint32_t t;
static char packet[384];
static void samples(uint8_t raw)
{
    for(unsigned i=0;i<10;++i) grant_sample(&g,++t,raw);
    struct gl_event e;
    while(grant_pop(&g,&e)) {
        gl_event_frame(packet,sizeof packet,&e);
        fputs(packet,stdout);
    }
    gl_status(packet,sizeof packet,&g);
    fputs(packet,stdout);
}
int main(void)
{
    grant_init(&g,0);
    gl_boot(packet,sizeof packet,0);
    fputs(packet,stdout);
    samples(0);
    samples(GL_REQUEST);
    samples(GL_REQUEST|GL_GRANT);
    samples(GL_REQUEST|GL_GRANT|GL_USE);
    samples(0);
    samples(GL_USE);
    return 0;
}
