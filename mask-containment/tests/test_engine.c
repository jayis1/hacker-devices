/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Ledger *s, unsigned raw, unsigned count)
{
    while (count--) {
        ledger_sample(s, tick++, raw);
        assert(ledger_valid(s));
    }
}
static void truth_table(void)
{
    for (unsigned a=0;a<16;++a) for (unsigned b=0;b<16;++b) {
        Ledger s; ledger_init(&s); tick=0;
        hold(&s,a,10); hold(&s,b,10);
        unsigned allowed=b&3, effective=b>>2;
        assert(s.excess==(effective&~allowed&3));
        assert(s.denied==(allowed&~effective&3));
        for(unsigned i=0;i<2;++i) {
            unsigned state=((b>>i)&1)|(((b>>(i+2))&1)<<1);
            assert(s.bins[4*i+state]>=7);
        }
    }
}
static void chatter_and_gaps(void)
{
    Ledger s; ledger_init(&s); tick=0;
    for(unsigned i=0;i<100;++i) hold(&s,4|((i&1)<<1),1);
    assert(s.valid==13 && s.excess==1 && s.episodes[0]==1);
    assert(s.bins[2]==97 && s.bins[4]==0);
    assert(s.streak[0]==97 && s.maximum[0]==97);
    tick+=20; hold(&s,4,4);
    assert(s.gaps==1 && s.missed==20 && s.episodes[0]==2);
    assert(s.streak[0]==1 && s.maximum[0]==97);
    hold(&s,0,10); assert(s.excess==0 && s.streak[0]==0);
    hold(&s,4,10); assert(s.episodes[0]==3);
}
static void boundaries(void)
{
    Ledger s; ledger_init(&s); tick=UINT32_MAX-2;
    hold(&s,4,4); assert(s.streak[0]==1);
    Ledger old=s; ledger_sample(&s,s.now,0);
    assert(!memcmp(&s,&old,sizeof s));
    hold(&s,0,1); hold(&s,4,4);
    assert(s.episodes[0]==1); /* Rejected low glitch does not close episode. */
    s.bins[2]=UINT32_MAX; s.maximum[0]=s.streak[0]=UINT32_MAX;
    assert(ledger_valid(&s));
    old=s; hold(&s,4,1);
    assert(s.halted && (s.faults&MC_EXHAUSTED));
    assert(s.bins[2]==old.bins[2] && s.episodes[0]==old.episodes[0]);
    assert(!s.valid && !s.excess && !s.streak[0]);
    old=s; hold(&s,15,20); assert(s.bins[2]==old.bins[2]);
    ledger_init(&s); ledger_halt(&s,MC_WATCHDOG); assert(ledger_valid(&s));
}
static void wire_checks(void)
{
    Ledger s; ledger_init(&s); tick=0; hold(&s,4,20);
    char out[768]; size_t n=wire_status(out,sizeof out,&s);
    assert(n>0 && out[n-1]=='\n');
    assert(!wire_status(out,4,&s)); assert(!wire_status(NULL,768,&s));
    assert(!wire_status(out,sizeof out,NULL));
    s.episodes[0]=0; assert(!wire_status(out,sizeof out,&s));
    uint16_t crc; assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1);
    assert(crc16_verify("123456789",9,crc));
    assert(!crc16_buffer(NULL,1,&crc));
}
static void independent_oracle(void)
{
    /* Paced random plateaus: independently count every resulting pair state. */
    Ledger s; ledger_init(&s); tick=0;
    uint32_t random=19, bins[8]={0}, episodes[4]={0};
    unsigned previous=0; bool established=false;
    for(unsigned turn=0;turn<10000;++turn) {
        random=random*1664525u+1013904223u; unsigned raw=random>>28;
        for(unsigned t=0;t<10;++t) {
            hold(&s,raw,1);
            if(turn==0 && t<3) continue;
            unsigned v=t<3 && established?previous:raw;
            for(unsigned lane=0;lane<2;++lane) {
                unsigned state=((v>>lane)&1)|(((v>>(lane+2))&1)<<1);
                bins[4*lane+state]++;
            }
        }
        for(unsigned lane=0;lane<2;++lane) {
            unsigned state=((raw>>lane)&1)|(((raw>>(lane+2))&1)<<1);
            unsigned prev=((previous>>lane)&1)|(((previous>>(lane+2))&1)<<1);
            if(state==2 && (!established || prev!=2)) episodes[lane]++;
            if(state==1 && (!established || prev!=1)) episodes[2+lane]++;
        }
        previous=raw; established=true;
        assert(!memcmp(bins,s.bins,sizeof bins));
        assert(!memcmp(episodes,s.episodes,sizeof episodes));
    }
}
int main(void)
{
    truth_table(); chatter_and_gaps(); boundaries(); wire_checks(); independent_oracle();
    puts("engine: 256 transitions, independent chatter, gaps, wrap, exhaustion, CRC, 10000-plateau oracle passed");
}
