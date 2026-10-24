/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Linux MMIO test: validates software behavior, not real USART hardware. */
#define _GNU_SOURCE
#include "board.h"
#include "registers.h"
#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
static uint32_t now;
uint32_t board_millis(void) { return now; }
int main(void)
{
    void *mapped=mmap((void *)0x40004000u,4096,PROT_READ|PROT_WRITE,
                     MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    assert(mapped!=MAP_FAILED);
    assert(!serial_submit(NULL,1));
    assert(!serial_submit("",0));
    assert(!serial_submit("x",SERIAL_BUFFER+1));
    assert(serial_submit("abc",3));
    assert(!serial_submit("x",1));
    USART2_SR=0;
    serial_poll();
    assert(serial_busy());
    USART2_SR=1u<<7;
    for(unsigned i=0;i<3;++i) {
        serial_poll();
        assert(USART2_DR==(uint32_t)"abc"[i]);
    }
    assert(!serial_busy());
    now=UINT32_MAX-50u;
    assert(serial_submit("abc",3));
    USART2_SR=0;
    now+=99;
    serial_poll();
    assert(serial_busy());
    ++now;
    serial_poll();
    assert(!serial_busy() && serial_take_timeout());
    assert(!serial_take_timeout());
    assert(serial_submit("x",1));
    USART2_SR=1u<<7;
    serial_poll();
    assert(USART2_DR=='x' && !serial_busy());
    assert(munmap(mapped,4096)==0);
    puts("UART production-driver bounds, timeout, recovery, wrap PASS");
    return 0;
}
