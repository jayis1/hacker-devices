/*
 * radio.h - Operator link framing for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_RADIO_H
#define POE_WHISPER_RADIO_H

#include "../board.h"

typedef enum {
    PW_OP_HELLO = 1,
    PW_OP_STATUS,
    PW_OP_LOAD_PROFILE,
    PW_OP_ARM_BROWNOUT,
    PW_OP_SET_MODE,
    PW_OP_ACK,
    PW_OP_NACK
} pw_radio_opcode_t;

typedef struct {
    pw_radio_opcode_t opcode;
    uint8_t payload[PW_MAX_RADIO_FRAME];
    size_t length;
} pw_radio_frame_t;

void pw_radio_init(void);
size_t pw_radio_encode(pw_radio_opcode_t opcode, const uint8_t *payload, size_t length, uint8_t *out, size_t out_len);
int pw_radio_decode(const uint8_t *buf, size_t len, pw_radio_frame_t *out);
uint8_t pw_radio_crc8(const uint8_t *buf, size_t len);

#endif
