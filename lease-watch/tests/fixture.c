/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic trace produced by the production engine, not hardware evidence. */
#include "lease.h"
#include "wire.h"
#include <stdio.h>
int main(void)
{
    lease_t state;
    char line[WIRE_CAPACITY];
    lease_init(&state, 0, 12);
    wire_boot(line, sizeof line, 0);
    fputs(line, stdout);
    for (uint32_t t = 1; t <= 1000; ++t) {
        uint8_t levels = 14;
        if (t < 400 && (t / 25) % 2) levels |= 1;
        lease_sample(&state, t, levels);
        lease_event_t event;
        while (lease_pop(&state, &event)) {
            wire_event(line, sizeof line, &event);
            fputs(line, stdout);
        }
        if (!(t % 100)) {
            wire_status(line, sizeof line, &state, t);
            fputs(line, stdout);
        }
    }
}
