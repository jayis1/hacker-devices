/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while(n--) {
        ledger_sample(s,tick++,(uint8_t)raw);
        assert(ledger_valid(s));
    }
}
static void fresh(Ledger *s) { tick=0; ledger_init(s); }
int main(void)
{
    Ledger s;
    fresh(&s);
    hold(&s,6,20); /* Already-high authorized CLEAR is not an edge. */
    assert(!s.known && !s.clears);
    hold(&s,4,10); hold(&s,6,10);
    assert(s.known && !s.expected && s.clears==1);
    hold(&s,9,10); /* Trip and latch together; trip has precedence. */
    assert(s.expected && s.sets==1 && !s.missing);
    hold(&s,8,10); hold(&s,0,10);
    assert(s.expected && s.missing && s.episodes[0]==1);
    hold(&s,2,10); /* Unauthorized clear may not erase obligation. */
    assert(s.expected && s.rejected==1 && s.clears==1);
    hold(&s,6,10); /* AUTH arriving under held clear is not a fresh edge. */
    assert(s.expected && s.clears==1);
    hold(&s,4,10); hold(&s,6,10);
    assert(!s.expected && s.clears==2 && !s.missing);
    hold(&s,14,10);
    assert(s.spurious && s.episodes[1]==1);
    hold(&s,5,10); hold(&s,7,10);
    assert(s.expected && s.rejected==2); /* Set dominates authorized clear. */
    uint32_t coverage=s.coverage;
    tick+=10; hold(&s,8,10);
    assert(!s.known && s.gaps==1 && s.missed==10 && s.coverage==coverage);
    hold(&s,9,10);
    assert(s.known && s.expected);
    Ledger before=s;
    ledger_sample(&s,s.now,0);
    assert(!memcmp(&s,&before,sizeof s));
    fresh(&s); tick=UINT32_MAX-5;
    hold(&s,9,20);
    assert(s.known && !s.gaps);
    s.coverage=UINT32_MAX;
    before=s; hold(&s,9,1);
    assert(s.halted && s.coverage==before.coverage && !s.known);
    assert(s.faults&ST_EXHAUSTED);
    char out[768];
    assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,4,&s));
    assert(!wire_status(NULL,sizeof out,&s));
    assert(crc16_verify("123456789",9,0x29b1));
    s.faults=0; assert(!ledger_valid(&s));
    fresh(&s); hold(&s,9,10);
    s.coverage=UINT32_MAX-1;
    s.totals[0]=UINT32_MAX-1;
    s.episodes[0]=1;
    s.maximum[0]=UINT32_MAX-1;
    s.faults|=ST_MISSING;
    assert(ledger_valid(&s));
    /* A new missing episode attempts several updates after qualification. */
    s.coverage=UINT32_MAX;
    before=s;
    hold(&s,0,1);
    assert(s.halted && s.totals[0]==before.totals[0]);
    fresh(&s); hold(&s,9,10);
    s.missed=UINT32_MAX; s.gaps=1; s.faults|=ST_GAP;
    assert(ledger_valid(&s)); before=s;
    tick+=2; hold(&s,0,1);
    assert(s.halted && s.gaps==before.gaps && s.missed==before.missed);
    assert(wire_status(out,sizeof out,&s));
    fresh(&s); hold(&s,9,10);
    s.totals[0]=3; s.episodes[0]=2; s.maximum[0]=1;
    s.faults|=ST_MISSING;
    assert(!ledger_valid(&s));
    /* Independent stable-vector oracle enumerates all 256 transitions. */
    for(unsigned first=0;first<16;++first) {
        for(unsigned second=0;second<16;++second) {
            fresh(&s); hold(&s,first,8);
            bool known=(first&1)!=0, expected=known;
            bool edge=(second&2) && !(first&2);
            bool accepted=edge && (first&4) && (second&4) && !(second&1);
            if(second&1) { known=true; expected=true; }
            else if(accepted) { known=true; expected=false; }
            hold(&s,second,8);
            assert(s.known==known && s.expected==expected);
            assert(s.rejected==(unsigned)(edge&&!accepted));
            assert(s.missing==(unsigned)(known&&expected&&!(second&8)));
            assert(s.spurious==(unsigned)(known&&!expected&&(second&8)));
        }
    }
    /* Unrelated chatter after baseline cannot suppress a retained duty. */
    fresh(&s); hold(&s,9,10); hold(&s,0,10);
    for(unsigned i=0;i<100;++i) hold(&s,(i&1)?4:0,1);
    assert(s.missing && s.episodes[0]==1 && s.streak[0]>100);
    /* A newly qualified authorization at the edge is deliberately too late. */
    fresh(&s); hold(&s,0,10); hold(&s,6,10);
    assert(!s.known && s.rejected==1);
    puts("engine: sticky obligation, clear provenance, all 256 transitions, gaps, wrap, exhaustion, CRC passed");
    return 0;
}
