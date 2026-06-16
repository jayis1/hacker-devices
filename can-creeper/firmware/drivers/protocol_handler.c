/**
 * @file protocol_handler.c
 * @brief Binary Wire Protocol Handler Implementation
 *
 * Full implementation of the CAN Creeper binary protocol.
 * Handles frame serialization/deserialization, CRC-16/CCITT,
 * command dispatch, and transport abstraction (BLE/USB).
 */

#include "protocol_handler.h"
#include "../board.h"
#include "ble_nus_driver.h"
#include "usb_cdc_driver.h"
#include <string.h>

/*===========================================================================
 * CRC-16/CCITT
 *===========================================================================*/

uint16_t protocol_crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = PROTOCOL_CRC_INIT;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ PROTOCOL_CRC_POLY;
            else              crc <<= 1;
        }
    }
    return crc;
}

/*===========================================================================
 * INITIALIZATION
 *===========================================================================*/

int protocol_init(protocol_state_t *state) {
    if (!state) return PROTO_ERR_NOT_INIT;
    memset(state, 0, sizeof(protocol_state_t));
    return PROTO_ERR_OK;
}

/*===========================================================================
 * FRAME SERIALIZATION
 *===========================================================================*/

/**
 * @brief Serialize a received CAN frame for transmission to app
 *
 * Format: START(0xAA) CMD(0x01) LEN(2) TIMESTAMP(4) CHANNEL(1) FLAGS(1) ID(4) DLC(1) DATA(N) CRC(2)
 */
int protocol_serialize_frame_rx(const can_frame_t *frame, uint8_t channel,
                                 uint8_t *buf, uint16_t *out_len) {
    if (!frame || !buf || !out_len) return PROTO_ERR_INVALID_LEN;

    uint8_t data_len = (frame->dlc > 8) ? frame->dlc : 8;
    if (data_len > 64) data_len = 64;

    uint16_t payload_len = 11 + data_len;  /* ts(4) + ch(1) + flags(1) + id(4) + dlc(1) + data */
    uint16_t total_len = 5 + payload_len;  /* start + cmd + len(2) + payload + crc(2) */

    if (total_len > PROTOCOL_MAX_PAYLOAD + 5) return PROTO_ERR_OVERFLOW;

    uint8_t *p = buf;

    *p++ = PROTOCOL_START_BYTE;
    *p++ = CMD_FRAME_RX;
    *p++ = (uint8_t)(payload_len & 0xFF);
    *p++ = (uint8_t)((payload_len >> 8) & 0xFF);

    /* Timestamp (32-bit LE) */
    *p++ = (uint8_t)(frame->timestamp & 0xFF);
    *p++ = (uint8_t)((frame->timestamp >> 8) & 0xFF);
    *p++ = (uint8_t)((frame->timestamp >> 16) & 0xFF);
    *p++ = (uint8_t)((frame->timestamp >> 24) & 0xFF);

    /* Channel */
    *p++ = channel;

    /* Flags */
    *p++ = frame->flags;

    /* CAN ID (32-bit LE, bits 28:0 valid) */
    uint32_t id = frame->id & 0x1FFFFFFFUL;
    *p++ = (uint8_t)(id & 0xFF);
    *p++ = (uint8_t)((id >> 8) & 0xFF);
    *p++ = (uint8_t)((id >> 16) & 0xFF);
    *p++ = (uint8_t)((id >> 24) & 0xFF);

    /* DLC */
    *p++ = frame->dlc;

    /* Data bytes */
    memcpy(p, frame->data, data_len);
    p += data_len;

    /* CRC-16 over bytes 1 through (p - buf - 1) */
    uint16_t crc = protocol_crc16(buf + 1, (uint16_t)(p - buf - 1));
    *p++ = (uint8_t)(crc & 0xFF);
    *p++ = (uint8_t)((crc >> 8) & 0xFF);

    *out_len = (uint16_t)(p - buf);
    return PROTO_ERR_OK;
}

/**
 * @brief Serialize a status response
 */
