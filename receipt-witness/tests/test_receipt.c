/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "receipt.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Receipt r;
static void run(unsigned n, uint8_t bits)
{
    while (n--) receipt_tick(&r, r.time + 1u, bits);
}
static void reset(void)
{
    receipt_init(&r, 0);
    run(30, 0);
    assert(r.ready);
}
static void pulse(uint8_t bits)
{
    run(5, bits);
    run(5, 0);
}
int main(void)
{
    reset();
    pulse(ACTION); pulse(APPEND); pulse(DURABLE);
    assert(r.actions == 1 && r.appended == 1 && r.receipts == 1);
    assert(!r.faults && !r.count);
    reset(); pulse(ACTION | APPEND);
    assert(r.count == 1 && r.appended == 1 && !r.receipts);
    pulse(DURABLE); assert(r.receipts == 1 && !r.faults);
    reset(); pulse(ACTION | APPEND | DURABLE);
    assert(r.faults & F_ORPHAN_DURABLE);
    assert(!r.ready && !r.receipts);
    assert(r.actions == 1 && r.abandoned == 1);
    reset(); pulse(ACTION); run(41,0);
    run(4,ACTION | BLOCKED);
    assert(r.faults & F_APPEND_LATE);
    assert(r.faults & F_BLOCKED_ACTION);
    assert(r.actions == 2 && r.abandoned == 2);
    reset(); pulse(APPEND);
    assert(r.faults & F_ORPHAN_APPEND);
    reset(); pulse(DURABLE);
    assert(r.faults & F_ORPHAN_DURABLE);
    reset(); pulse(ACTION | BLOCKED);
    assert(r.faults & F_BLOCKED_ACTION);
    reset(); pulse(ACTION); run(60, 0);
    assert(r.faults & F_APPEND_LATE);
    assert(r.abandoned == 1 && !r.ready);
    /* Recovery needs a fresh all-low baseline after invalidation. */
    run(30, 0); assert(r.ready && !r.count);
    reset(); pulse(ACTION | APPEND); run(210, 0);
    assert(r.faults & F_DURABLE_LATE);
    reset();
    for (unsigned i=0;i<CAPACITY;i++) pulse(ACTION | APPEND);
    assert(r.count == CAPACITY);
    pulse(ACTION | APPEND);
    assert(r.faults & F_OVERFLOW);
    assert(r.abandoned == CAPACITY + 1 && !r.count);
    reset(); pulse(ACTION);
    receipt_tick(&r,r.time+7,0);
    assert(r.gaps == 1 && r.missed == 6 && r.abandoned == 1);
    assert(!r.ready && (r.faults & F_GAP));
    run(30,0); assert(r.ready);
    /* Held pulse creates one action, not one per sample. */
    reset(); run(40,ACTION | APPEND); assert(r.actions == 1);
    run(5,0); pulse(DURABLE); assert(r.receipts == 1);
    /* Independent qualification cannot be suppressed by BLOCKED chatter. */
    reset();
    for (unsigned i=0;i<10;i++) run(1,ACTION | ((i&1)?BLOCKED:0));
    assert(r.actions == 1);
    assert(r.faults & F_BLOCKED_ACTION);
    /* Timer wrap is a continuous sample, not a gap. */
    receipt_init(&r,UINT32_MAX-15u); run(30,0);
    pulse(ACTION | APPEND); pulse(DURABLE);
    assert(!r.gaps && r.receipts == 1);
    assert(receipt_add(UINT32_MAX-1,5)==UINT32_MAX);
    /* Short pulses do not become qualified actions. */
    reset(); run(2,ACTION); run(10,0); assert(!r.actions);
    /* Inclusive append deadline: qualified edge at age 50 succeeds. */
    reset(); run(4,ACTION); run(46,0); run(4,APPEND);
    assert(r.appended == 1 && !(r.faults & F_APPEND_LATE));
    reset(); run(4,ACTION); run(47,0); run(4,APPEND);
    assert(r.faults & F_APPEND_LATE);
    /* Oldest receipt cannot skip an action whose append is missing. */
    reset(); pulse(ACTION); pulse(DURABLE);
    assert(r.faults & F_ORPHAN_DURABLE);
    char out[384];
    /* Inclusive durable deadline, then a tick beyond it. */
    reset(); run(4,ACTION | APPEND); run(196,0); run(4,DURABLE);
    assert(r.receipts == 1 && r.max_durable == 200 && !r.faults);
    reset(); run(4,ACTION | APPEND); run(197,0); run(4,DURABLE);
    assert(!r.receipts && (r.faults & F_DURABLE_LATE));
    /* Multiple pending operations, FIFO wrap and same-tick replacement. */
    reset();
    for (unsigned i=0;i<8;i++) pulse(ACTION | APPEND);
    pulse(DURABLE | ACTION | APPEND);
    assert(r.receipts == 1 && r.count == 8 && !r.faults);
    for (unsigned i=0;i<8;i++) pulse(DURABLE);
    assert(r.receipts == 9 && !r.count && !r.faults);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(!wire_status(out,3,&r,0));
    assert(wire_status(out,sizeof out,&r,0));
    assert(strncmp(out,"RW1,S,",6)==0);
    assert(wire_policy(out,sizeof out));
    assert(strncmp(out,"RW1,P,8,50,200,3,20*",19)==0);
    puts("Receipt policy: ordering, deadlines, capacity, gaps, wrap, qualification, CRC PASS");
    return 0;
}
