/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "audit.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    audit_t a;
    audit_init(&a, 0, 0);
    const uint8_t forward[] = {1, 3, 2, 0};
    for (unsigned i = 0; i < 4; ++i)
        audit_sample(&a, i + 1, forward[i]);
    assert(a.axis[0].position == 4);
    assert(a.axis[0].forward == 4);
    assert(a.axis[0].cycles_forward == 1);
    assert(a.axis[1].position == 0);
    audit_sample(&a, 5, 2);
    assert(a.axis[0].reverse == 1 && a.axis[0].reversals == 1);
    audit_sample(&a, 6, 1);
    assert(a.axis[0].illegal == 1);
    assert(a.axis[0].position == 3);
    audit_sample(&a, 10, 3);
    assert(a.gaps == 1 && a.missed == 3);
    assert(a.axis[0].position == 3);
    audit_sample(&a, 10, 0); /* duplicate tick cannot fabricate an edge */
    assert(a.axis[0].state == 3);
    audit_sample(&a, 11, 2);
    assert(a.axis[0].position == 4);
    audit_init(&a, UINT32_MAX, 0);
    audit_sample(&a, 0, 1);
    assert(a.axis[0].forward == 1 && a.gaps == 0);
    a.axis[0].position = INT32_MAX;
    audit_sample(&a, 1, 3);
    assert(a.axis[0].position == INT32_MAX && a.axis[0].saturated);
    audit_init(&a, 0, 0);
    for (unsigned t = 1; t <= 100; ++t)
        audit_sample(&a, t, (t & 1) ? 3 : 0);
    assert(a.dropped > 0 && a.count == EVENT_CAPACITY);
    audit_event_t e;
    unsigned count = 0;
    while (audit_pop(&a, &e)) ++count;
    assert(count == EVENT_CAPACITY);
    const int8_t expected_delta[] = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};
    for (unsigned old = 0; old < 4; ++old) {
        for (unsigned next = 0; next < 4; ++next) {
            audit_init(&a, 0, old);
            audit_sample(&a, 1, next);
            assert(a.axis[0].illegal == ((old ^ next) == 3));
            assert(a.axis[0].edges == (old != next));
            assert(a.axis[0].position == expected_delta[old*4+next]);
            audit_init(&a, 0, old << 2);
            audit_sample(&a, 1, next << 2);
            assert(a.axis[1].position == expected_delta[old*4+next]);
            assert(a.axis[1].illegal == ((old ^ next) == 3));
            assert(a.axis[0].position == 0);
        }
    }
    char line[WIRE_CAPACITY];
    size_t n = wire_axis(line, sizeof line, &a, 0, 1);
    assert(n && line[n - 1] == '\n');
    assert(strncmp(line, "GA1,S,", 6) == 0);
    assert(wire_axis(line, 8, &a, 0, 1) == 0);
    assert(wire_axis(line, sizeof line, &a, 2, 1) == 0);
    audit_init(&a, 0, 0);
    a.axis[0].position = INT32_MIN;
    audit_sample(&a, 1, 2);
    assert(a.axis[0].position == INT32_MIN && a.axis[0].saturated);
    a.tx_failed = UINT32_MAX;
    audit_tx_failure(&a);
    assert(a.tx_failed == UINT32_MAX);
    a.axis[0].reverse = UINT32_MAX;
    audit_sample(&a, 2, 3);
    assert(a.axis[0].reverse == UINT32_MAX);
    puts("audit tests passed");
    return 0;
}
