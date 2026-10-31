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
static void operation(Ledger *s, unsigned slot, unsigned client, bool allocate)
{
    unsigned tag = slot | (client << 1);
    hold(s, tag, 12);
    hold(s, tag | (allocate ? 4 : 8), 8);
    hold(s, tag, 8);
}
static Ledger baseline(void)
{
    Ledger s;
    ledger_init(&s);
    hold(&s, 0, 20);
    assert(s.phase == OB_TRACK);
    return s;
}
/* Independent oracle: two array entries, -1 means vacant. A client may have
 * only one entry. No knowledge of packed firmware representation is needed. */
static unsigned paths;
static void explore(Ledger s, const int owners[2], unsigned depth)
{
    if (!depth) return;
    for (unsigned slot = 0; slot < 2; ++slot)
    for (unsigned client = 0; client < 2; ++client)
    for (unsigned alloc = 0; alloc < 2; ++alloc) {
        Ledger next = s;
        unsigned fault = 0;
        if (alloc) {
            if (owners[slot] >= 0) fault = OB_DUPLICATE;
            else if (owners[slot ^ 1u] == (int)client) fault = OB_BINDING;
        } else {
            if (owners[slot] < 0) fault = OB_ORPHAN;
            else if (owners[slot] != (int)client) fault = OB_BINDING;
        }
        operation(&next, slot, client, alloc);
        assert(next.faults == fault);
        ++paths;
        if (!fault) {
            int copy[2] = {owners[0], owners[1]};
            copy[slot] = alloc ? (int)client : -1;
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
    const int empty[2] = {-1, -1};
    explore(s, empty, 7);
    /* Same client reuse, including migration to the other slot, is legal. */
    operation(&s, 0, 0, true); operation(&s, 0, 0, false);
    operation(&s, 0, 0, true); operation(&s, 0, 0, false);
    operation(&s, 1, 0, true); operation(&s, 1, 0, false);
    assert(!s.halted && s.rounds == 3);
    /* Two different slots cannot simultaneously belong to the same client. */
    operation(&s, 0, 1, true); operation(&s, 1, 1, true);
    assert(s.faults == OB_BINDING && s.active == 1);
    Ledger frozen = s; hold(&s, 15, 50);
    frozen.now = s.now; assert(!memcmp(&s, &frozen, sizeof s));
    /* A wrong-client release must preserve the rightful binding. */
    s = baseline(); operation(&s, 1, 1, true); operation(&s, 1, 0, false);
    assert(s.faults == OB_BINDING && s.active == 2 && s.completed == 0);
    for (unsigned first = 0; first < 2; ++first) {
        s = baseline(); operation(&s, 0, 0, true); operation(&s, 1, 1, true);
        operation(&s, first, first, false);
        operation(&s, first ^ 1, first ^ 1, false);
        assert(!s.faults && s.rounds == 1 && s.queued == 1 && s.peak == 2);
    }
    s = baseline(); hold(&s, 12, 8);
    assert(s.faults == OB_INTERFACE && s.rejected == 2);
    s = baseline(); ledger_sample(&s, s.now + 3, 0);
    assert(s.faults == OB_GAP && s.missed == 2 && ledger_valid(&s));
    s = baseline(); Ledger same = s; ledger_sample(&s, s.now, 15);
    assert(!memcmp(&s, &same, sizeof s));
    s.now = UINT32_MAX; ledger_sample(&s, 0, 0); assert(!s.faults);
    s = baseline(); s.observed = UINT32_MAX; ledger_sample(&s, s.now + 1, 0);
    assert(s.faults == OB_EXHAUSTED && s.observed == UINT32_MAX);
    s = baseline(); hold(&s, 4, 2); hold(&s, 0, 10); assert(!s.attempts);
    hold(&s, 4, 8); assert(s.launched == 1);
    hold(&s, 4, 30); assert(s.launched == 1);
    s = baseline(); hold(&s, 6, 8); assert(s.faults == OB_INTERFACE);
    /* An unqualified raw opposite strobe still makes this edge ambiguous. */
    s = baseline(); hold(&s, 4, 3); hold(&s, 12, 1);
    assert(s.faults == OB_INTERFACE && s.rejected == 1);
    /* Client chatter cannot defer the independently qualifying operation. */
    s = baseline();
    for (unsigned i = 0; i < 8; ++i) hold(&s, 4 | ((i & 1u) << 1), 1);
    assert(s.faults == OB_INTERFACE);
    /* Exhaustion at the release endpoint must not clear a live binding. */
    s = baseline(); operation(&s, 0, 0, true);
    s.launched = 2147483648u;
    s.completed = s.rounds = 2147483647u;
    s.attempts = UINT32_MAX;
    s.observed = UINT32_MAX - 100u;
    assert(ledger_valid(&s));
    operation(&s, 0, 0, false);
    assert(s.faults == OB_EXHAUSTED && s.active == 1);
    assert(s.completed == 2147483647u && s.attempts == UINT32_MAX);
    char out[768]; size_t n = wire_status(out, sizeof out, &s); assert(n);
    assert(!wire_status(out, n - 1, &s)); assert(!wire_status(NULL, sizeof out, &s));
    assert(wire_boot(out, sizeof out, 0)); assert(wire_policy(out, sizeof out));
    uint16_t crc; assert(crc16_buffer("123456789", 9, &crc) && crc == 0x29b1);
    s = baseline(); s.active = 1; assert(!ledger_valid(&s));
    s = baseline(); operation(&s, 0, 0, true); operation(&s, 1, 1, true);
    s.order &= ~2u; assert(!ledger_valid(&s));
    assert(!wire_status(out, sizeof out, &s));
    printf("engine: %u oracle branches; binding, migration, reuse, wrap, gaps PASS\n", paths);
}