int protocol_serialize_status_rsp(uint8_t *buf, uint16_t *out_len,
                                   uint32_t uptime, uint16_t batt_mv,
                                   bool ble_conn, bool usb_conn,
                                   uint8_t can0_tec, uint8_t can0_rec,
                                   uint8_t can1_tec, uint8_t can1_rec,
                                   uint32_t can0_count, uint32_t can1_count,
                                   uint32_t can0_ovf, uint32_t can1_ovf) {
    if (!buf || !out_len) return PROTO_ERR_INVALID_LEN;

    uint16_t payload_len = 29;  /* Fixed size for STATUS_RSP */
    uint8_t *p = buf;

    *p++ = PROTOCOL_START_BYTE;
    *p++ = CMD_STATUS_RSP;
    *p++ = (uint8_t)(payload_len & 0xFF);
    *p++ = (uint8_t)((payload_len >> 8) & 0xFF);

    /* Uptime (32-bit LE, seconds) */
    *p++ = (uint8_t)(uptime & 0xFF);
    *p++ = (uint8_t)((uptime >> 8) & 0xFF);
    *p++ = (uint8_t)((uptime >> 16) & 0xFF);
    *p++ = (uint8_t)((uptime >> 24) & 0xFF);

    /* Battery voltage (16-bit LE, mV) */
    *p++ = (uint8_t)(batt_mv & 0xFF);
    *p++ = (uint8_t)((batt_mv >> 8) & 0xFF);

    /* Connection status */
    *p++ = ble_conn ? 1 : 0;
    *p++ = usb_conn ? 1 : 0;

    /* CAN0 bus status (TEC + REC) */
    *p++ = can0_tec;
    *p++ = can0_rec;

    /* CAN1 bus status */
    *p++ = can1_tec;
    *p++ = can1_rec;

    /* CAN0 frame count (32-bit LE) */
    *p++ = (uint8_t)(can0_count & 0xFF);
    *p++ = (uint8_t)((can0_count >> 8) & 0xFF);
    *p++ = (uint8_t)((can0_count >> 16) & 0xFF);
    *p++ = (uint8_t)((can0_count >> 24) & 0xFF);

    /* CAN1 frame count */
    *p++ = (uint8_t)(can1_count & 0xFF);
    *p++ = (uint8_t)((can1_count >> 8) & 0xFF);
    *p++ = (uint8_t)((can1_count >> 16) & 0xFF);
    *p++ = (uint8_t)((can1_count >> 24) & 0xFF);

    /* CAN0 overflow count */
    *p++ = (uint8_t)(can0_ovf & 0xFF);
    *p++ = (uint8_t)((can0_ovf >> 8) & 0xFF);
    *p++ = (uint8_t)((can0_ovf >> 16) & 0xFF);
    *p++ = (uint8_t)((can0_ovf >> 24) & 0xFF);

    /* CAN1 overflow count */
    *p++ = (uint8_t)(can1_ovf & 0xFF);
    *p++ = (uint8_t)((can1_ovf >> 8) & 0xFF);
    *p++ = (uint8_t)((can1_ovf >> 16) & 0xFF);
    *p++ = (uint8_t)((can1_ovf >> 24) & 0xFF);

    /* CRC-16 */
    uint16_t crc = protocol_crc16(buf + 1, (uint16_t)(p - buf - 1));
    *p++ = (uint8_t)(crc & 0xFF);
    *p++ = (uint8_t)((crc >> 8) & 0xFF);

    *out_len = (uint16_t)(p - buf);
    return PROTO_ERR_OK;
}

/*===========================================================================
 * COMMAND PARSING
 *===========================================================================*/

/**
 * @brief Parse incoming protocol data stream
 *
 * State machine that accumulates bytes until a complete frame is received,
 * verifies CRC, and dispatches the command.
 */
