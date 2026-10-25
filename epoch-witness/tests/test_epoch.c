/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "epoch.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void run(Epoch *e, unsigned n, uint8_t raw)
{
    while (n--) epoch_tick(e, e->time + 1u, raw);
}
static void init(Epoch *e)
{
    epoch_init(e, 0);
    run(e, 30, 0);
    assert(e->phase == READY);
}
static void commit(Epoch *e, uint8_t generation)
{
    run(e, 15, generation);
    run(e, 15, generation | INPUT_COMMIT);
    run(e, 10, generation);
}
int main(void)
{
    Epoch e;
    init(&e);
    commit(&e, 2);
    assert(e.valid && e.highest == 2 && e.accepted == 1);
    run(&e, 10, 2 | INPUT_EXECUTE);
    assert(!e.faults);
    run(&e, 10, 2);
    commit(&e, 1);
    assert(!e.valid && e.highest == 2 && e.downgrades == 1);
    assert(e.faults & FAULT_ROLLBACK);
    run(&e, 10, 1 | INPUT_EXECUTE);
    assert(e.faults & FAULT_EXECUTE);
    init(&e);
    commit(&e, 0);
    commit(&e, 0);
    assert(e.accepted == 2 && e.highest == 0);
    for (unsigned width=MIN_COMMIT_MS; width<=MAX_COMMIT_MS; width+=990) {
        init(&e);
        run(&e, 15, 1);
        run(&e, width, 1 | INPUT_COMMIT);
        run(&e, 10, 1);
        assert(e.valid && e.accepted == 1 && !e.faults);
    }
    init(&e);
    run(&e, 15, 1);
    run(&e, MAX_COMMIT_MS+1, 1 | INPUT_COMMIT);
    run(&e, 10, 1);
    assert(!e.valid && (e.faults & FAULT_TIMEOUT));
    init(&e);
    commit(&e, 2);
    run(&e, 1, 1); run(&e, 10, 2);
    assert(!e.valid); /* Returning to old data cannot reauthorize execution. */
    epoch_init(&e, 0);
    run(&e, 50, INPUT_COMMIT | INPUT_EXECUTE);
    assert(!e.accepted && e.phase == UNKNOWN && (e.faults & FAULT_STARTUP));
    init(&e);
    for (unsigned i=0;i<30;++i) run(&e,1,(i&1u)|INPUT_EXECUTE);
    assert(e.execute_ms > 0); /* Data chatter cannot hide execute. */
    init(&e);
    run(&e, 10, 3 | INPUT_COMMIT); /* No setup before edge. */
    run(&e, 10, 3);
    assert(!e.valid && (e.faults & FAULT_SETUP));
    init(&e);
    run(&e, 15, 1);
    run(&e, 10, 1 | INPUT_COMMIT);
    run(&e, 10, 2 | INPUT_COMMIT);
    run(&e, 10, 2);
    assert(!e.valid && (e.faults & FAULT_HOLD));
    init(&e);
    run(&e, 15, 1);
    run(&e, 5, 1 | INPUT_COMMIT);
    run(&e, 10, 1);
    assert(!e.valid && (e.faults & FAULT_WIDTH));
    init(&e);
    run(&e, 15, 1);
    run(&e, 15, 1 | INPUT_COMMIT);
    run(&e, 10, 1 | INPUT_EXECUTE); /* Completion and execute same sample. */
    assert(e.faults & FAULT_EXECUTE);
    init(&e);
    commit(&e, 3);
    epoch_tick(&e, e.time + 9u, 0);
    assert(!e.valid && e.highest == 3 && e.gaps == 1 && e.missed == 8);
    run(&e, 30, 0);
    commit(&e, 1);
    assert(e.downgrades == 1); /* Gap must not erase high water mark. */
    init(&e);
    run(&e, MAX_COMMIT_MS + 40u, INPUT_COMMIT);
    assert(e.faults & FAULT_TIMEOUT);
    assert(e.phase == UNKNOWN);
    run(&e, 30, 0);
    assert(e.phase == READY);
    epoch_init(&e, UINT32_MAX - 10u);
    run(&e, 30, 0);
    assert(!e.gaps && e.phase == READY);
    e.accepted = UINT32_MAX;
    commit(&e, 1);
    assert(e.accepted == UINT32_MAX);
    init(&e);
    for (unsigned i=0; i<100; ++i) commit(&e, 0);
    assert(e.dropped > 0);
    Event event;
    unsigned count=0;
    while (epoch_pop(&e, &event)) ++count;
    assert(count == EVENT_CAPACITY);
    assert(!epoch_pop(NULL, &event) && !epoch_pop(&e, NULL));
    assert(wire_crc("123456789", 9) == 0x29b1);
    char frame[384];
    assert(wire_status(frame, sizeof frame, &e, 0));
    assert(strncmp(frame, "EW1,S,", 6) == 0);
    for (unsigned n=0; n<20; ++n) assert(!wire_status(frame,n,&e,0));
    assert(!wire_status(NULL,384,&e,0));
    /* Deterministic noisy input checks invariants, not physical measurements. */
    init(&e);
    uint32_t rng=12345;
    uint8_t high=e.highest;
    for (unsigned i=0;i<100000;++i) {
        rng=rng*1664525u+1013904223u;
        epoch_tick(&e,e.time+((rng & 255u)==0 ? 2u:1u),(uint8_t)(rng>>24));
        assert(e.highest >= high && e.highest <= 3);
        assert(e.used <= EVENT_CAPACITY);
        assert(!e.valid || (e.have_highest && e.current == e.highest));
        high=e.highest;
    }
    puts("Epoch policy, loss, wrap, saturation, CRC and noisy-input invariants PASS");
}
