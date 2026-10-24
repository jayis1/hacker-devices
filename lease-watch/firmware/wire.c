/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"

/* Allocation-free encoder, with CRC over every byte before the asterisk. */
typedef struct {
    char *out;
    size_t capacity;
    size_t length;
    uint16_t crc;
    bool failed;
} writer_t;

static void put(writer_t *w, char ch, bool checksum)
{
    if (w->failed)
        return;
    if (!w->out || w->length + 1u >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->length++] = ch;
    if (checksum) {
        w->crc ^= (uint16_t)(uint8_t)ch << 8;
        for (unsigned i = 0; i < 8; ++i) {
            if (w->crc & 0x8000u)
                w->crc = (uint16_t)((w->crc << 1) ^ 0x1021u);
            else
                w->crc = (uint16_t)(w->crc << 1);
        }
    }
}

static void literal(writer_t *w, const char *text)
{
    while (*text)
        put(w, *text++, true);
}

static void number(writer_t *w, uint32_t value)
{
    char digits[10];
    unsigned used = 0;
    do {
        digits[used++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    put(w, ',', true);
    while (used)
        put(w, digits[--used], true);
}

static writer_t begin(char *out, size_t capacity, const char *type)
{
    writer_t w = {out, capacity, 0, 0xffffu, false};
    literal(&w, "LW1,");
    literal(&w, type);
    return w;
}

static size_t finish(writer_t *w)
{
    static const char hex[] = "0123456789ABCDEF";
    put(w, '*', false);
    for (int shift = 12; shift >= 0; shift -= 4)
        put(w, hex[(w->crc >> shift) & 15u], false);
    put(w, '\n', false);
    if (w->failed) {
        if (w->out && w->capacity)
            w->out[0] = '\0';
        return 0;
    }
    w->out[w->length] = '\0';
    return w->length;
}

size_t wire_boot(char *out, size_t cap, uint32_t reset_flags)
{
    writer_t w = begin(out, cap, "B");
    number(&w, reset_flags);
    number(&w, LEASE_TIMEOUT);
    number(&w, LEASE_QUALIFY);
    return finish(&w);
}

size_t wire_status(char *out, size_t cap, const lease_t *s, uint32_t now)
{
    writer_t w = begin(out, cap, "S");
    number(&w, now);
    number(&w, s->levels);
    number(&w, s->alarms);
    number(&w, lease_age(s, now));
    number(&w, s->heartbeats);
    number(&w, s->last_period);
    number(&w, s->min_period);
    number(&w, s->max_period);
    number(&w, s->gaps);
    number(&w, s->missed);
    number(&w, s->dropped);
    number(&w, s->tx_failures);
    number(&w, s->max_enabled_stale);
    for (unsigned i = 0; i < LEASE_ALARM_COUNT; ++i)
        number(&w, s->violations[i]);
    return finish(&w);
}

size_t wire_event(char *out, size_t cap, const lease_event_t *event)
{
    writer_t w = begin(out, cap, "E");
    number(&w, event->sequence);
    number(&w, event->time);
    number(&w, event->kind);
    number(&w, event->value);
    return finish(&w);
}
