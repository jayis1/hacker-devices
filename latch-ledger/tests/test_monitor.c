/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "monitor.h"
#include "wire.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void debounce(void)
{
    monitor_t m;
    monitor_init(&m, 0, 0);
    monitor_sample(&m, 1, 1);
    monitor_sample(&m, 2, 0);
    monitor_sample(&m, 3, 1);
    for (uint32_t t = 4; t <= 7; ++t)
        monitor_sample(&m, t, 1);
    assert(m.channel[0].stable == 0);
    monitor_sample(&m, 8, 1);
    assert(m.channel[0].stable == 1);
    assert(m.channel[0].raw_edges == 3);
    assert(m.channel[0].changes == 1);
    assert(m.channel[0].short_pulses == 2);
    assert(m.pair[0].mismatches == 1);
    monitor_sample(&m, 9, 3);
    for (uint32_t t = 10; t <= 14; ++t)
        monitor_sample(&m, t, 3);
    assert(m.pair[0].active == 0);
    assert(m.pair[0].longest_ms == 6);
}

static void rollover(void)
{
    monitor_t m;
    monitor_init(&m, UINT32_MAX - 2, 0);
    monitor_sample(&m, UINT32_MAX - 1, 1);
    monitor_sample(&m, UINT32_MAX, 1);
    for (uint32_t t = 0; t <= 4; ++t)
        monitor_sample(&m, t, 1);
    assert(m.channel[0].stable == 1);
    assert(m.channel[0].changes == 1);
    assert(m.missed_samples == 0);
}

static void queue_loss(void)
{
    monitor_t m;
    event_t e;
    monitor_init(&m, 0, 0);
    for (uint32_t t = 1; t < 300; ++t)
        monitor_sample(&m, t, (t & 1) ? 15 : 0);
    assert(m.dropped > 0);
    unsigned n = 0;
    uint32_t seq = 0;
    while (monitor_pop(&m, &e)) {
        assert(e.sequence > seq);
        seq = e.sequence;
        ++n;
    }
    assert(n == EVENT_CAPACITY);
    assert(!monitor_pop(&m, &e));
}

static void encoding(void)
{
    monitor_t m;
    char line[WIRE_CAPACITY];
    monitor_init(&m, 123, 9);
    size_t n = wire_status(line, sizeof line, &m, 0, 123);
    assert(n > 0 && line[n - 1] == '\n');
    assert(strncmp(line, "LL1,S,", 6) == 0);
    assert(wire_status(line, 8, &m, 0, 123) == 0);
    assert(wire_crc((const uint8_t *)"123456789", 9) == 0x29b1);
    assert(wire_status(line, sizeof line, &m, 4, 123) == 0);
}

static void saturation(void)
{
    monitor_t m;
    monitor_init(&m, 0, 0);
    m.channel[0].raw_edges = UINT32_MAX;
    monitor_sample(&m, 1, 1);
    assert(m.channel[0].raw_edges == UINT32_MAX);
    monitor_sample(&m, 1, 0); /* duplicate timestamp is not a sample */
    assert(m.channel[0].raw == 1);
}

int main(void)
{
    monitor_t gap;
    monitor_init(&gap, 0, 0);
    monitor_sample(&gap, 1, 1);
    monitor_sample(&gap, 10, 1);
    assert(gap.channel[0].stable == 0);
    assert(gap.missed_samples == 8);
    debounce();
    rollover();
    queue_loss();
    encoding();
    saturation();
    puts("monitor: 5 test groups passed");
    return 0;
}
