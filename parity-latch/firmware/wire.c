/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <stdbool.h>
/* Checked, allocation-free encoder. A partial output is never submitted. */
typedef struct {
    char *out;
    size_t cap;
    size_t used;
    uint16_t crc;
    bool failed;
} Writer;
static void byte(Writer *w, char ch, bool checksum)
{
    if (w->used >= w->cap) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = ch;
    if (!checksum)
        return;
    w->crc ^= (uint16_t)(uint8_t)ch << 8;
    for (unsigned bit = 0; bit < 8; ++bit)
        w->crc = (uint16_t)((w->crc << 1) ^ ((w->crc & 0x8000u) ? 0x1021u : 0));
}
static void number(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned length = 0;
    byte(w, ',', true);
    do {
        digits[length++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (length)
        byte(w, digits[--length], true);
}
static Writer begin(char *out, size_t cap, char kind)
{
    Writer w = {out, cap, 0, 0xffffu, false};
    if (!out) {
        w.cap = 0;
        w.failed = true;
    }
    const char *marker = "PL1,";
    while (*marker)
        byte(&w, *marker++, true);
    byte(&w, kind, true);
    return w;
}
static size_t finish(Writer *w)
{
    const char hex[] = "0123456789ABCDEF";
    uint16_t crc = w->crc;
    byte(w, '*', false);
    for (int shift = 12; shift >= 0; shift -= 4)
        byte(w, hex[(crc >> shift) & 15], false);
    byte(w, '\n', false);
    return w->failed ? 0 : w->used;
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
    number(&w, PL_SETUP);
    number(&w, PL_HOLD);
    number(&w, PL_QUALIFY);
    return finish(&w);
}
size_t wire_status(char *out, size_t cap, const Latch *c)
{
    if (!latch_valid(c))
        return 0;
    Writer w = begin(out, cap, 'S');
    number(&w, c->now);
    number(&w, c->levels);
    number(&w, c->valid);
    number(&w, c->active);
    number(&w, c->halted);
    number(&w, c->faults);
    number(&w, c->captured);
    number(&w, c->age);
    number(&w, c->starts);
    number(&w, c->completed);
    number(&w, c->good);
    number(&w, c->bad);
    number(&w, c->aborted);
    number(&w, c->parity);
    number(&w, c->setup);
    number(&w, c->hold);
    number(&w, c->collisions);
    number(&w, c->gaps);
    number(&w, c->missed);
    for (unsigned i = 0; i < 4; ++i)
        number(&w, c->bins[i]);
    number(&w, c->tx_fail);
    return finish(&w);
}
