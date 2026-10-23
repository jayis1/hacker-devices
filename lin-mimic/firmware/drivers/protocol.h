/* USB control protocol. Author: jayis1. */
#ifndef LIN_MIMIC_PROTOCOL_H
#define LIN_MIMIC_PROTOCOL_H
#include <stdint.h>
#include <stdbool.h>
#include "lin.h"
#include "policy.h"
typedef struct { uint8_t data[96]; uint8_t used; bool escaped; } protocol_parser_t;
typedef struct {
 uint32_t frames_seen; uint32_t frames_forwarded; uint32_t frames_injected;
 uint32_t denied; uint32_t parity_errors; uint32_t checksum_errors;
} device_counters_t;
void protocol_init(protocol_parser_t *parser);
bool protocol_feed(protocol_parser_t *parser,uint8_t byte,uint8_t *command,uint8_t *payload,uint8_t *length);
void protocol_send_frame(const lin_frame_t *frame);
void protocol_send_status(const safety_policy_t *policy,const device_counters_t *counts,uint16_t mv,int16_t ma);
void protocol_send_ack(uint8_t command,uint8_t status);
uint16_t protocol_crc16(const uint8_t *data,unsigned length);
#endif
