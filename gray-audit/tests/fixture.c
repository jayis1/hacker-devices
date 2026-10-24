/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic input stream encoded by the production implementation. */
#include "audit.h"
#include "wire.h"
#include <stdio.h>
int main(void)
{
    audit_t a;
    char out[WIRE_CAPACITY];
    audit_init(&a, 0, 0);
    wire_boot(out, sizeof out, 0);
    fputs(out, stdout);
    const uint8_t motion[] = {1, 3, 2, 0};
    for (unsigned t = 1; t <= 40; ++t)
        audit_sample(&a, t, motion[(t-1)%4]);
    audit_sample(&a, 41, 3);
    for (unsigned i = 0; i < AXES; ++i) {
        wire_axis(out, sizeof out, &a, i, 41);
        fputs(out, stdout);
        wire_histogram(out, sizeof out, &a, i, 41);
        fputs(out, stdout);
    }
    wire_health(out, sizeof out, &a, 41);
    fputs(out, stdout);
    audit_event_t e;
    while (audit_pop(&a, &e)) {
        wire_event(out, sizeof out, &e);
        fputs(out, stdout);
    }
    return 0;
}
