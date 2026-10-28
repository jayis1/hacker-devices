/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic fixture emitted by the production encoder, not hardware evidence. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Mailbox s;
static uint32_t now;
static char out[768];
static void emit(size_t length)
{
    assert(length);
    assert(fwrite(out,1,length,stdout)==length);
}
static void run(unsigned raw, unsigned duration)
{
    while (duration--)
        mailbox_sample(&s,++now,raw);
    emit(wire_status(out,sizeof out,&s));
}
int main(void)
{
    mailbox_init(&s);
    emit(wire_boot(out,sizeof out,0));
    emit(wire_policy(out,sizeof out));
    run(0,30);
    run(4,10);
    run(12,10);
    run(8,10);
    run(9,1);
    return 0;
}
