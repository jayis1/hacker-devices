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
    /* Keep explicit capacity and reserve NUL even for length-based UART. */
    if (!w->data || !w->capacity || w->used >= w->capacity - 1u) {
        w->failed = true;
        return;
    }
    w->data[w->used++] = c;
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

static Writer begin(char *out, size_t capacity, char kind)
{
    Writer w = {out, capacity, 0, false};
    const char *s = "RW1,";
    while (*s)
        put(&w, *s++);
    put(&w, kind);
    return w;
}

static size_t finish(Writer *w)
{
    if (w->failed)
        return 0;
    uint16_t crc = wire_crc(w->data, w->used);
    const char *hex = "0123456789ABCDEF";
    put(w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        put(w, hex[(crc >> shift) & 15u]);
    put(w, '\n');
    if (w->failed)
        return 0;
    w->data[w->used] = '\0';
    return w->used;
}

size_t wire_status(char *out, size_t capacity, const Receipt *r, uint32_t tx_fail)
{
    if (!r)
        return 0;
    Writer w = begin(out, capacity, 'S');
    number(&w, r->time);
    number(&w, r->ready);
    number(&w, r->levels);
    number(&w, r->count);
    number(&w, receipt_waiting(r));
    number(&w, r->faults);
    number(&w, r->actions);
    number(&w, r->appended);
    number(&w, r->receipts);
    number(&w, r->abandoned);
    number(&w, r->gaps);
    number(&w, r->missed);
    number(&w, r->segments);
    number(&w, r->max_append);
    number(&w, r->max_durable);
    number(&w, tx_fail);
    return finish(&w);
}

size_t wire_policy(char *out, size_t capacity)
{
    Writer w = begin(out, capacity, 'P');
    number(&w, CAPACITY);
    number(&w, APPEND_MS);
    number(&w, DURABLE_MS);
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
