/*
 * lora-phantom / drivers/protocol.c
 * Wire protocol codec: frame encode/decode, CRC16-CCITT.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Frame format:
 *   [SYNC0 0x55][SYNC1 0xAA][LEN u16 LE][CMD u8][PAYLOAD[LEN]][CRC16 u16 LE]
 *
 * The CRC16-CCITT (polynomial 0x1021, init 0xFFFF) covers CMD + PAYLOAD.
 * LEN is the payload length (excludes CMD and CRC).
 */

#include "../board.h"
#include "../registers.h"

/* ---- CRC16-CCITT (polynomial 0x1021, init 0xFFFF) ---- */
uint16_t crc16_ccitt(const uint8_t *data, uint32_t len) {
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc = (crc << 1);
        }
    }
    return crc;
}

/* ---- Encode a command + payload into a framed packet ---- */
int proto_encode(uint8_t cmd, const uint8_t *payload, uint16_t plen,
                 uint8_t *out, uint16_t out_max) {
    uint16_t total = 4 + 1 + plen + 2;   /* sync(2) + len(2) + cmd(1) + payload + crc(2) */
    if (total > out_max) return -1;
    if (plen > PROTO_MAX_PAYLOAD) return -1;

    uint16_t p = 0;
    out[p++] = PROTO_SYNC0;
    out[p++] = PROTO_SYNC1;
    out[p++] = (uint8_t)(plen & 0xFF);
    out[p++] = (uint8_t)(plen >> 8);
    out[p++] = cmd;
    for (uint16_t i = 0; i < plen; i++) out[p++] = payload[i];

    /* CRC over CMD + PAYLOAD */
    uint16_t crc = crc16_ccitt(out + 4, 1 + plen);
    out[p++] = (uint8_t)(crc & 0xFF);
    out[p++] = (uint8_t)(crc >> 8);

    return (int)p;
}

/* ---- Decode a framed packet ---- */
int proto_decode(const uint8_t *buf, uint16_t len, uint8_t *cmd,
                 uint8_t *payload, uint16_t *plen, uint16_t payload_max) {
    if (len < 7) return -1;   /* minimum: sync(2) + len(2) + cmd(1) + crc(2) */
    if (buf[0] != PROTO_SYNC0 || buf[1] != PROTO_SYNC1) return -2;

    uint16_t plen_field = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
    uint16_t expected_total = 4 + 1 + plen_field + 2;
    if (len < expected_total) return -3;
    if (plen_field > payload_max) return -4;

    /* Verify CRC */
    uint16_t crc_recv = (uint16_t)buf[4 + 1 + plen_field] |
                        ((uint16_t)buf[4 + 1 + plen_field + 1] << 8);
    uint16_t crc_calc = crc16_ccitt(buf + 4, 1 + plen_field);
    if (crc_recv != crc_calc) return -5;

    *cmd = buf[4];
    *plen = plen_field;
    for (uint16_t i = 0; i < plen_field; i++) {
        payload[i] = buf[5 + i];
    }

    return 0;
}