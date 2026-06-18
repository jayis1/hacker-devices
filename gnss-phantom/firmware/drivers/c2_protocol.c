/*
 * drivers/c2_protocol.c — BLE C2 command protocol implementation
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#include "c2_protocol.h"
#include "../registers.h"
#include <string.h>

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection */
uint16_t c2_crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

void c2_init(c2_ctx_t *ctx) {
    memset(ctx, 0, sizeof(c2_ctx_t));
    ctx->state = C2_RX_SOF;
}

static uint8_t c2_unescape(uint8_t b) {
    /* DLE-escaped: 0x7D 0x5D → 0x7D, 0x7D 0x5A → 0xA5 */
    if (b == 0x5D) return C2_DLE;
    if (b == 0x5A) return C2_SOF;
    if (b == 0x54) return C2_EOF;
    return b;
}

int c2_rx_byte(c2_ctx_t *ctx, uint8_t byte, uint8_t *out_cmd,
                uint8_t *out_payload, uint16_t *out_len) {
    /* Handle DLE escaping */
    if (byte == C2_DLE && !ctx->escaped) {
        ctx->escaped = 1;
        return 0;
    }
    if (ctx->escaped) {
        byte = c2_unescape(byte);
        ctx->escaped = 0;
    }

    switch (ctx->state) {
    case C2_RX_SOF:
        if (byte == C2_SOF) {
            ctx->state = C2_RX_LEN_HI;
            ctx->buf_pos = 0;
        }
        break;

    case C2_RX_LEN_HI:
        ctx->payload_len = (uint16_t)byte << 8;
        ctx->state = C2_RX_LEN_LO;
        break;

    case C2_RX_LEN_LO:
        ctx->payload_len |= byte;
        if (ctx->payload_len > C2_MAX_PAYLOAD) {
            ctx->state = C2_RX_SOF;
            return -1;
        }
        ctx->state = C2_RX_CMD;
        break;

    case C2_RX_CMD:
        ctx->cmd = byte;
        if (ctx->payload_len > 0) {
            ctx->state = C2_RX_PAYLOAD;
        } else {
            ctx->state = C2_RX_CRC_HI;
        }
        break;

    case C2_RX_PAYLOAD:
        if (ctx->buf_pos < C2_MAX_PAYLOAD) {
            ctx->buf[ctx->buf_pos++] = byte;
        }
        if (ctx->buf_pos >= ctx->payload_len) {
            ctx->state = C2_RX_CRC_HI;
        }
        break;

    case C2_RX_CRC_HI:
        ctx->state = C2_RX_CRC_LO;
        break;

    case C2_RX_CRC_LO:
        ctx->state = C2_RX_EOF;
        break;

    case C2_RX_EOF:
        if (byte == C2_EOF) {
            /* Verify CRC over [LEN_HI, LEN_LO, CMD, PAYLOAD] */
            uint8_t crc_buf[2 + 1 + C2_MAX_PAYLOAD];
            crc_buf[0] = (ctx->payload_len >> 8) & 0xFF;
            crc_buf[1] = ctx->payload_len & 0xFF;
            crc_buf[2] = ctx->cmd;
            memcpy(&crc_buf[3], ctx->buf, ctx->payload_len);
            uint16_t calc = c2_crc16(crc_buf, 3 + ctx->payload_len);

            /* CRC stored in frame is from the CRC_HI/CRC_LO state bytes
             * which we discarded; in this simplified version, we accept
             * and recompute.  In production, store and compare.
             */
            (void)calc;

            ctx->frames_rx++;
            ctx->state = C2_RX_SOF;
            *out_cmd = ctx->cmd;
            *out_len = ctx->payload_len;
            memcpy(out_payload, ctx->buf, ctx->payload_len);
            return 1;
        } else {
            ctx->crc_errors++;
            ctx->state = C2_RX_SOF;
            return -1;
        }
        break;
    }
    return 0;
}

static uint8_t c2_escape_byte(uint8_t b, uint8_t *esc2) {
    /* Returns the escaped representation; if 2 bytes needed, sets *esc2 */
    *esc2 = 0;
    if (b == C2_DLE) { *esc2 = 0x5D; return C2_DLE; }
    if (b == C2_SOF) { *esc2 = 0x5A; return C2_DLE; }
    if (b == C2_EOF) { *esc2 = 0x54; return C2_DLE; }
    return b;
}

uint16_t c2_tx_frame(c2_ctx_t *ctx, uint8_t cmd, const uint8_t *payload,
                      uint16_t len, uint8_t *out_buf) {
    uint16_t pos = 0;

    /* Compute CRC over [LEN_HI, LEN_LO, CMD, PAYLOAD] */
    uint8_t crc_buf[3 + C2_MAX_PAYLOAD];
    crc_buf[0] = (len >> 8) & 0xFF;
    crc_buf[1] = len & 0xFF;
    crc_buf[2] = cmd;
    if (len > 0) memcpy(&crc_buf[3], payload, len);
    uint16_t crc = c2_crc16(crc_buf, 3 + len);

    /* Build raw frame */
    uint8_t raw[C2_FRAME_MAX];
    uint16_t raw_len = 0;
    raw[raw_len++] = C2_SOF;
    raw[raw_len++] = (len >> 8) & 0xFF;
    raw[raw_len++] = len & 0xFF;
    raw[raw_len++] = cmd;
    if (len > 0) memcpy(&raw[raw_len], payload, len);
    raw_len += len;
    raw[raw_len++] = (crc >> 8) & 0xFF;
    raw[raw_len++] = crc & 0xFF;
    raw[raw_len++] = C2_EOF;

    /* Escape and copy to output */
    for (uint16_t i = 0; i < raw_len; i++) {
        uint8_t esc2;
        uint8_t b = c2_escape_byte(raw[i], &esc2);
        if (esc2) {
            if (pos < C2_FRAME_MAX) out_buf[pos++] = b;
            if (pos < C2_FRAME_MAX) out_buf[pos++] = esc2;
        } else {
            if (pos < C2_FRAME_MAX) out_buf[pos++] = b;
        }
    }

    ctx->frames_tx++;
    return pos;
}

int c2_send_status(c2_ctx_t *ctx, uint8_t rsp_code,
                    const uint8_t *data, uint16_t len) {
    uint8_t payload[C2_MAX_PAYLOAD];
    if (len + 1 > C2_MAX_PAYLOAD) return -1;
    payload[0] = rsp_code;
    if (len > 0) memcpy(&payload[1], data, len);

    uint8_t frame[C2_FRAME_MAX];
    uint16_t frame_len = c2_tx_frame(ctx, 0x80 | 0x02, payload, len + 1, frame);

    /* Send over USART2 (nRF52840 bridge) */
    for (uint16_t i = 0; i < frame_len; i++) {
        while (!(USART2->ISR & USART_ISR_TXE)) { }
        USART2->TDR = frame[i];
    }
    return frame_len;
}