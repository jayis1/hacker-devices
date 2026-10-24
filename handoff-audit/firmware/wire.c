/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <stdbool.h>

struct writer {
    char *data;
    size_t capacity;
    size_t used;
    bool failed;
};

uint16_t ha_crc(const char *data, size_t size)
{
    uint16_t crc=0xffffu;
    for (size_t i=0;i<size;++i) {
        crc^=(uint16_t)((uint16_t)(uint8_t)data[i]<<8);
        for (unsigned bit=0;bit<8;++bit) {
            if (crc&0x8000u)
                crc=(uint16_t)((crc<<1)^0x1021u);
            else
                crc=(uint16_t)(crc<<1);
        }
    }
    return crc;
}

static void character(struct writer *w, char value)
{
    if (w->used>=w->capacity) {
        w->failed=true;
        return;
    }
    w->data[w->used++]=value;
}

static void text(struct writer *w, const char *value)
{
    while (*value)
        character(w,*value++);
}

static void number(struct writer *w, uint32_t value)
{
    char reverse[10];
    unsigned used=0;
    character(w,',');
    do {
        reverse[used++]=(char)('0'+value%10u);
        value/=10u;
    } while (value);
    while (used)
        character(w,reverse[--used]);
}

static struct writer begin(char *out, size_t capacity, const char *prefix)
{
    struct writer w={out,capacity,0,false};
    if (!out)
        w.capacity=0;
    text(&w,prefix);
    return w;
}

static size_t finish(struct writer *w)
{
    static const char hex[]="0123456789ABCDEF";
    if (w->failed)
        return 0;
    uint16_t crc=ha_crc(w->data,w->used);
    character(w,'*');
    for (int shift=12;shift>=0;shift-=4)
        character(w,hex[(crc>>shift)&15u]);
    character(w,'\n');
    size_t size=w->used;
    character(w,'\0');
    return w->failed?0:size;
}

size_t ha_boot(char *out, size_t capacity, uint32_t flags)
{
    struct writer w=begin(out,capacity,"HA1,B");
    number(&w,flags);
    number(&w,HA_BREAK_MS);
    number(&w,HA_SETTLE_MS);
    number(&w,HA_QUALIFY);
    return finish(&w);
}

size_t ha_policy(char *out, size_t capacity)
{
    struct writer w=begin(out,capacity,"HA1,P");
    number(&w,HA_BREAK_MS);
    number(&w,HA_SETTLE_MS);
    number(&w,HA_QUALIFY);
    return finish(&w);
}

size_t ha_event_frame(char *out, size_t capacity, const struct ha_event *e)
{
    if (!e)
        return 0;
    struct writer w=begin(out,capacity,"HA1,E");
    number(&w,e->sequence);
    number(&w,e->time);
    number(&w,e->kind);
    number(&w,e->value);
    return finish(&w);
}

size_t ha_status(char *out, size_t capacity, const struct handoff *h)
{
    if (!h)
        return 0;
    struct writer w=begin(out,capacity,"HA1,S");
    number(&w,h->time);
    number(&w,h->levels);
    number(&w,h->synced);
    number(&w,h->last_path);
    number(&w,h->last_dead);
    number(&w,h->transitions);
    number(&w,h->command_overlap);
    number(&w,h->feedback_overlap);
    number(&w,h->short_break);
    number(&w,h->mismatch[0]);
    number(&w,h->mismatch[1]);
    number(&w,h->gaps);
    number(&w,h->missed);
    number(&w,h->dropped);
    number(&w,h->tx_fail);
    return finish(&w);
}
