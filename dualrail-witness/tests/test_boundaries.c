/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Witness w;
static uint32_t tick;
static char frame[384];
static void step(unsigned raw, unsigned n)
{
    while(n--) {
        witness_sample(&w,++tick,(uint8_t)raw);
        assert(witness_consistent(&w));
        assert(wire_status(frame,sizeof frame,&w));
    }
}
static void reset(unsigned raw)
{
    witness_init(&w); tick=0; step(raw,10);
}
static void halted(void)
{
    assert(w.halted && (w.faults&DW_EXHAUSTED));
    assert(witness_consistent(&w));
    assert(wire_status(frame,sizeof frame,&w));
}
int main(void)
{
    /* FAULT takes four observations to qualify; recovery at the exact
       threshold still charges the preceding interval. */
    for(unsigned endpoint=19;endpoint<=21;++endpoint) {
        reset(0);
        while(w.silent_ms<endpoint-4) step(0,1);
        step(8,4);
        assert(w.silent_ms==0);
        assert(w.silent==(endpoint>=20));
        assert(w.max_invalid==endpoint);
    }
    reset(2);
    w.invalid=UINT32_MAX; w.faults|=DW_INVALID;
    step(0,4); halted();
    reset(2);
    w.unsafe=UINT32_MAX; w.faults|=DW_UNSAFE;
    step(6,4); halted();
    reset(0);
    w.silent=UINT32_MAX; w.faults|=DW_SILENT; w.max_invalid=20;
    step(0,30); halted();
    reset(0);
    w.invalid_ms=w.max_invalid=UINT32_MAX;
    step(0,1); halted();
    reset(6);
    w.unsafe_ms=w.max_unsafe=UINT32_MAX;
    step(6,1); halted();
    reset(2);
    w.gaps=w.missed=UINT32_MAX; w.faults|=DW_GAP;
    witness_sample(&w,tick+2,2); halted();
    reset(2);
    w.missed=UINT32_MAX;
    witness_sample(&w,tick+2,2); halted();
    reset(2);
    w.tx_fail=UINT32_MAX; w.faults|=DW_TX;
    witness_tx_failure(&w); halted();
    /* Deterministic mixed vectors stress invariants after every tick. */
    reset(2);
    uint32_t rng=0x12345678u;
    for(unsigned i=0;i<10000;++i) {
        rng=rng*1664525u+1013904223u;
        step((rng>>20)&15u,1+(rng&31u));
        if((i%91u)==0) {
            witness_sample(&w,tick+7,(uint8_t)(rng&15u)); tick+=7;
            assert(witness_consistent(&w));
        }
    }
    puts("boundaries: endpoints, valid-state overflow, mixed vectors passed");
}
