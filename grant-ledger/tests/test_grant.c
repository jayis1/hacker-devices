/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "grant.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void level(struct grant *g, uint8_t bits)
{
    for (unsigned i=0; i<3; ++i) grant_sample(g, ++now, bits);
}
static void setup(struct grant *g)
{
    now=0;
    grant_init(g, now);
    level(g, 0);
    assert(g->synced);
}
static void request_grant(struct grant *g)
{
    level(g, GL_REQUEST);
    level(g, GL_REQUEST|GL_GRANT);
    assert(g->state == GL_CREDIT);
}
int main(void)
{
    struct grant g;
    now=0;
    grant_init(&g,now);
    level(&g,GL_REQUEST);
    assert(!g.synced && g.requests==0);
    level(&g,0);
    assert(g.synced);
    grant_sample(&g,now,GL_REQUEST);
    grant_sample(&g,++now,GL_REQUEST);
    grant_sample(&g,++now,0);
    grant_sample(&g,++now,GL_REQUEST);
    assert(g.state==GL_IDLE);
    level(&g,GL_REQUEST);
    assert(g.state==GL_PENDING);
    setup(&g);
    level(&g,GL_REQUEST);
    for(unsigned i=0;i<997;++i) grant_sample(&g,++now,GL_REQUEST);
    level(&g,GL_REQUEST|GL_GRANT);
    assert(g.faults[GL_EXPIRED]==1 && g.faults[GL_ORPHAN]==1);
    setup(&g);
    level(&g,GL_REQUEST);
    for(unsigned i=0;i<900;++i) grant_sample(&g,++now,GL_REQUEST);
    level(&g,GL_REQUEST|GL_GRANT);
    assert(g.state==GL_CREDIT && g.age==0);
    for(unsigned i=0;i<997;++i) grant_sample(&g,++now,GL_REQUEST|GL_GRANT);
    level(&g,GL_REQUEST|GL_GRANT|GL_USE);
    assert(g.accepted==0 && g.faults[GL_EXPIRED]==1 && g.faults[GL_NO_CREDIT]==1);
    setup(&g);
    request_grant(&g);
    level(&g,GL_REVOKE);
    level(&g,0);
    level(&g,GL_USE);
    assert(g.accepted==0 && g.faults[GL_NO_CREDIT]==1);
    setup(&g);
    request_grant(&g);
    level(&g, GL_REQUEST|GL_GRANT|GL_USE);
    assert(g.accepted==1 && g.state==GL_IDLE);
    level(&g, 0);
    level(&g, GL_USE);
    assert(g.faults[GL_NO_CREDIT]==1);
    setup(&g);
    level(&g, GL_GRANT);
    assert(g.faults[GL_ORPHAN]==1 && g.state==GL_IDLE);
    setup(&g);
    request_grant(&g);
    level(&g, GL_REQUEST|GL_GRANT|GL_USE|GL_REVOKE);
    assert(g.accepted==0 && g.faults[GL_REVOKED_USE]==1);
    setup(&g);
    level(&g, GL_REQUEST|GL_GRANT);
    assert(g.faults[GL_AMBIGUOUS]==1 && g.state==GL_IDLE);
    setup(&g);
    request_grant(&g);
    grant_sample(&g, now+5, GL_REQUEST|GL_GRANT);
    now+=5;
    assert(!g.synced && g.state==GL_IDLE && g.gaps==1 && g.missed==4);
    level(&g, GL_REQUEST|GL_GRANT);
    assert(!g.synced);
    level(&g, 0);
    assert(g.synced);
    setup(&g);
    request_grant(&g);
    for(unsigned i=0;i<GL_WINDOW_MS;++i) grant_sample(&g, ++now, GL_REQUEST|GL_GRANT);
    assert(g.state==GL_IDLE && g.faults[GL_EXPIRED]==1);
    setup(&g);
    level(&g, GL_REQUEST);
    level(&g, 0);
    level(&g, GL_REQUEST);
    assert(g.faults[GL_OVERLAP]==1 && g.state==GL_IDLE);
    setup(&g);
    now=UINT32_MAX-2;
    grant_init(&g, now);
    level(&g,0);
    request_grant(&g);
    assert(g.state==GL_CREDIT && g.gaps==0);
    setup(&g);
    for(unsigned i=0;i<100;++i) {level(&g,GL_USE);level(&g,0);}
    assert(g.dropped>0 && g.used==GL_QUEUE);
    struct gl_event e;
    unsigned count=0;
    while(grant_pop(&g,&e)) ++count;
    assert(count==GL_QUEUE);
    assert(gl_crc("123456789",9)==0x29b1);
    char out[384];
    assert(gl_status(out,sizeof out,&g)>0);
    assert(strncmp(out,"GL1,S,",6)==0);
    assert(gl_status(out,5,&g)==0);
    g.faults[GL_NO_CREDIT]=UINT32_MAX;
    level(&g,GL_USE);
    assert(g.faults[GL_NO_CREDIT]==UINT32_MAX);
    puts("grant engine: transaction, revoke, ambiguity, expiry, gaps, wrap, queue, CRC PASS");
}
