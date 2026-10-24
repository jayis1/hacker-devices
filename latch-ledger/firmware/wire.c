/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <stdbool.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    bool failed;
} builder_t;

uint16_t wire_crc(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}

static void put(builder_t *b, char value)
{
    if (b->failed)
        return;
    if (b->length + 1 >= b->capacity) {
        b->failed = true;
        return;
    }
    b->data[b->length++] = value;
    b->data[b->length] = '\0';
}

static void text(builder_t *b, const char *value)
{
    while (*value)
        put(b, *value++);
}

static void number(builder_t *b, uint32_t value)
{
    char reverse[10];
    unsigned count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (count)
        put(b, reverse[--count]);
}

static void field(builder_t *b, uint32_t value)
{
    put(b, ',');
    number(b, value);
}

static builder_t begin(char *out, size_t capacity, const char *kind)
{
    builder_t b = {out, 0, capacity, capacity == 0};
    text(&b, "LL1,");
    text(&b, kind);
    return b;
}

static size_t finish(builder_t *b)
{
    if (b->failed)
        return 0;
    uint16_t crc = wire_crc((const uint8_t *)b->data, b->length);
    const char *hex = "0123456789ABCDEF";
    put(b, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        put(b, hex[(crc >> shift) & 15]);
    put(b, '\n');
    return b->failed ? 0 : b->length;
}

size_t wire_event(char *out, size_t capacity, const event_t *e)
{
    builder_t b = begin(out, capacity, "E");
    field(&b, e->sequence);
    field(&b, e->time_ms);
    field(&b, e->kind);
    field(&b, e->channel);
    field(&b, e->value);
    field(&b, e->duration_ms);
    return finish(&b);
}

size_t wire_status(char *out, size_t capacity, const monitor_t *m,
                   uint8_t index, uint32_t now)
{
    if (index >= CHANNELS)
        return 0;
    const channel_t *c = &m->channel[index];
    builder_t b = begin(out, capacity, "S");
    field(&b, now);
    field(&b, index);
    field(&b, c->raw);
    field(&b, c->stable);
    field(&b, c->raw_edges);
    field(&b, c->changes);
    field(&b, c->short_pulses);
    field(&b, monitor_open_ms(c, now));
    field(&b, c->shortest_raw_ms);
    return finish(&b);
}

size_t wire_pair(char *out, size_t capacity, const monitor_t *m,
                 uint8_t index, uint32_t now)
{
    if (index >= PAIRS)
        return 0;
    const pair_t *p = &m->pair[index];
    builder_t b = begin(out, capacity, "P");
    field(&b, now);
    field(&b, index);
    field(&b, p->active);
    field(&b, p->mismatches);
    field(&b, monitor_pair_ms(p, now));
    return finish(&b);
}

size_t wire_health(char *out, size_t capacity, const monitor_t *m,
                   uint32_t now)
{
    builder_t b = begin(out, capacity, "H");
    field(&b, now);
    field(&b, m->sequence);
    field(&b, m->dropped);
    field(&b, m->missed_samples);
    field(&b, m->tx_failures);
    return finish(&b);
}

size_t wire_boot(char *out, size_t capacity, uint32_t reset_flags)
{
    builder_t b = begin(out, capacity, "B");
    field(&b, reset_flags);
    field(&b, DEBOUNCE_MS);
    field(&b, 1);
    return finish(&b);
}
