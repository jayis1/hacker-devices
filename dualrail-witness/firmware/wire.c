/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <string.h>
/* No printf dependency: fixed-memory checked decimal/CRC encoder. */
typedef struct {
    char *out;
    size_t used;
    size_t size;
    uint16_t crc;
    bool failed;
} Writer;
static void put(Writer *p, char ch, bool checksum)
{
    if (p->failed)
        return;
    if (p->used + 1u >= p->size) {
        p->failed = true;
        return;
    }
    p->out[p->used++] = ch;
    if (checksum) {
        p->crc ^= (uint16_t)((uint16_t)(uint8_t)ch << 8);
        for (unsigned i = 0; i < 8; ++i)
            p->crc = (uint16_t)((p->crc << 1) ^ ((p->crc & 0x8000u) ? 0x1021u : 0u));
    }
}
static void text(Writer *p, const char *s)
{
    while (*s)
        put(p, *s++, true);
}
static void number(Writer *p, uint32_t n)
{
    char digits[10];
    unsigned used = 0;
    put(p, ',', true);
    do {
        digits[used++] = (char)('0' + n % 10u);
        n /= 10u;
    } while (n);
    while (used)
        put(p, digits[--used], true);
}
static Writer begin(char *out, size_t size, char kind)
{
    Writer p = {out, 0, size, 0xffffu, out == NULL || size == 0};
    text(&p, "DW1,");
    put(&p, kind, true);
    return p;
}
static size_t finish(Writer *p)
{
    static const char hex[] = "0123456789ABCDEF";
    uint16_t crc = p->crc;
    put(p, '*', false);
    for (int shift = 12; shift >= 0; shift -= 4)
        put(p, hex[(crc >> shift) & 15u], false);
    put(p, '\n', false);
    if (p->failed) {
        if (p->out && p->size)
            p->out[0] = 0;
        return 0;
    }
    p->out[p->used] = 0;
    return p->used;
}
size_t wire_policy(char *out, size_t size)
{
    Writer p = begin(out, size, 'P');
    number(&p, DW_QUALIFY);
    number(&p, DW_GRACE);
    return finish(&p);
}
size_t wire_boot(char *out, size_t size, uint32_t reset)
{
    Writer p = begin(out, size, 'B');
    number(&p, reset);
    return finish(&p);
}
size_t wire_status(char *out, size_t size, const Witness *w)
{
    if (!witness_consistent(w))
        return 0;
    Writer p = begin(out, size, 'S');
    number(&p, w->now);
    number(&p, w->levels);
    number(&p, w->valid);
    number(&p, w->halted);
    number(&p, w->faults);
    number(&p, w->invalid);
    number(&p, w->unsafe);
    number(&p, w->silent);
    number(&p, w->invalid_ms);
    number(&p, w->unsafe_ms);
    number(&p, w->silent_ms);
    number(&p, w->max_invalid);
    number(&p, w->max_unsafe);
    number(&p, w->gaps);
    number(&p, w->missed);
    number(&p, w->tx_fail);
    return finish(&p);
}
