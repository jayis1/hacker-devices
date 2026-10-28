/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static Mailbox s;
static uint32_t tick;
static void run(unsigned raw, unsigned count)
{
    while (count--) {
        mailbox_sample(&s, ++tick, raw);
        assert(mailbox_valid(&s));
    }
}
static void reset(unsigned raw)
{
    tick = 0;
    mailbox_init(&s);
    run(raw, 30);
    assert(s.ready);
}
static void happy(void)
{
    for (unsigned initial=0; initial<2; ++initial) {
        reset(initial ? 12 : 0);
        unsigned phase = initial;
        for (unsigned word=0; word<4; ++word) {
            run(word | (phase ? 12 : 0), 12);
            phase ^= 1;
            unsigned pending = word | (phase << 2) | ((phase ^ 1) << 3);
            run(pending, 10);
            assert(s.active && s.captured == word);
            run(word | (phase ? 12 : 0), 10);
            assert(!s.active && s.completed == word+1 && !s.faults);
        }
        assert(s.starts == 4 && s.acks == 4);
    }
}
int main(void)
{
    happy();
    reset(0);
    run(4, 10); run(5, 1); /* raw payload excursion must not be hidden by debounce */
    assert(s.hold == 1 && s.abandoned == 1 && !s.ready);
    run(0, 100); assert(!s.ready); /* no silent resynchronization */
    reset(0); run(8, 4);
    assert(s.orphan == 1 && s.acks == 1);
    reset(0); run(12, 4);
    assert(s.overlap == 1 && s.starts == 1 && s.abandoned == 1);
    reset(0); run(4, 10); run(0, 4);
    assert(s.overtake == 1 && s.starts == 2 && s.abandoned == 2);
    mailbox_init(&s); tick=0; run(4,50);
    assert(!s.ready && !s.starts); /* late attach cannot invent request */
    run(12,30); assert(s.ready && !s.starts);
    reset(0); run(4,10); run(12,3); run(13,1);
    assert(s.hold == 1 && s.acks == 1 && !s.completed);
    reset(0);
    for (unsigned i=0;i<4;++i) run(8u | (i&1u),1);
    assert(s.orphan == 1); /* data chatter cannot mask ACK qualification */
    reset(0); run(4,3); run(0,10);
    assert(!s.starts && s.ready); /* rejected request glitch */
    reset(0); run(5, 4);
    assert(s.setup_fail == 1 && s.starts == 1);
    reset(0); run(4, 4); run(4, TM_DEADLINE);
    assert(s.active && s.age == TM_DEADLINE);
    run(4, 1); assert(s.timeouts == 1 && s.abandoned == 1);
    reset(0); run(4, 4); run(4, TM_DEADLINE-4); run(12, 4);
    assert(s.completed == 1 && !s.timeouts); /* inclusive exact deadline */
    reset(0); run(4, 4); run(4, TM_DEADLINE-3); run(12, 4);
    assert(s.timeouts == 1 && s.acks == 1 && !s.completed);
    reset(0); run(4, 10);
    mailbox_sample(&s, tick+5, 4); tick+=5;
    assert(s.gaps == 1 && s.missed == 4 && s.abandoned == 1 && mailbox_valid(&s));
    reset(0); s.now=UINT32_MAX-2; tick=s.now;
    run(0, 8); assert(!s.gaps && s.ready);
    reset(0); s.starts=UINT32_MAX; s.completed=UINT32_MAX; s.acks=UINT32_MAX; s.max_latency=1;
    run(4,4); assert(s.halted && (s.faults & TM_EXHAUSTED) && mailbox_valid(&s));
    reset(0); s.starts=UINT32_MAX-1; s.completed=UINT32_MAX-1; s.acks=UINT32_MAX-1; s.max_latency=1;
    run(4,10); run(0,4);
    assert(s.halted && s.abandoned == 1 && mailbox_valid(&s));
    reset(0); mailbox_halt(&s, TM_WATCHDOG); assert(mailbox_valid(&s));
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,2,&s));
    s.completed++; assert(!wire_status(out,sizeof out,&s));
    reset(0); run(4,10); s.captured=1;
    assert(!mailbox_valid(&s) && !wire_status(out,sizeof out,&s));
    /* Independent randomized invariant stress across fresh captures. */
    uint32_t rng=123;
    for (unsigned trial=0; trial<100; ++trial) {
        reset(0);
        for (unsigned i=0; i<1000; ++i) {
            rng=rng*1664525u+1013904223u;
            run(rng>>28,1);
        }
    }
    puts("PASS toggle handshake, both phases, payload hold, deadlines, loss, overflow, invariants");
}
