/* Framed USB transport. Author: jayis1. */
#include "protocol.h"
#include "../board.h"
#include <string.h>
#define SOF 0x7eu
#define ESC 0x7du
#define XOR 0x20u

uint16_t protocol_crc16(const uint8_t *data,unsigned length) {
 uint16_t crc=0xffffu;
 for(unsigned i=0;i<length;i++) {
  crc^=(uint16_t)data[i]<<8;
  for(unsigned bit=0;bit<8;bit++) crc=(crc&0x8000u)?(uint16_t)((crc<<1)^0x1021u):(uint16_t)(crc<<1);
 }
 return crc;
}
void protocol_init(protocol_parser_t *p){ memset(p,0,sizeof(*p)); }

bool protocol_feed(protocol_parser_t *p,uint8_t b,uint8_t *command,uint8_t *payload,uint8_t *length) {
 if(b==SOF){ p->used=0;p->escaped=false;return false; }
 if(b==ESC){ p->escaped=true;return false; }
 if(p->escaped){ b^=XOR;p->escaped=false; }
 if(p->used>=sizeof(p->data)){ p->used=0;return false; }
 p->data[p->used++]=b;
 if(p->used<2)return false;
 uint8_t expected=p->data[1];
 if(expected>88u){p->used=0;return false;}
 if(p->used!=(uint8_t)(expected+4u))return false;
 uint16_t supplied=(uint16_t)p->data[p->used-2]|((uint16_t)p->data[p->used-1]<<8);
 uint16_t actual=protocol_crc16(p->data,p->used-2);
 if(supplied!=actual){p->used=0;return false;}
 *command=p->data[0];*length=expected;
 memcpy(payload,&p->data[2],expected);p->used=0;return true;
}

static void escaped(uint8_t b) {
 if(b==SOF||b==ESC){uint8_t pair[2]={ESC,(uint8_t)(b^XOR)};board_usb_write(pair,2);}
 else board_usb_write(&b,1);
}
static void packet(uint8_t type,const uint8_t *data,uint8_t length) {
 uint8_t raw[94];raw[0]=type;raw[1]=length;
 if(length)memcpy(&raw[2],data,length);
 uint16_t crc=protocol_crc16(raw,(unsigned)length+2u);
 raw[length+2u]=(uint8_t)crc;raw[length+3u]=(uint8_t)(crc>>8);
 uint8_t sof=SOF;board_usb_write(&sof,1);
 for(unsigned i=0;i<(unsigned)length+4u;i++)escaped(raw[i]);
}
void protocol_send_frame(const lin_frame_t *f) {
 uint8_t p[20];unsigned n=0;
 p[n++]=(uint8_t)f->timestamp_us;p[n++]=(uint8_t)(f->timestamp_us>>8);
 p[n++]=(uint8_t)(f->timestamp_us>>16);p[n++]=(uint8_t)(f->timestamp_us>>24);
 p[n++]=f->pid;p[n++]=f->length;p[n++]=(uint8_t)f->source;
 p[n++]=(uint8_t)((f->parity_ok?1:0)|(f->checksum_ok?2:0)|(f->collision?4:0));
 for(uint8_t i=0;i<f->length;i++)p[n++]=f->data[i];
 p[n++]=f->checksum;packet(0x81,p,(uint8_t)n);
}
void protocol_send_status(const safety_policy_t *policy,const device_counters_t *c,uint16_t mv,int16_t ma) {
 uint8_t p[32];unsigned n=0;
 #define PUT32(v) do{uint32_t x=(v);p[n++]=(uint8_t)x;p[n++]=(uint8_t)(x>>8);p[n++]=(uint8_t)(x>>16);p[n++]=(uint8_t)(x>>24);}while(0)
 p[n++]=(uint8_t)policy->mode;p[n++]=(uint8_t)policy->physically_armed;p[n++]=(uint8_t)policy->session_unlocked;
 p[n++]=(uint8_t)mv;p[n++]=(uint8_t)(mv>>8);p[n++]=(uint8_t)ma;p[n++]=(uint8_t)(ma>>8);
 PUT32(c->frames_seen);PUT32(c->frames_forwarded);PUT32(c->frames_injected);PUT32(c->denied);
 packet(0x82,p,(uint8_t)n);
 #undef PUT32
}
void protocol_send_ack(uint8_t command,uint8_t status){uint8_t p[2]={command,status};packet(0x80,p,2);}
