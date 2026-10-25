/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <stdbool.h>

/* No printf, dynamic allocation, native-endian structure dump or truncation. */
typedef struct {
    char *out;
    size_t cap;
    size_t used;
    bool failed;
} Writer;

uint16_t wire_crc(const char *s, size_t n)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < n; ++i) {
        crc ^= (uint16_t)(uint8_t)s[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc & 0x8000u) ? ((uint32_t)crc << 1) ^ 0x1021u : (uint32_t)crc << 1);
    }
    return crc;
}

static void character(Writer *w, char c)
{
    if (!w->out || w->used >= w->cap) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = c;
}

static Writer begin(char *out, size_t cap, char kind)
{
    Writer w = {out, cap, 0, false};
    const char *prefix = "RD1,";
    while (*prefix)
        character(&w, *prefix++);
    character(&w, kind);
    return w;
}

static void number(Writer *w, uint32_t value)
{
    char digits[10];
    size_t used = 0;
    character(w, ',');
    do {
        digits[used++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (used)
        character(w, digits[--used]);
}

static size_t finish(Writer *w)
{
    if (w->failed)
        return 0;
    uint16_t crc = wire_crc(w->out, w->used);
    character(w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(w, "0123456789ABCDEF"[(crc >> shift) & 15]);
    character(w, '\n');
    size_t length = w->used;
    character(w, '\0');
    return w->failed ? 0 : length;
}

size_t wire_boot(char *out, size_t cap, uint32_t reset)
{
    Writer w = begin(out, cap, 'B');
    number(&w, reset);
    return finish(&w);
}

size_t wire_policy(char *out, size_t cap)
{
    Writer w = begin(out, cap, 'P');
    number(&w, RD_QUIET_MS);
    number(&w, RD_DEADLINE_MS);
    return finish(&w);
}

size_t wire_status(char *out, size_t cap, const Drain *d)
{
    if (!drain_consistent(d))
        return 0;
    Writer w = begin(out, cap, 'S');
    number(&w, d->last);
    number(&w, d->state);
    number(&w, d->levels);
    number(&w, d->started);
    number(&w, d->completed);
    number(&w, d->abandoned);
    number(&w, d->age);
    number(&w, d->quiet);
    number(&w, d->admissions);
    number(&w, d->blocked);
    number(&w, d->early);
    number(&w, d->gaps);
    number(&w, d->tx_fail);
    number(&w, d->faults);
    return finish(&w);
}
