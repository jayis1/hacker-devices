/*
 * radio.c - Operator link framing for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <string.h>
#include "radio.h"
#include "../registers.h"

void pw_radio_init(void)
{
    memset(&PW_RADIO, 0, sizeof(PW_RADIO));
}

uint8_t pw_radio_crc8(const uint8_t *buf, size_t len)
{
    uint8_t crc = 0x42u;
    for (size_t i = 0; i < len; ++i) {
        crc ^= buf[i];
        for (uint8_t b = 0; b < 8; ++b) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

size_t pw_radio_encode(pw_radio_opcode_t opcode, const uint8_t *payload, size_t length, uint8_t *out, size_t out_len)
{
    if (length + 4 > out_len || length > PW_MAX_RADIO_FRAME) {
        return 0;
    }
    out[0] = 0xA5u;
    out[1] = (uint8_t)opcode;
    out[2] = (uint8_t)length;
    if (length && payload) {
        memcpy(&out[3], payload, length);
    }
    out[3 + length] = pw_radio_crc8(&out[1], length + 2);
    PW_RADIO.tx_count++;
    PW_RADIO.last_opcode = opcode;
    return length + 4;
}

int pw_radio_decode(const uint8_t *buf, size_t len, pw_radio_frame_t *out)
{
    if (!buf || len < 4 || buf[0] != 0xA5u) {
        return -1;
    }
    uint8_t payload_len = buf[2];
    if ((size_t)payload_len + 4 != len) {
        PW_RADIO.crc_errors++;
        return -2;
    }
    if (pw_radio_crc8(&buf[1], payload_len + 2) != buf[len - 1]) {
        PW_RADIO.crc_errors++;
        return -3;
    }
    out->opcode = (pw_radio_opcode_t)buf[1];
    out->length = payload_len;
    memcpy(out->payload, &buf[3], payload_len);
    PW_RADIO.rx_count++;
    PW_RADIO.last_opcode = out->opcode;
    return 0;
}
