/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include "drivers/crc16.h"
#include <stdbool.h>
/* Writer never emits a partially valid packet to the transport. */
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
    const char *marker = "EL1,";
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
    number(&w, EL_QUALIFY);
    number(&w, 1); /* nominal sample milliseconds */
    number(&w, 4); /* input count */
    number(&w, EL_PAIRS);
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
    for (unsigned i = 0; i < EL_PAIRS; ++i) {
        const Pair *p = &s->pair[i];
        number(&w, p->entries);
        number(&w, p->duration);
        number(&w, p->maximum);
        number(&w, p->age);
        number(&w, p->active);
    }
    number(&w, s->tx_fail);
    return finish(&w);
}
