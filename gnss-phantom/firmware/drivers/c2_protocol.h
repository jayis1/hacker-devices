/*
 * drivers/c2_protocol.h — BLE C2 command protocol for GNSS-Phantom
 *
 * Implements a framed, CRC-protected command/response protocol between the
 * STM32H743 and an nRF52840 BLE module (which bridges to the companion app
 * over Nordic UART Service).  Uses SLIP-like framing with DLE escaping.
 *
 * Frame format:
 *   [SOF][LEN_HI][LEN_LO][CMD][PAYLOAD...][CRC16_HI][CRC16_LO][EOF]
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#ifndef GNSS_PHANTOM_C2_H
#define GNSS_PHANTOM_C2_H

#include <stdint.h>
#include "../board.h"

#define C2_FRAME_MAX  (C2_MAX_PAYLOAD + 8)

typedef enum {
    C2_RX_SOF = 0,
    C2_RX_LEN_HI,
    C2_RX_LEN_LO,
    C2_RX_CMD,
    C2_RX_PAYLOAD,
    C2_RX_CRC_HI,
    C2_RX_CRC_LO,
    C2_RX_EOF,
} c2_rx_state_t;

typedef struct {
    c2_rx_state_t state;
    uint8_t buf[C2_FRAME_MAX];
    uint16_t buf_pos;
    uint16_t payload_len;
    uint8_t cmd;
    uint8_t escaped;
    uint32_t frames_rx;
    uint32_t frames_tx;
    uint32_t crc_errors;
} c2_ctx_t;

/* CRC-16/CCITT (poly 0x1021, init 0xFFFF) */
uint16_t c2_crc16(const uint8_t *data, uint16_t len);

/* Initialize protocol context */
void c2_init(c2_ctx_t *ctx);

/* Feed a received byte from USART2 (from nRF52840).
 * Returns >0 with command in *out_cmd and payload in *out_payload when
 * a complete frame is received, 0 otherwise.
 */
int c2_rx_byte(c2_ctx_t *ctx, uint8_t byte, uint8_t *out_cmd,
                uint8_t *out_payload, uint16_t *out_len);

/* Encode a command + payload into a frame for TX.
 * Returns frame length.
 */
uint16_t c2_tx_frame(c2_ctx_t *ctx, uint8_t cmd, const uint8_t *payload,
                      uint16_t len, uint8_t *out_buf);

/* Send a status response */
int c2_send_status(c2_ctx_t *ctx, uint8_t rsp_code,
                    const uint8_t *data, uint16_t len);

#endif /* GNSS_PHANTOM_C2_H */