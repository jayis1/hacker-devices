/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic input only, using the production policy and wire encoder. */
#include "epoch.h"
#include "wire.h"
#include <stdio.h>
static Epoch epoch;
static char frame[384];
static void run(unsigned n, uint8_t raw)
{
    while (n--) {
        epoch_tick(&epoch, epoch.time + 1u, raw);
        Event event;
        while (epoch_pop(&epoch, &event)) {
            wire_event(frame, sizeof frame, &event);
            fputs(frame, stdout);
        }
        if (epoch.time % 10u == 0) {
            wire_status(frame, sizeof frame, &epoch, 0);
            fputs(frame, stdout);
        }
    }
}
int main(void)
{
    epoch_init(&epoch, 0);
    wire_boot(frame, sizeof frame, 0); fputs(frame, stdout);
    wire_policy(frame, sizeof frame); fputs(frame, stdout);
    run(30,0);
    run(20,2); run(20,2|INPUT_COMMIT); run(20,2);
    run(20,2|INPUT_EXECUTE); run(20,2);
    run(20,1); run(20,1|INPUT_COMMIT); run(20,1);
    run(20,1|INPUT_EXECUTE);
    return 0;
}
