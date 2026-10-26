/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Witness w;
static uint32_t t;
static void step(unsigned raw, unsigned count)
{
    while (count--) witness_sample(&w, ++t, (uint8_t)raw);
}
static void start(unsigned raw)
{
    witness_init(&w);
    t = 0;
    step(raw, 10);
}
int main(void)
{
    start(2); /* DENY only: valid denial, no output. */
    assert(w.valid == 15 && w.unsafe == 0 && w.invalid == 0);
    step(1, 10);
    step(5, 10);
    assert(w.unsafe == 0); /* Established allow before enable. */
    step(7, 30); /* Invalid 11 code and enabled. */
    assert(w.invalid == 1 && w.unsafe == 1 && w.silent == 1);
    assert(w.invalid_ms >= 20 && w.max_invalid == w.invalid_ms);
    step(15, 10); /* Fault now reported, still enabled. */
    assert(w.silent == 1 && w.unsafe == 1);
    step(10, 10); /* Deny plus fault, output off. */
    assert(w.invalid_ms == 0 && w.unsafe_ms == 0);
    step(2, 10);
    step(5, 4); /* Simultaneous allow+enable is conservatively early. */
    assert(w.unsafe == 2);
    step(5, 10);
    assert(w.unsafe_ms == 0);
    start(0);
    step(0, 30);
    assert(w.invalid == 1 && w.silent == 1);
    uint32_t before = w.invalid;
    step(3, 10); /* 00 -> 11 is still one continuous invalid episode. */
    assert(w.invalid == before);
    step(11, 10);
    assert(w.silent_ms == 0);
    witness_sample(&w, t + 10, 11);
    t += 10;
    assert(w.valid == 0 && w.gaps == 1 && w.invalid_ms == 0);
    step(11, 5);
    assert(w.valid == 15 && w.invalid == before + 1);
    start(2);
    /* Chatter on FAULT cannot hide sustained invalid rails. */
    for (unsigned i = 0; i < 40; ++i) step((i & 1) ? 8 : 0, 1);
    assert(w.invalid == 1 && w.silent == 1);
    start(2);
    t = UINT32_MAX - 10;
    witness_sample(&w, t, 2);
    step(2, 30);
    assert(w.valid == 15 && w.now == 19);
    char frame[384];
    assert(wire_status(frame, sizeof frame, &w) > 0);
    assert(strncmp(frame, "DW1,S,", 6) == 0);
    assert(wire_status(frame, 5, &w) == 0);
    w.max_invalid = 0;
    w.invalid_ms = 1;
    assert(wire_status(frame, sizeof frame, &w) == 0);
    start(2);
    w.max_invalid = 1;
    assert(!witness_consistent(&w));
    start(2);
    w.max_unsafe = 1;
    assert(!witness_consistent(&w));
    start(2);
    w.silent = 1;
    w.faults = DW_SILENT;
    assert(!witness_consistent(&w));
    start(2);
    w.invalid = UINT32_MAX;
    step(0, 10);
    assert(w.halted && (w.faults & DW_EXHAUSTED));
    uint32_t frozen = w.invalid;
    step(2, 20);
    assert(w.invalid == frozen && w.halted);
    start(2);
    uint32_t sampled = w.now;
    witness_sample(&w, sampled, 15);
    assert(w.levels == 2); /* Same tick is not a second observation. */
    puts("engine: all assertions passed");
    return 0;
}
