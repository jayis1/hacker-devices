/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static void ticks(Ledger *s, unsigned n, unsigned raw)
{
    while (n--) {
        ledger_sample(s, s->now + 1u, raw);
        assert(ledger_valid(s));
    }
}
static void pairs(void)
{
    for (unsigned mask=0; mask<16; ++mask) {
        Ledger s; ledger_init(&s);
        ticks(&s,4,mask);
        assert(s.valid==15 && s.observed==1);
        unsigned k=0;
        for(unsigned i=0;i<4;++i) for(unsigned j=i+1;j<4;++j,++k) {
            unsigned both=((mask>>i)&1u)&&((mask>>j)&1u);
            assert(s.pair[k].entries==both);
            assert(s.pair[k].active==both);
            assert(s.pair[k].duration==0);
        }
        ticks(&s,10,mask);
        for(k=0;k<6;++k) assert(s.pair[k].duration==10*s.pair[k].active);
        ticks(&s,4,0);
        for(k=0;k<6;++k) {
            assert(!s.pair[k].active);
            assert(s.pair[k].duration==13*s.pair[k].entries);
            assert(s.pair[k].maximum==s.pair[k].duration);
        }
    }
}
static void interruptions(void)
{
    Ledger s; ledger_init(&s); ticks(&s,10,3);
    assert(s.pair[0].entries==1 && s.pair[0].duration==6);
    ledger_sample(&s,s.now+5,3); assert(ledger_valid(&s));
    assert(s.gaps==1 && s.missed==4 && !s.pair[0].active);
    ticks(&s,3,3);
    assert(s.pair[0].entries==2 && s.pair[0].duration==6);
    assert(s.pair[0].maximum==6 && s.pair[0].age==0);
    ticks(&s,2,3); assert(s.pair[0].duration==8);
    uint32_t observed=s.observed;
    ledger_sample(&s,s.now,0); assert(s.observed==observed);
    s.now=UINT32_MAX; ticks(&s,1,3); assert(!s.halted && s.gaps==1);
}
static void independent(void)
{
    Ledger s; ledger_init(&s);
    for(unsigned i=0;i<20;++i) ledger_sample(&s,i,3u|((i&1u)<<2));
    assert(ledger_valid(&s));
    /* Chatter on channel 2 does not hide the established 0/1 collision. */
    assert(s.pair[0].entries==1 && s.pair[0].duration==16);
    assert(s.observed==0); /* whole-vector coverage needs all four qualified */
}
static void exhaustion(void)
{
    Ledger s; ledger_init(&s); ticks(&s,4,3);
    s.pair[0].duration=UINT32_MAX-1;
    s.pair[0].age=UINT32_MAX-1;
    s.pair[0].maximum=UINT32_MAX-1;
    ticks(&s,1,3); assert(!s.halted);
    ticks(&s,1,3); assert(s.halted && (s.faults&EL_EXHAUSTED));
    assert(s.pair[0].duration==UINT32_MAX && !s.pair[0].active);
    char frame[768]; assert(wire_status(frame,sizeof frame,&s));
    uint32_t duration=s.pair[0].duration;
    ticks(&s,100,0); assert(s.pair[0].duration==duration);
    assert(!wire_status(frame,2,&s));
}
static void random_oracle(void)
{
    Ledger s; ledger_init(&s);
    unsigned seed=17, raw=0, candidates=0, runs[4]={0}, valid=0, levels=0;
    unsigned active[6]={0}, entries[6]={0}, duration[6]={0};
    for(unsigned t=0;t<100000;++t) {
        seed=seed*1664525u+1013904223u;
        if((seed&7)==0) raw=(seed>>8)&15;
        for(unsigned i=0;i<4;++i) {
            unsigned bit=1u<<i;
            if(t==0 || ((raw^candidates)&bit)) {
                candidates=(candidates&~bit)|(raw&bit); runs[i]=0;
            } else if(runs[i]<3) ++runs[i];
            if(runs[i]==3) {valid|=bit; levels=(levels&~bit)|(raw&bit);}
        }
        unsigned k=0;
        for(unsigned i=0;i<4;++i) for(unsigned j=i+1;j<4;++j,++k) {
            unsigned bits=(1u<<i)|(1u<<j);
            unsigned on=(valid&bits)==bits && (levels&bits)==bits;
            if(on && !active[k]) ++entries[k];
            if(on && active[k]) ++duration[k];
            active[k]=on;
        }
        ledger_sample(&s,t,raw); assert(ledger_valid(&s));
        for(k=0;k<6;++k) {
            assert(s.pair[k].entries==entries[k]);
            assert(s.pair[k].duration==duration[k]);
        }
    }
}
int main(void)
{
    uint16_t crc=0;
    assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1u);
    assert(crc16_verify("123456789",9,0x29b1u));
    assert(!crc16_verify("123456780",9,0x29b1u));
    assert(crc16_verify(NULL,0,0xffffu));
    assert(!crc16_buffer(NULL,1,&crc));
    assert(!crc16_buffer("x",1,NULL));
    pairs(); interruptions(); independent(); exhaustion(); random_oracle();
    puts("engine: all masks, pair oracle, qualification, gaps, wrap, exhaustion passed");
}
