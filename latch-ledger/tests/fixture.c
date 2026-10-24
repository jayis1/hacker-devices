/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Deterministic SOFTWARE fixture, never evidence of physical measurement. */
#include "monitor.h"
#include "wire.h"
#include <stdio.h>

int main(void)
{
    monitor_t m;
    char line[WIRE_CAPACITY];
    monitor_init(&m, 0, 0);
    if (!wire_boot(line, sizeof line, 0))
        return 1;
    fputs(line, stdout);
    for (uint32_t now = 1; now <= 50; ++now) {
        uint8_t contacts = now < 10 ? 0 : (now < 20 ? 1 : 3);
        monitor_sample(&m, now, contacts);
        event_t event;
        while (monitor_pop(&m, &event)) {
            if (!wire_event(line, sizeof line, &event))
                return 1;
            fputs(line, stdout);
        }
    }
    for (uint8_t i = 0; i < CHANNELS; ++i) {
        if (!wire_status(line, sizeof line, &m, i, 50))
            return 1;
        fputs(line, stdout);
    }
    for (uint8_t i = 0; i < PAIRS; ++i) {
        if (!wire_pair(line, sizeof line, &m, i, 50))
            return 1;
        fputs(line, stdout);
    }
    if (!wire_health(line, sizeof line, &m, 50))
        return 1;
    fputs(line, stdout);
    return 0;
}
