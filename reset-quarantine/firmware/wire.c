/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"

/* Bounded encoder: no printf, heap, truncation success, or locale dependency. */
struct writer {
    char *data;
    size_t capacity;
    size_t used;
    bool failed;
};

uint16_t rq_crc(const char *data, size_t size)
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
    uint16_t crc=rq_crc(w->data,w->used);
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
    number(w,RQ_WINDOW);
    number(w,RQ_QUALIFY);
}

size_t rq_boot(char *out, size_t capacity, uint32_t flags)
{
    struct writer w=begin(out,capacity,"RQ1,B");
    number(&w,flags);
    policy(&w);
    return finish(&w);
}

size_t rq_policy(char *out, size_t capacity)
{
    struct writer w=begin(out,capacity,"RQ1,P");
    policy(&w);
    return finish(&w);
}

size_t rq_event_frame(char *out, size_t capacity, const struct rq_event *e)
{
    if (!e)
        return 0;
    struct writer w=begin(out,capacity,"RQ1,E");
    number(&w,e->sequence);
    number(&w,e->time);
    number(&w,e->kind);
    number(&w,e->value);
    return finish(&w);
}

size_t rq_status(char *out, size_t capacity, const struct quarantine *c)
{
    if (!c)
        return 0;
    struct writer w=begin(out,capacity,"RQ1,S");
    number(&w,c->time);
    number(&w,c->levels);
    number(&w,c->valid);
    number(&w,c->state);
    number(&w,c->age);
    number(&w,c->quiet_ms);
    number(&w,c->releases);
    number(&w,c->contradictions);
    number(&w,c->unsafe);
    number(&w,c->interrupted);
    number(&w,c->revoked);
    number(&w,c->gaps);
    number(&w,c->missed);
    number(&w,c->dropped);
    number(&w,c->tx_fail);
    return finish(&w);
}
