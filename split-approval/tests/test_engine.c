/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, s->now + 1, (uint8_t)raw);
        assert(ledger_valid(s));
    }
}
static void operation(Ledger *s, unsigned slot, unsigned actor, bool proposal)
{
    unsigned tag = slot | (actor << 1);
    hold(s, tag, 12);
    hold(s, tag | (proposal ? 4 : 8), 8);
    hold(s, tag, 8);
}
static Ledger baseline(void)
{
    Ledger s;
    ledger_init(&s);
    hold(&s, 0, 20);
    assert(s.phase == SP_TRACK);
    return s;
}
/* Independent array oracle: vacant or proposer; no cross-slot actor constraint. */
static unsigned paths;
static void explore(Ledger s, const int authors[2], unsigned depth)
{
    if (!depth) return;
    for (unsigned slot = 0; slot < 2; ++slot)
    for (unsigned actor = 0; actor < 2; ++actor)
    for (unsigned propose = 0; propose < 2; ++propose) {
        Ledger next = s;
        unsigned fault = 0;
        if (propose) {
            if (authors[slot] >= 0) fault = SP_DUPLICATE;
        } else {
            if (authors[slot] < 0) fault = SP_ORPHAN;
            else if (authors[slot] == (int)actor) fault = SP_SELF;
        }
        operation(&next, slot, actor, propose);
        assert(next.faults == fault);
        ++paths;
        if (!fault) {
            int copy[2] = {authors[0], authors[1]};
            copy[slot] = propose ? (int)actor : -1;
            for (unsigned i = 0; i < 2; ++i) {
                assert(!!(next.active & (1u << i)) == (copy[i] >= 0));
                if (copy[i] >= 0)
                    assert(((next.order >> i) & 1u) == (unsigned)copy[i]);
            }
            explore(next, copy, depth - 1);
        } else {
            assert(next.active == s.active && next.order == s.order);
        }
    }
}
int main(void)
{
    Ledger s = baseline();
    /* First acceptance: approval by the OTHER actor must succeed. */
    operation(&s, 0, 0, true); operation(&s, 0, 1, false);
    assert(!s.faults && s.completed == 1);
    const int empty[2] = {-1, -1};
    explore(baseline(), empty, 7);
    for (unsigned proposer = 0; proposer < 2; ++proposer)
    for (unsigned first = 0; first < 2; ++first) {
        s = baseline();
        operation(&s, 0, proposer, true); operation(&s, 1, proposer, true);
        operation(&s, first, proposer ^ 1, false);
        operation(&s, first ^ 1, proposer ^ 1, false);
        assert(!s.faults && s.rounds == 1 && s.queued == 1 && s.peak == 2);
        operation(&s, 0, proposer, true); operation(&s, 0, proposer, false);
        assert(s.faults == SP_SELF && s.active == 1 && s.completed == 2);
        Ledger frozen = s; hold(&s, 15, 50);
        frozen.now = s.now; assert(!memcmp(&s, &frozen, sizeof s));
    }
    s = baseline();
    for (unsigned i=0;i<4;++i) {
        operation(&s, 0, i&1, true); operation(&s, 0, (i&1)^1, false);
    }
    assert(!s.faults && s.rounds == 4);
    s = baseline(); hold(&s, 12, 8);
    assert(s.faults == SP_INTERFACE && s.rejected == 2);
    s = baseline(); ledger_sample(&s, s.now + 3, 0);
    assert(s.faults == SP_GAP && s.missed == 2 && ledger_valid(&s));
    s = baseline(); Ledger same = s; ledger_sample(&s, s.now, 15);
    assert(!memcmp(&s, &same, sizeof s));
    s.now = UINT32_MAX; ledger_sample(&s, 0, 0); assert(!s.faults);
    s = baseline(); s.observed = UINT32_MAX; ledger_sample(&s, s.now + 1, 0);
    assert(s.faults == SP_EXHAUSTED && s.observed == UINT32_MAX);
    s = baseline(); hold(&s, 4, 2); hold(&s, 0, 10); assert(!s.attempts);
    hold(&s, 4, 8); hold(&s, 4, 30); assert(s.launched == 1);
    s = baseline(); hold(&s, 6, 8); assert(s.faults == SP_INTERFACE);
    s = baseline(); hold(&s, 4, 3); hold(&s, 12, 1);
    assert(s.faults == SP_INTERFACE && s.rejected == 1);
    s = baseline();
    for (unsigned i=0;i<8;++i) hold(&s, 4|((i&1u)<<1), 1);
    assert(s.faults == SP_INTERFACE);
    s = baseline(); operation(&s, 0, 0, true);
    s.launched = 2147483648u;
    s.completed = s.rounds = 2147483647u;
    s.attempts = UINT32_MAX;
    s.observed = UINT32_MAX - 100u;
    assert(ledger_valid(&s));
    operation(&s, 0, 1, false);
    assert(s.faults == SP_EXHAUSTED && s.active == 1);
    assert(s.completed == 2147483647u && s.attempts == UINT32_MAX);
    char out[768]; size_t n=wire_status(out,sizeof out,&s); assert(n);
    assert(!wire_status(out,n-1,&s)); assert(!wire_status(NULL,sizeof out,&s));
    assert(wire_boot(out,sizeof out,0)); assert(wire_policy(out,sizeof out));
    uint16_t crc; assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1);
    s=baseline(); s.active=1; assert(!ledger_valid(&s));
    printf("engine: %u oracle branches; independent approval, reuse, gaps, exhaustion PASS\n",paths);
}
