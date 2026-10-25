/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"

typedef struct {
    char *data;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static void put(Writer *w, char c)
{
    /* Reserve a terminator even though serial_submit takes an explicit length. */
    if (!w->data || !w->capacity || w->used >= w->capacity - 1) {
        w->failed = true;
        return;
    }
    w->data[w->used++] = c;
}

static void text(Writer *w, const char *s)
{
    while (*s)
        put(w, *s++);
}

static void number(Writer *w, uint32_t value)
{
    char digits[10];
    size_t count = 0;
    put(w, ',');
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
        put(w, digits[--count]);
}

static Writer begin(char *data, size_t capacity, char kind)
{
    Writer w = {data, capacity, 0, false};
    text(&w, "EW1,");
    put(&w, kind);
    return w;
}

static size_t finish(Writer *w)
{
    if (w->failed)
        return 0;
    uint16_t crc = wire_crc(w->data, w->used);
    static const char hex[] = "0123456789ABCDEF";
    put(w, '*');
    put(w, hex[(crc >> 12) & 15u]);
    put(w, hex[(crc >> 8) & 15u]);
    put(w, hex[(crc >> 4) & 15u]);
    put(w, hex[crc & 15u]);
    put(w, '\n');
    if (w->failed)
        return 0;
    w->data[w->used] = '\0';
    return w->used;
}

size_t wire_status(char *out, size_t capacity, const Epoch *b, uint32_t tx_fail)
{
    if (!b)
        return 0;
    Writer w = begin(out, capacity, 'S');
    number(&w, b->time);
    number(&w, b->phase);
    number(&w, b->qualified);
    number(&w, b->levels);
    number(&w, b->valid);
    number(&w, b->have_highest);
    number(&w, b->highest);
    number(&w, b->current);
    number(&w, b->capture_age);
    number(&w, b->faults);
    number(&w, b->accepted);
    number(&w, b->downgrades);
    number(&w, b->invalid);
    number(&w, b->execute_ms);
    number(&w, b->gaps);
    number(&w, b->missed);
    number(&w, b->segments);
    number(&w, b->dropped);
    number(&w, tx_fail);
    return finish(&w);
}

size_t wire_event(char *out, size_t capacity, const Event *e)
{
    if (!e)
        return 0;
    Writer w = begin(out, capacity, 'E');
    number(&w, e->sequence);
    number(&w, e->time);
    number(&w, e->kind);
    number(&w, e->value);
    return finish(&w);
}

size_t wire_policy(char *out, size_t capacity)
{
    Writer w = begin(out, capacity, 'P');
    number(&w, SETUP_MS);
    number(&w, MIN_COMMIT_MS);
    number(&w, MAX_COMMIT_MS);
    number(&w, QUALIFY_MS);
    number(&w, BASELINE_MS);
    return finish(&w);
}

size_t wire_boot(char *out, size_t capacity, uint32_t flags)
{
    Writer w = begin(out, capacity, 'B');
    number(&w, flags);
    return finish(&w);
}
