/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static void hold(Ledger *s, unsigned v, unsigned n)
{
    while (n--) {
        ledger_sample(s, s->now + 1, (uint8_t)v);
        assert(ledger_valid(s));
    }
}
static void ready(Ledger *s, unsigned value)
{
    ledger_init(s);
    hold(s, value | 8, 8);
    hold(s, value, 8);
    assert(s->state == SF_OPEN);
}
int main(void)
{
    Ledger s;
    char out[768];
    ledger_init(&s);
    hold(&s, 5, 20);
    assert(s.state == SF_UNKNOWN && !s.seals);
    for (unsigned a = 0; a < 4; ++a) {
        for (unsigned b = 0; b < 4; ++b) {
            ready(&s, a);
            hold(&s, a | 4, 8);
            assert(s.state == SF_SEALED && s.captured == a && s.seals == 1);
            hold(&s, b | 4, 8);
            assert(!!s.totals[0] == (a != b));
            hold(&s, b, 8);
            assert(s.unsealed && s.totals[1]);
            assert(s.captured == a);
            hold(&s, a | 4, 8);
            assert(!s.mismatch && !s.unsealed);
            assert(s.captured == a && s.seals == 1);
            hold(&s, b | 8, 8);
            assert(s.state == SF_UNKNOWN);
            hold(&s, b, 8);
            hold(&s, b | 4, 8);
            assert(s.seals == 2 && s.captured == b);
        }
    }
    ready(&s, 0);
    hold(&s, 5, 8); /* simultaneous payload and seal is not established setup */
    assert(s.rejected == 1 && s.state == SF_UNKNOWN);
    hold(&s, 0, 8);
    hold(&s, 4, 8);
    assert(!s.seals); /* silence cannot repair lost provenance */
    ready(&s, 0);
    hold(&s, 4, 8);
    hold(&s, 12, 8); /* reset with seal high still ends the contract */
    assert(s.state == SF_UNKNOWN && s.resets == 2);
    hold(&s, 4, 8);
    hold(&s, 0, 8);
    hold(&s, 4, 8);
    assert(s.state == SF_UNKNOWN && s.seals == 1);
    ready(&s, 0);
    hold(&s, 12, 8); /* reset/SEAL coexistence is not a usable baseline */
    hold(&s, 4, 8);
    assert(s.state == SF_UNKNOWN);
    ready(&s, 0);
    hold(&s, 4, 3); /* initial candidate plus only two intervals */
    assert(s.state == SF_OPEN);
    hold(&s, 4, 1);
    assert(s.state == SF_SEALED && s.coverage == 1);
    ready(&s, 1);
    hold(&s, 5, 8);
    ledger_sample(&s, s.now + 10, 5);
    hold(&s, 5, 8);
    assert(s.gaps == 1 && s.missed == 9 && s.state == SF_UNKNOWN);
    ready(&s, 0);
    hold(&s, 4, 8);
    /* Chattering CFG1 must not hide a qualified CFG0 deviation. */
    for (unsigned i=0; i<30; ++i) hold(&s, 5 | ((i&1)*2), 1);
    assert(s.totals[0] && s.state == SF_SEALED);
    ready(&s, 0);
    s.now = UINT32_MAX - 2;
    hold(&s, 0, 8);
    assert(!s.gaps);
    s.seals = UINT32_MAX; /* exhaustion must not partially capture */
    s.resets = UINT32_MAX;
    s.coverage = UINT32_MAX;
    hold(&s, 4, 8);
    assert(s.halted && (s.faults & SF_EXHAUSTED));
    assert(s.seals == UINT32_MAX);
    assert(wire_status(out, sizeof out, &s));
    ledger_init(&s);
    ledger_halt(&s, SF_WATCHDOG);
    assert(ledger_valid(&s));
    assert(wire_status(out, sizeof out, &s));
    assert(!wire_status(out, 2, &s));
    assert(!wire_status(NULL, 0, &s));
    s.state = SF_SEALED;
    assert(!ledger_valid(&s));
    /* Exhaustive stable-vector transitions preserve output invariants. */
    for (unsigned a=0; a<16; ++a) for (unsigned b=0; b<16; ++b) {
        ready(&s, 0); hold(&s, a, 8); hold(&s, b, 8);
        assert(wire_status(out,sizeof out,&s));
    }
    /* Long varied traces exercise temporal state beyond two-vector tests. */
    unsigned random = 1937;
    ledger_init(&s);
    for (unsigned i=0; i<100000; ++i) {
        random = random * 1664525u + 1013904223u;
        hold(&s, random >> 28, 1 + ((random >> 20) & 15));
        assert(wire_status(out,sizeof out,&s));
    }
    puts("PASS seal provenance, 16 payload pairs, 256 vectors, gaps, chatter, wrap, overflow, encoder");
}
