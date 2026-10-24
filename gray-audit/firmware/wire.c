/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <stdbool.h>
#include <stdint.h>

/* Bounded allocation-free CSV encoder. CRC covers everything before '*'. */
typedef struct {
    char *out;
    size_t capacity;
    size_t length;
    uint16_t crc;
    bool failed;
} writer_t;

static writer_t start(char *out, size_t capacity)
{
    writer_t w = {out, capacity, 0, 0xffffu, !out || !capacity};
    return w;
}

static void raw(writer_t *w, char c)
{
    if (w->failed)
        return;
    if (w->length + 1 >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->length++] = c;
    w->out[w->length] = '\0';
}

static void character(writer_t *w, char c)
{
    raw(w, c);
    w->crc ^= (uint16_t)(uint8_t)c << 8;
    for (unsigned bit = 0; bit < 8; ++bit)
        w->crc = (uint16_t)((w->crc & 0x8000u) ?
                 ((uint32_t)w->crc << 1) ^ 0x1021u : (uint32_t)w->crc << 1);
}

static void text(writer_t *w, const char *s)
{
    while (*s)
        character(w, *s++);
}

static void number(writer_t *w, uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
        character(w, digits[--count]);
}

static void field(writer_t *w, uint32_t value)
{
    character(w, ',');
    number(w, value);
}

static void signed_field(writer_t *w, int32_t value)
{
    character(w, ',');
    if (value < 0) {
        character(w, '-');
        /* Unsigned subtraction safely represents INT32_MIN's magnitude. */
        number(w, 0u - (uint32_t)value);
    } else {
        number(w, (uint32_t)value);
    }
}

static size_t finish(writer_t *w)
{
    static const char hex[] = "0123456789ABCDEF";
    uint16_t crc = w->crc;
    raw(w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        raw(w, hex[(crc >> shift) & 15u]);
    raw(w, '\n');
    if (w->failed) {
        if (w->out && w->capacity)
            w->out[0] = '\0';
        return 0;
    }
    return w->length;
}

size_t wire_axis(char *out, size_t capacity, const audit_t *a,
                 unsigned index, uint32_t now)
{
    if (!a || index >= AXES)
        return 0;
    const axis_t *s = &a->axis[index];
    writer_t w = start(out, capacity);
    text(&w, "GA1,S");
    field(&w, now);
    field(&w, index);
    field(&w, s->state);
    signed_field(&w, s->position);
    field(&w, s->forward);
    field(&w, s->reverse);
    field(&w, s->illegal);
    field(&w, s->reversals);
    field(&w, s->edges);
    field(&w, s->cycles_forward);
    field(&w, s->cycles_reverse);
    field(&w, s->phase_a);
    field(&w, s->phase_b);
    field(&w, audit_age(s, now));
    field(&w, s->min_interval);
    field(&w, s->max_interval);
    field(&w, s->saturated);
    return finish(&w);
}

size_t wire_histogram(char *out, size_t capacity, const audit_t *a,
                      unsigned index, uint32_t now)
{
    if (!a || index >= AXES)
        return 0;
    writer_t w = start(out, capacity);
    text(&w, "GA1,D");
    field(&w, now);
    field(&w, index);
    for (unsigned state = 0; state < 4; ++state)
        field(&w, a->axis[index].dwell[state]);
    for (unsigned bin = 0; bin < HISTOGRAM_BINS; ++bin)
        field(&w, a->axis[index].histogram[bin]);
    return finish(&w);
}

size_t wire_health(char *out, size_t capacity, const audit_t *a, uint32_t now)
{
    writer_t w = start(out, capacity);
    text(&w, "GA1,H");
    field(&w, now);
    field(&w, a->samples);
    field(&w, a->gaps);
    field(&w, a->missed);
    field(&w, a->dropped);
    field(&w, a->tx_failed);
    field(&w, a->sequence);
    return finish(&w);
}

size_t wire_event(char *out, size_t capacity, const audit_event_t *e)
{
    writer_t w = start(out, capacity);
    text(&w, "GA1,E");
    field(&w, e->time);
    field(&w, e->sequence);
    field(&w, e->axis);
    field(&w, e->kind);
    field(&w, e->before);
    field(&w, e->after);
    return finish(&w);
}

size_t wire_boot(char *out, size_t capacity, uint32_t flags)
{
    writer_t w = start(out, capacity);
    text(&w, "GA1,B");
    field(&w, 0);
    field(&w, flags);
    return finish(&w);
}
