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
    const char *marker = "CS1,";
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
    number(&w, CS_CAPACITY);
    number(&w, CS_SETUP);
    number(&w, CS_QUALIFY);
    number(&w, CS_BASELINE);
    return finish(&w);
}
size_t wire_status(char *out, size_t cap, const Stack *c)
{
    if (!stack_valid(c))
        return 0;
    Writer w = begin(out, cap, 'S');
    number(&w, c->now);
    number(&w, c->levels);
    number(&w, c->valid);
    number(&w, c->ready);
    number(&w, c->halted);
    number(&w, c->faults);
    number(&w, c->depth);
    number(&w, c->top);
    number(&w, c->pushes);
    number(&w, c->pops);
    number(&w, c->matched);
    number(&w, c->abandoned);
    number(&w, c->mismatch);
    number(&w, c->underflow);
    number(&w, c->overflow);
    number(&w, c->collision);
    number(&w, c->unstable);
    number(&w, c->gaps);
    number(&w, c->missed);
    number(&w, c->peak);
    number(&w, c->tx_fail);
    return finish(&w);
}
