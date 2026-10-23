/* LIN framing and decoder
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "lin.h"
#include "../board.h"
#include <string.h>

static uint8_t expected_length(uint8_t id) {
    /* LIN diagnostic frames are always eight bytes. For unknown schedules the
       capture engine waits for inter-byte timeout; this table supplies common
       signal lengths and can be replaced by the host's learned schedule. */
    if (id == 0x3c || id == 0x3d) return 8;
    if (id < 0x20) return 2;
    if (id < 0x30) return 4;
    return 8;
}

uint8_t lin_make_pid(uint8_t id) {
    id &= 0x3fu;
    uint8_t b0=(id>>0)&1u,b1=(id>>1)&1u,b2=(id>>2)&1u;
    uint8_t b3=(id>>3)&1u,b4=(id>>4)&1u,b5=(id>>5)&1u;
    uint8_t p0=b0^b1^b2^b4;
    uint8_t p1=(uint8_t)(!(b1^b3^b4^b5));
    return (uint8_t)(id|(p0<<6)|(p1<<7));
}

bool lin_pid_valid(uint8_t pid) { return lin_make_pid(pid & 0x3fu) == pid; }

uint8_t lin_checksum(const uint8_t *data,uint8_t length,uint8_t pid,lin_checksum_t type) {
    uint16_t sum=(type==LIN_ENHANCED)?pid:0u;
    for(uint8_t i=0;i<length;i++) {
        sum += data[i];
        if(sum>255u) sum=(sum&255u)+1u;
    }
    return (uint8_t)(~sum);
}

void lin_decoder_init(lin_decoder_t *d,lin_side_t side) {
    memset(d,0,sizeof(*d));
    d->state=RX_WAIT_BREAK;
    d->pending.source=side;
}

bool lin_decoder_break(lin_decoder_t *d,uint32_t now) {
    d->state=RX_WAIT_SYNC;
    d->count=0;
    d->pending.timestamp_us=now;
    d->pending.parity_ok=false;
    d->pending.checksum_ok=false;
    d->pending.collision=false;
    return false;
}

static bool finish(lin_decoder_t *d,lin_frame_t *out) {
    if(d->count<2) { d->state=RX_WAIT_BREAK; return false; }
    d->pending.length=(uint8_t)(d->count-1u);
    uint8_t classic=lin_checksum(d->pending.data,d->pending.length,d->pending.pid,LIN_CLASSIC);
    uint8_t enhanced=lin_checksum(d->pending.data,d->pending.length,d->pending.pid,LIN_ENHANCED);
    if(classic==d->pending.checksum) {
        d->pending.checksum_ok=true; d->pending.checksum_type=LIN_CLASSIC;
    } else if(enhanced==d->pending.checksum) {
        d->pending.checksum_ok=true; d->pending.checksum_type=LIN_ENHANCED;
    } else {
        d->pending.checksum_ok=false; d->pending.checksum_type=LIN_AUTO; d->checksum_errors++;
    }
    *out=d->pending;
    d->frames++;
    d->state=RX_WAIT_BREAK;
    d->count=0;
    return true;
}

bool lin_decoder_byte(lin_decoder_t *d,uint8_t byte,uint32_t now,lin_frame_t *out) {
    d->last_byte_us=now;
    switch(d->state) {
    case RX_WAIT_BREAK:
        return false;
    case RX_WAIT_SYNC:
        if(byte==0x55u) d->state=RX_WAIT_PID;
        else d->state=RX_WAIT_BREAK;
        return false;
    case RX_WAIT_PID:
        d->pending.pid=byte;
        d->pending.id=(uint8_t)(byte&0x3fu);
        d->pending.parity_ok=lin_pid_valid(byte);
        if(!d->pending.parity_ok) d->parity_errors++;
        d->state=RX_DATA;
        d->count=0;
        return false;
    case RX_DATA: {
        uint8_t payload=expected_length(d->pending.id);
        if(d->count<payload) d->pending.data[d->count]=byte;
        else if(d->count==payload) d->pending.checksum=byte;
        else { d->state=RX_WAIT_BREAK; return false; }
        d->count++;
        /* One checksum follows the expected payload. */
        if(d->count==(uint8_t)(payload+1u)) return finish(d,out);
        return false;
    }
    default:
        d->state=RX_WAIT_BREAK;
        return false;
    }
}

bool lin_decoder_timeout(lin_decoder_t *d,uint32_t now,lin_frame_t *out) {
    if(d->state==RX_DATA && d->count>=2u && (uint32_t)(now-d->last_byte_us)>1500u)
        return finish(d,out);
    if(d->state!=RX_WAIT_BREAK && (uint32_t)(now-d->last_byte_us)>5000u)
        d->state=RX_WAIT_BREAK;
    return false;
}

void lin_send_header(lin_side_t side,uint8_t id) {
    /* USART hardware emits a standards-compliant 13-bit dominant break. */
    board_uart_write((unsigned)side,0x00u);
    board_delay_us(700u);
    board_uart_write((unsigned)side,0x55u);
    board_uart_write((unsigned)side,lin_make_pid(id));
}

void lin_send_response(lin_side_t side,const lin_frame_t *frame) {
    for(uint8_t i=0;i<frame->length;i++) board_uart_write((unsigned)side,frame->data[i]);
    uint8_t check=lin_checksum(frame->data,frame->length,frame->pid,frame->checksum_type);
    board_uart_write((unsigned)side,check);
}
