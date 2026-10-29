/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include "drivers/crc16.h"
#include <stdbool.h>
/* A failed writer returns zero; callers must not submit partial output. */
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
    if (checksum)
        w->crc = crc16_byte(w->crc, (uint8_t)ch);
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
    const char *marker = "BO1,";
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
    number(&w, BO_QUALIFY);
    number(&w, 1); /* Nominal sample period in milliseconds. */
    number(&w, 4); /* Owners. */
    number(&w, 16); /* Directed matrix cells. */
    return finish(&w);
}
size_t wire_status(char *out, size_t cap, const Ledger *s)
{
    if (!ledger_valid(s))
        return 0;
    Writer w = begin(out, cap, 'S');
    number(&w, s->now);
    number(&w, s->levels);
    number(&w, s->valid);
    number(&w, s->halted);
    number(&w, s->faults);
    number(&w, s->observed);
    number(&w, s->gaps);
    number(&w, s->missed);
    number(&w, s->anchors);
    number(&w, s->forward);
    number(&w, s->skip);
    number(&w, s->reverse);
    number(&w, s->repeat);
    number(&w, s->collisions);
    number(&w, s->last);
    number(&w, s->ready);
    number(&w, s->idle);
    number(&w, s->collision_active);
    for (unsigned i = 0; i < 16; ++i)
        number(&w, s->matrix[i]);
    number(&w, s->tx_fail);
    return finish(&w);
}
