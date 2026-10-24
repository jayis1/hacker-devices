/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic diagnostic vectors, not a physical measurement. */
#include "quorum.h"
#include "wire.h"
#include <stdio.h>
static struct quorum q;
static char frame[384];
static void hold(uint8_t levels, unsigned ticks)
{
    while (ticks--) qw_sample(&q, q.time + 1, levels);
}
int main(void)
{
    qw_init(&q, 0);
    qw_boot(frame, sizeof frame, 0);
    fputs(frame, stdout);
    qw_policy(frame, sizeof frame);
    fputs(frame, stdout);
    hold(0, 3);
    hold(8, 53);
    hold(0, 3);
    hold(3, 103);
    hold(15, 3);
    struct qw_event event;
    while (qw_pop(&q, &event)) {
        qw_event_frame(frame, sizeof frame, &event);
        fputs(frame, stdout);
    }
    qw_status(frame, sizeof frame, &q);
    fputs(frame, stdout);
}
