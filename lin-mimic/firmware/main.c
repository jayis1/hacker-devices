/* LIN Mimic controller
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "board.h"
#include "drivers/lin.h"
#include "drivers/policy.h"
#include "drivers/protocol.h"
#include <string.h>

#define CMD_STATUS 0x01u
#define CMD_UNLOCK 0x02u
#define CMD_MODE 0x03u
#define CMD_ALLOW_IDS 0x04u
#define CMD_TRANSMIT 0x05u
#define CMD_LOCK 0x06u

typedef struct {
 lin_decoder_t decoder[2];
 safety_policy_t policy;
 protocol_parser_t host;
 device_counters_t counters;
 lin_frame_t learned[64];
 uint64_t learned_valid;
 uint32_t last_status_us;
} device_t;
static device_t dev;

static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}

static void remember(const lin_frame_t *frame) {
 if(!frame->checksum_ok)return;
 dev.learned[frame->id]=*frame;
 dev.learned_valid|=(uint64_t)1u<<frame->id;
}

static void forward_if_inline(const lin_frame_t *frame) {
 if(dev.policy.mode!=MODE_INLINE)return;
 lin_side_t destination=frame->source==LIN_SIDE_CONTROLLER?LIN_SIDE_TARGET:LIN_SIDE_CONTROLLER;
 lin_send_header(destination,frame->id);
 lin_send_response(destination,frame);
 dev.counters.frames_forwarded++;
}

static void accept_frame(const lin_frame_t *frame) {
 dev.counters.frames_seen++;
 if(!frame->parity_ok)dev.counters.parity_errors++;
 if(!frame->checksum_ok)dev.counters.checksum_errors++;
 remember(frame);
 protocol_send_frame(frame);
 forward_if_inline(frame);
}

static bool decode_host_frame(const uint8_t *p,uint8_t n,lin_frame_t *frame) {
 if(n<4u)return false;
 memset(frame,0,sizeof(*frame));
 frame->id=(uint8_t)(p[0]&0x3fu);frame->pid=lin_make_pid(frame->id);
 frame->length=p[1];frame->checksum_type=(lin_checksum_t)p[2];frame->source=(lin_side_t)(p[3]&1u);
 if(frame->length==0u||frame->length>8u||n!=(uint8_t)(frame->length+4u))return false;
 memcpy(frame->data,&p[4],frame->length);frame->parity_ok=true;frame->checksum_ok=true;
 frame->checksum=lin_checksum(frame->data,frame->length,frame->pid,frame->checksum_type);
 return true;
}

static void command(uint8_t cmd,const uint8_t *payload,uint8_t length,uint32_t now) {
 bool ok=false;
 switch(cmd) {
 case CMD_STATUS:
  protocol_send_status(&dev.policy,&dev.counters,board_target_mv(),board_target_ma());return;
 case CMD_UNLOCK:
  if(length==4u)ok=policy_unlock(&dev.policy,get32(payload),now);
  break;
 case CMD_MODE:
  if(length==1u&&payload[0]<=MODE_FUZZ)ok=policy_set_mode(&dev.policy,(operation_mode_t)payload[0],now);
  break;
 case CMD_ALLOW_IDS:
  if(length==8u&&dev.policy.session_unlocked){
   uint64_t mask=0;for(unsigned i=0;i<8;i++)mask|=(uint64_t)payload[i]<<(i*8u);
   dev.policy.allowed_ids=mask;ok=true;
  }
  break;
 case CMD_TRANSMIT: {
  lin_frame_t frame;
  if(decode_host_frame(payload,length,&frame)&&policy_allow_tx(&dev.policy,&frame,now,board_target_mv())){
   lin_side_t destination=frame.source==LIN_SIDE_CONTROLLER?LIN_SIDE_TARGET:LIN_SIDE_CONTROLLER;
   lin_send_header(destination,frame.id);lin_send_response(destination,&frame);
   dev.counters.frames_injected++;ok=true;
  } else dev.counters.denied++;
  break;
 }
 case CMD_LOCK:
  policy_lock(&dev.policy);ok=true;break;
 default:break;
 }
 protocol_send_ack(cmd,ok?0u:1u);
}

static void service_bus(unsigned channel,uint32_t now) {
 uint8_t byte;lin_frame_t frame;
 int event=board_uart_read(channel,&byte);
 if(event<0)lin_decoder_break(&dev.decoder[channel],now);
 else if(event>0&&lin_decoder_byte(&dev.decoder[channel],byte,now,&frame))accept_frame(&frame);
 if(lin_decoder_timeout(&dev.decoder[channel],now,&frame))accept_frame(&frame);
}

static void service_host(uint32_t now) {
 uint8_t input[64],payload[90],cmd,length;
 int count=board_usb_read(input,sizeof(input));
 for(int i=0;i<count;i++)if(protocol_feed(&dev.host,input[i],&cmd,payload,&length))command(cmd,payload,length,now);
}

static void initialize(void) {
 memset(&dev,0,sizeof(dev));board_init();
 lin_decoder_init(&dev.decoder[0],LIN_SIDE_CONTROLLER);
 lin_decoder_init(&dev.decoder[1],LIN_SIDE_TARGET);
 policy_init(&dev.policy);protocol_init(&dev.host);
 board_set_bypass(true);board_set_transceiver(0,true);board_set_transceiver(1,true);
}

int main(void) {
 initialize();
#ifdef HOST_SIM
 /* Deterministic smoke path for sanitizers and CI. */
 lin_frame_t f={0};f.id=0x12;f.pid=lin_make_pid(f.id);f.length=2;f.data[0]=0x34;f.data[1]=0x56;
 f.checksum_type=LIN_ENHANCED;f.parity_ok=true;f.checksum_ok=true;f.source=LIN_SIDE_CONTROLLER;
 if(!lin_pid_valid(f.pid))return 2;
 if(lin_checksum(f.data,f.length,f.pid,LIN_ENHANCED)==0)return 3;
 accept_frame(&f);
 service_bus(0,board_micros());
 service_host(board_micros());
 return 0;
#else
 for(;;){
  uint32_t now=board_micros();
  policy_update_arm(&dev.policy,board_read_arm(),now);
  service_bus(0,now);service_bus(1,now);service_host(now);
  if((uint32_t)(now-dev.last_status_us)>1000000u){
   protocol_send_status(&dev.policy,&dev.counters,board_target_mv(),board_target_ma());dev.last_status_us=now;
  }
  board_led_pattern((unsigned)dev.policy.mode);board_watchdog_kick();
 }
#endif
}
