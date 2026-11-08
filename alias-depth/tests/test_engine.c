/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static unsigned words;
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, ++clock_ms, raw);
        assert(ledger_valid(s));
    }
}
static void start(Ledger *s)
{
    clock_ms = 0;
    ledger_init(s);
    hold(s, 0, 20);
}
static void op(Ledger *s, unsigned tag, bool acquire)
{
    hold(s, tag, 12);
    hold(s, tag | (acquire ? 4u : 8u), 6);
    hold(s, tag, 6);
}
/* Independent oracle uses actual handle tokens, not a reference counter. */
typedef struct { bool handle[4][3]; } Oracle;
static unsigned count(const Oracle *o, unsigned tag)
{
    unsigned n = 0;
    for (unsigned i = 0; i < 3; ++i) n += o->handle[tag][i];
    return n;
}
static void enumerate(Ledger state, Oracle oracle, unsigned depth)
{
    if (!depth || state.halted) return;
    for (unsigned action = 0; action < 8; ++action) {
        Ledger next = state;
        Oracle expected = oracle;
        unsigned tag = action % 4, reason = 0, n = count(&oracle, tag);
        if (action < 4) {
            if (n == 3) reason = 1;
            else expected.handle[tag][n] = true;
        } else {
            if (!n) reason = 2;
            else expected.handle[tag][n - 1] = false;
        }
        clock_ms = next.now;
        op(&next, tag, action < 4);
        assert(next.faults == reason);
        unsigned total = 0;
        for (unsigned j = 0; j < 4; ++j) {
            unsigned want = count(&expected, j);
            assert(next.refs[j] == want);
            assert(!!(next.active & (1u << j)) == !!want);
            assert(ledger_eligible(&next, j) == (want < 3));
            total += want;
        }
        assert(next.depth == total);
        ++words;
        enumerate(next, expected, depth - 1);
    }
}
int main(void)
{
    Ledger s;
    start(&s);
    Oracle empty = {{{false}}};
    enumerate(s, empty, 7);
    /* The selected object remains live until its last reference returns. */
    for (unsigned tag = 0; tag < 4; ++tag) {
        start(&s);
        for (unsigned i = 0; i < 3; ++i) op(&s, tag, true);
        assert(!s.halted && s.refs[tag] == 3 && s.depth == 3);
        op(&s, tag, false);
        assert(s.refs[tag] == 2 && s.active == (1u << tag));
        op(&s, tag, true);
        assert(!s.halted && s.uses[tag] == 4);
        op(&s, tag, true);
        assert(s.faults == 1 && s.refs[tag] == 3);
    }
    start(&s);
    for (unsigned tag = 0; tag < 4; ++tag)
        for (unsigned i = 0; i < 3; ++i) op(&s, tag, true);
    assert(s.depth == 12 && s.peak == 12 && !s.halted);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned tag = 0; tag < 4; ++tag) op(&s, tag, false);
    assert(!s.depth && s.rounds == 1 && s.completed == 12);
    op(&s, 2, true); op(&s, 2, false);
    assert(s.rounds == 2 && s.uses[2] == 4);
    op(&s, 2, false); assert(s.faults == 2);
    start(&s); hold(&s, 12, 6); assert(s.faults == 8 && s.rejected == 2);
    start(&s); ledger_sample(&s, s.now + 2, 0); assert(s.faults == 16 && ledger_valid(&s));
    Ledger frozen = s; clock_ms = s.now; hold(&s, 15, 20); frozen.now = s.now;
    assert(!memcmp(&s, &frozen, sizeof s));
    start(&s); hold(&s, 4, 3); s.observed = UINT32_MAX; hold(&s, 4, 1);
    assert(s.faults == 32 && !s.attempts && !s.refs[0]);
    start(&s); hold(&s, 4, 3); hold(&s, 12, 1); assert(s.faults == 8);
    start(&s); hold(&s, 4, 3); hold(&s, 5, 1); assert(s.faults == 8);
    start(&s); Ledger old = s; ledger_sample(&s, s.now, 15); assert(!memcmp(&s, &old, sizeof s));
    s.now = UINT32_MAX; clock_ms = UINT32_MAX; hold(&s, 0, 1); assert(!s.halted);
    char out[768]; assert(wire_status(out, sizeof out, &s));
    assert(!wire_status(out, 3, &s) && !wire_status(NULL, 100, &s));
    start(&s); op(&s, 0, true); s.refs[0] = 0; assert(!ledger_valid(&s));
    start(&s); s.halted = 1; s.valid = 0; s.faults = 1; s.rejected = 1; s.attempts = 1;
    assert(!ledger_valid(&s)); /* Ceiling finding requires three retained refs. */
    start(&s); op(&s, 0, true); s.halted = 1; s.valid = 0;
    s.faults = 2; s.rejected = 1; ++s.attempts;
    assert(!ledger_valid(&s)); /* Underflow cannot name a live object. */
    printf("engine: %u independent handle-token oracle transitions PASS\n", words);
    puts("engine: multiplicity, boundaries, qualification, freeze, wrap, encoder PASS");
}
