/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"

/* Bounded encoder: no printf, heap, truncation success, or locale dependency. */
struct writer {
    char *data;
    size_t capacity;
    size_t used;
    bool failed;
};

uint16_t cg_crc(const char *data, size_t size)
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
    uint16_t crc=cg_crc(w->data,w->used);
    character(w,'*');
    for (int shift=12;shift>=0;shift-=4)
        character(w,hex[(crc>>shift)&15u]);
    character(w,'\n');
    size_t size=w->used;
    character(w,'\0');
    return w->failed?0:size;
}

static void policy(struct writer *w)
{
    number(w,CG_WINDOW);
    number(w,CG_QUALIFY);
}

size_t cg_boot(char *out, size_t capacity, uint32_t flags)
{
    struct writer w=begin(out,capacity,"CG1,B");
    number(&w,flags);
    policy(&w);
    return finish(&w);
}

size_t cg_policy(char *out, size_t capacity)
{
    struct writer w=begin(out,capacity,"CG1,P");
    policy(&w);
    return finish(&w);
}

size_t cg_event_frame(char *out, size_t capacity, const struct cg_event *e)
{
    if (!e)
        return 0;
    struct writer w=begin(out,capacity,"CG1,E");
    number(&w,e->sequence);
    number(&w,e->time);
    number(&w,e->kind);
    number(&w,e->value);
    return finish(&w);
}

size_t cg_status(char *out, size_t capacity, const struct consent *c)
{
    if (!c)
        return 0;
    struct writer w=begin(out,capacity,"CG1,S");
    number(&w,c->time);
    number(&w,c->levels);
    number(&w,c->valid);
    number(&w,c->state);
    number(&w,c->age);
    number(&w,c->skew_ms);
    number(&w,c->pairs);
    number(&w,c->rejected);
    number(&w,c->unsafe);
    number(&w,c->expired);
    number(&w,c->revoked);
    number(&w,c->gaps);
    number(&w,c->missed);
    number(&w,c->dropped);
    number(&w,c->tx_fail);
    return finish(&w);
}
