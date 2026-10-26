/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
/* Fixed-capacity builder: no printf dependency or partial-frame submission. */
typedef struct {
    char *out;
    size_t size;
    size_t used;
    bool valid;
} Writer;

uint16_t wire_crc(const char *data, size_t n)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < n; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned b = 0; b < 8; ++b) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static void character(Writer *w, char ch)
{
    if (!w->valid)
        return;
    if (w->used + 1 >= w->size) {
        w->valid = false;
        return;
    }
    w->out[w->used++] = ch;
}

static void text(Writer *w, const char *s)
{
    while (*s)
        character(w, *s++);
}

static void number(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned length = 0;
    character(w, ',');
    do {
        reverse[length++] = '0' + value % 10u;
        value /= 10u;
    } while (value);
    while (length)
        character(w, reverse[--length]);
}

static Writer begin(char *out, size_t size, char kind)
{
    Writer w = {out, size, 0, out != NULL && size != 0};
    text(&w, "BB1,");
    character(&w, kind);
    return w;
}

static size_t finish(Writer *w)
{
    if (!w->valid)
        return 0;
    uint16_t crc = wire_crc(w->out, w->used);
    character(w, '*');
    const char *hex = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        character(w, hex[(crc >> shift) & 15]);
    character(w, '\n');
    if (!w->valid)
        return 0;
    w->out[w->used] = 0;
    return w->used;
}

size_t wire_boot(char *out, size_t cap, uint32_t flags)
{
    Writer w = begin(out, cap, 'B');
    number(&w, flags);
    return finish(&w);
}

size_t wire_policy(char *out, size_t cap)
{
    Writer w = begin(out, cap, 'P');
    number(&w, BB_CAPACITY);
    number(&w, BB_PERIOD);
    number(&w, BB_GRACE);
    number(&w, BB_QUALIFY);
    number(&w, BB_BASELINE);
    return finish(&w);
}

size_t wire_status(char *out, size_t cap, const Budget *s)
{
    if (!budget_valid(s))
        return 0;
    Writer w = begin(out, cap, 'S');
    number(&w, s->now);
    number(&w, s->phase);
    number(&w, s->levels);
    number(&w, s->qualified);
    number(&w, s->tokens);
    number(&w, s->refill);
    number(&w, s->empty_age);
    number(&w, s->faults);
    number(&w, s->admissions);
    number(&w, s->allowed);
    number(&w, s->excess);
    number(&w, s->denied);
    number(&w, s->missing);
    number(&w, s->gaps);
    number(&w, s->segments);
    number(&w, s->tx_fail);
    number(&w, s->minted);
    number(&w, s->discarded);
    return finish(&w);
}
