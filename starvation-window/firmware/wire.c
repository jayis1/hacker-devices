/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <stdbool.h>

typedef struct {
    char *out;
    size_t size;
    size_t used;
    bool failed;
} Writer;

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc & 0x8000u) ?
                ((uint32_t)crc << 1) ^ 0x1021u : (uint32_t)crc << 1);
    }
    return crc;
}

static void character(Writer *w, char value)
{
    if (!w->out || w->used + 1 >= w->size) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = value;
    w->out[w->used] = '\0';
}

static void text(Writer *w, const char *value)
{
    while (*value)
        character(w, *value++);
}

static void number(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned used = 0;
    do {
        digits[used++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    character(w, ',');
    while (used)
        character(w, digits[--used]);
}

static size_t finish(Writer *w)
{
    if (w->failed)
        return 0;
    uint16_t crc = wire_crc(w->out, w->used);
    const char *hex = "0123456789ABCDEF";
    character(w, '*');
    for (unsigned shift = 16; shift; ) {
        shift -= 4;
        character(w, hex[(crc >> shift) & 15u]);
    }
    character(w, '\n');
    return w->failed ? 0 : w->used;
}

size_t wire_policy(char *out, size_t size)
{
    Writer w = {out, size, 0, false};
    text(&w, "SW1,P");
    number(&w, IW_DEAD_MS);
    number(&w, 1); /* nominal sample period */
    number(&w, IW_WAIT_MS);
    number(&w, IW_BYPASSES);
    return finish(&w);
}

size_t wire_boot(char *out, size_t size, uint32_t flags)
{
    Writer w = {out, size, 0, false};
    text(&w, "SW1,B");
    number(&w, flags);
    return finish(&w);
}

size_t wire_status(char *out, size_t size, const Isolation *s)
{
    if (!isolation_valid(s))
        return 0;
    Writer w = {out, size, 0, false};
    text(&w, "SW1,S");
    number(&w, s->now);
    number(&w, s->state);
    number(&w, s->levels);
    number(&w, s->quiet);
    number(&w, s->last_owner);
    number(&w, s->switches);
    number(&w, s->early);
    number(&w, s->overlaps);
    number(&w, s->overlap_ms);
    number(&w, s->unrequested);
    number(&w, s->gaps);
    number(&w, s->tx_fail);
    number(&w, s->faults);
    number(&w, s->minimum);
    for (unsigned i = 0; i < 4; ++i)
        number(&w, s->histogram[i]);
    for (unsigned i = 0; i < 2; ++i)
        number(&w, s->wait[i]);
    for (unsigned i = 0; i < 2; ++i)
        number(&w, s->bypass[i]);
    for (unsigned i = 0; i < 2; ++i)
        number(&w, s->starved[i]);
    for (unsigned i = 0; i < 2; ++i)
        number(&w, s->services[i]);
    return finish(&w);
}