int protocol_parse(protocol_state_t *state, const uint8_t *data, uint16_t len,
                   protocol_transport_t transport) {
    if (!state || !data) return PROTO_ERR_NOT_INIT;

    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data[i];

        if (!state->rx_in_frame) {
            /* Looking for start byte */
            if (byte == PROTOCOL_START_BYTE) {
                state->rx_in_frame = true;
                state->rx_pos = 0;
                state->rx_buf[state->rx_pos++] = byte;
            }
            /* Else: discard byte (out of sync) */
        } else {
            /* Accumulating frame */
            if (state->rx_pos >= sizeof(state->rx_buf)) {
                /* Overflow — reset */
                state->rx_in_frame = false;
                state->rx_pos = 0;
                continue;
            }

            state->rx_buf[state->rx_pos++] = byte;

            /* After receiving cmd + len(2), we know expected length */
            if (state->rx_pos == 4) {
                uint8_t cmd = state->rx_buf[1];
                uint16_t payload_len = state->rx_buf[2] | ((uint16_t)state->rx_buf[3] << 8);
                state->rx_expected_len = 5 + payload_len;  /* start + cmd + len(2) + payload + crc(2) */

                if (state->rx_expected_len > sizeof(state->rx_buf)) {
                    /* Invalid length — reset */
                    state->rx_in_frame = false;
                    state->rx_pos = 0;
                }
            }

            /* Check if frame is complete */
            if (state->rx_pos >= 5 && state->rx_pos == state->rx_expected_len) {
                /* Verify CRC */
                uint16_t received_crc = state->rx_buf[state->rx_pos - 2] |
                                       ((uint16_t)state->rx_buf[state->rx_pos - 1] << 8);
                uint16_t computed_crc = protocol_crc16(state->rx_buf + 1,
                                                       state->rx_pos - 3);  /* Exclude start and CRC */

                if (received_crc == computed_crc) {
                    /* Valid frame — dispatch */
                    uint8_t cmd = state->rx_buf[1];
                    uint16_t payload_len = state->rx_buf[2] | ((uint16_t)state->rx_buf[3] << 8);
                    uint8_t *payload = &state->rx_buf[4];

                    switch (cmd) {
                        case CMD_FRAME_TX:
                            /* Parse and transmit CAN frame */
                            if (payload_len >= 8) {
                                can_frame_t frame;
                                frame.flags = payload[1];
                                frame.id = (uint32_t)payload[2] | ((uint32_t)payload[3] << 8) |
                                          ((uint32_t)payload[4] << 16) | ((uint32_t)payload[5] << 24);
                                frame.id &= 0x1FFFFFFFUL;
                                frame.dlc = payload[6];
                                uint8_t data_len = (frame.dlc > 8) ? frame.dlc : 8;
                                if (data_len > 64) data_len = 64;
                                memcpy(frame.data, &payload[8], data_len);
                                frame.timestamp = 0;
                                can_fd_transmit(payload[0], &frame);
                            }
                            break;

                        case CMD_CONFIG_SET:
                            /* Configuration change — handled by main loop */
                            break;

                        case CMD_CONFIG_GET:
                            /* Respond with current config */
                            break;

                        case CMD_STATUS_REQ:
                            /* Status request — main loop sends STATUS_RSP */
                            break;

                        case CMD_SCRIPT_LOAD:
                        case CMD_SCRIPT_START:
                        case CMD_SCRIPT_STOP:
                            /* Script commands — handled by injection engine */
                            break;

                        case CMD_PING:
                            /* Respond with PONG */
                            {
                                uint8_t pong[5] = { PROTOCOL_START_BYTE, CMD_PONG, 0, 0 };
                                uint16_t crc = protocol_crc16(pong + 1, 2);
                                pong[3] = (uint8_t)(crc & 0xFF);
                                pong[4] = (uint8_t)((crc >> 8) & 0xFF);
                                if (transport == PROTOCOL_TRANSPORT_BLE) {
                                    ble_nus_tx(pong, 5);
                                } else {
                                    usb_cdc_tx(pong, 5);
                                }
                            }
                            break;

                        default:
                            break;
                    }
                }
                /* Reset parser for next frame */
                state->rx_in_frame = false;
                state->rx_pos = 0;
            }
        }
    }

    return PROTO_ERR_OK;
}

/*===========================================================================
 * FRAME QUEUING AND FLUSHING
 *===========================================================================*/

/**
 * @brief Queue a received CAN frame for transmission to app
 */
int protocol_queue_frame(protocol_state_t *state, uint8_t channel, const can_frame_t *frame) {
    if (!state || !frame) return PROTO_ERR_NOT_INIT;

    uint8_t buf[PROTOCOL_MAX_PAYLOAD + 5];
    uint16_t len;

    int ret = protocol_serialize_frame_rx(frame, channel, buf, &len);
    if (ret != PROTO_ERR_OK) return ret;

    /* Try BLE first if connected */
    if (ble_nus_is_connected()) {
        if (ble_nus_tx(buf, len) == 0) {
            return PROTO_ERR_OK;  /* Queued on BLE */
        }
    }

    /* Fall back to USB if connected */
    if (usb_cdc_is_connected()) {
        if (usb_cdc_tx(buf, len) == 0) {
            return PROTO_ERR_OK;  /* Queued on USB */
        }
    }

    /* Neither connected — frame is only in PSRAM ring buffer */
    return PROTO_ERR_OK;
}

/**
 * @brief Flush any pending BLE transmissions
 */
int protocol_flush_ble(protocol_state_t *state) {
    (void)state;
    /* BLE TX is handled asynchronously by SoftDevice events */
    /* This function is a placeholder for any deferred BLE work */
    return PROTO_ERR_OK;
}

/**
 * @brief Flush any pending USB transmissions
 */
int protocol_flush_usb(protocol_state_t *state) {
    (void)state;
    /* USB TX is handled asynchronously by USBD events */
    return PROTO_ERR_OK;
}
