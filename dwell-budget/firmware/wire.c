/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"

/* A fail-closed bounded encoder avoids printf/newlib heap and locale behavior.
 * Every public encoder returns zero on insufficient space; callers must not
 * transmit partially constructed output. No implicit terminating NUL is sent. */
typedef struct {
    char *out;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned j = 0; j < 8; ++j) {
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
    if (w->failed)
        return;
    if (!w->out || w->used == w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = c;
}

static void text(Writer *w, const char *s)
{
    while (*s)
        put(w, *s++);
}

static void decimal(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
        put(w, reverse[--count]);
}

static void field(Writer *w, uint32_t value)
{
    put(w, ',');
    decimal(w, value);
}

static Writer begin(char *out, size_t capacity, char kind)
{
    Writer w = {out, capacity, 0, false};
    /* LF also permits resynchronization after a timed-out previous frame. */
    text(&w, "DB1,");
    put(&w, kind);
    return w;
}

static size_t finish(Writer *w)
{
    if (w->failed)
        return 0;
    uint16_t crc = wire_crc(w->out, w->used);
    static const char hex[] = "0123456789ABCDEF";
    put(w, '*');
    put(w, hex[(crc >> 12) & 15u]);
    put(w, hex[(crc >> 8) & 15u]);
    put(w, hex[(crc >> 4) & 15u]);
    put(w, hex[crc & 15u]);
    put(w, '\n');
    return w->failed ? 0 : w->used;
}

size_t wire_boot(char *out, size_t capacity, uint32_t reset)
{
    Writer w = begin(out, capacity, 'B');
    field(&w, reset);
    return finish(&w);
}

size_t wire_policy(char *out, size_t capacity)
{
    Writer w = begin(out, capacity, 'P');
    field(&w, DB_WINDOW);
    field(&w, DB_LIMIT);
    field(&w, DB_RESPONSE);
    return finish(&w);
}

size_t wire_status(char *out, size_t capacity, const Budget *b)
{
    if (!budget_valid(b))
        return 0;
    Writer w = begin(out, capacity, 'S');
    field(&w, b->last);
    field(&w, b->fill);
    field(&w, b->levels);
    field(&w, b->occupied);
    field(&w, b->unauthorized);
    field(&w, b->total_active);
    field(&w, b->total_denied);
    field(&w, b->excess_episodes);
    field(&w, b->gaps);
    field(&w, b->missed);
    field(&w, b->faults);
    field(&w, b->tx_fail);
    field(&w, b->over_age);
    field(&w, b->peak);
    return finish(&w);
}
