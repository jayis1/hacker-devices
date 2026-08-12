/**
 * @file protocol_handler.c
 * @brief Binary protocol handler implementation
 *
 * Parses incoming command packets from the companion app and dispatches
 * them to the appropriate handler functions via the ble_on_command callback.
 *
 * Packet format:
 *   [SYNC1][SYNC2][CMD][PARAM_LEN(2)][PARAMS...][CRC8]
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "protocol_handler.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* Protocol constants */
#define PROTO_SYNC1             0xAA
#define PROTO_SYNC2             0x55
#define PROTO_MAX_PARAM_LEN     248
#define PROTO_HEADER_SIZE       5    /* SYNC1 + SYNC2 + CMD + LEN_HI + LEN_LO */

/* Parser state */
typedef enum {
    PARSE_WAIT_SYNC1,
    PARSE_WAIT_SYNC2,
    PARSE_WAIT_CMD,
    PARSE_WAIT_LEN_HI,
    PARSE_WAIT_LEN_LO,
    PARSE_WAIT_PARAMS,
    PARSE_WAIT_CRC
} parse_state_t;

/* External callback (defined in main.c) */
extern void ble_on_command(uint8_t command, const uint8_t *params, uint16_t param_len);

/* CRC8 lookup table (polynomial 0x07) */
static const uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x16, 0x11, 0x18, 0x1F, 0x0A, 0x0D, 0x04, 0x03,
    0xD6, 0xD1, 0xD8, 0xDF, 0xCA, 0xCD, 0xC4, 0xC3,
    0xEE, 0xE9, 0xE0, 0xE7, 0xF2, 0xF5, 0xFC, 0xFB,
    0xA6, 0xA1, 0xA8, 0xAF, 0xBA, 0xBD, 0xB4, 0xB3,
    0x9E, 0x99, 0x90, 0x97, 0x82, 0x85, 0x8C, 0x8B
};

static uint8_t calculate_crc8(const uint8_t *data, uint16_t length)
{
    uint8_t crc = 0;
    for (uint16_t i = 0; i < length; i++) {
        crc = crc8_table[crc ^ data[i]];
    }
    return crc;
}

void protocol_handler_init(protocol_state_t *state)
{
    memset(state, 0, sizeof(protocol_state_t));
}

void protocol_handler_receive_data(protocol_state_t *state, const uint8_t *data, uint16_t length)
{
    static parse_state_t parse_state = PARSE_WAIT_SYNC1;
    static uint8_t cmd = 0;
    static uint16_t param_len = 0;
    static uint16_t param_idx = 0;

    for (uint16_t i = 0; i < length; i++) {
        uint8_t byte = data[i];

        switch (parse_state) {
        case PARSE_WAIT_SYNC1:
            if (byte == PROTO_SYNC1) {
                parse_state = PARSE_WAIT_SYNC2;
            }
            break;

        case PARSE_WAIT_SYNC2:
            if (byte == PROTO_SYNC2) {
                parse_state = PARSE_WAIT_CMD;
            } else {
                parse_state = PARSE_WAIT_SYNC1;
            }
            break;

        case PARSE_WAIT_CMD:
            cmd = byte;
            parse_state = PARSE_WAIT_LEN_HI;
            break;

        case PARSE_WAIT_LEN_HI:
            param_len = (uint16_t)byte << 8;
            parse_state = PARSE_WAIT_LEN_LO;
            break;

        case PARSE_WAIT_LEN_LO:
            param_len |= byte;
            if (param_len > PROTO_MAX_PARAM_LEN) {
                /* Parameter too long — discard */
                parse_state = PARSE_WAIT_SYNC1;
            } else if (param_len == 0) {
                /* No parameters — wait for CRC */
                parse_state = PARSE_WAIT_CRC;
            } else {
                param_idx = 0;
                parse_state = PARSE_WAIT_PARAMS;
            }
            break;

        case PARSE_WAIT_PARAMS:
            state->rx_buffer[param_idx++] = byte;
            if (param_idx >= param_len) {
                parse_state = PARSE_WAIT_CRC;
            }
            break;

        case PARSE_WAIT_CRC:
            {
                /* Verify CRC */
                uint8_t calc_crc = calculate_crc8(state->rx_buffer, param_len);
                /* CRC is over CMD + LEN + PARAMS */
                uint8_t crc_data[3] = { cmd, (uint8_t)(param_len >> 8), (uint8_t)(param_len & 0xFF) };
                calc_crc ^= calculate_crc8(crc_data, 3);

                if (byte == calc_crc) {
                    /* Valid packet — queue command */
                    state->pending_command = cmd;
                    state->pending_param_len = param_len;
                    memcpy(state->pending_params, state->rx_buffer, param_len);
                    state->command_pending = true;
                }
            }
            parse_state = PARSE_WAIT_SYNC1;
            break;

        default:
            parse_state = PARSE_WAIT_SYNC1;
            break;
        }
    }
}

bool protocol_handler_has_pending_command(const protocol_state_t *state)
{
    return state->command_pending;
}

void protocol_handler_process_command(protocol_state_t *state)
{
    if (!state->command_pending) return;

    /* Dispatch to command handler (defined in main.c) */
    ble_on_command(state->pending_command, state->pending_params, 
                   state->pending_param_len);

    state->command_pending = false;
}

void protocol_handler_send_status(const protocol_state_t *state, 
                                   uint8_t msg_type, uint8_t sys_state)
{
    /* Build and send status message
     * [SYNC1][SYNC2][MSG_TYPE][SYS_STATE][BAT_MV(2)][TEMP][CRC8]
     */
    uint8_t msg[8] = { PROTO_SYNC1, PROTO_SYNC2, msg_type, sys_state, 0, 0, 0, 0 };
    /* CRC would be calculated and appended */
    /* Sent via BLE or USB */
    (void)state;
}

void protocol_handler_send_capture_stats(const protocol_state_t *state,
    const void *stats, uint16_t bat_mv, int8_t temp, uint8_t sys_state)
{
    /* Build and send capture statistics message */
    (void)state;
    (void)stats;
    (void)bat_mv;
    (void)temp;
    (void)sys_state;
}

void protocol_handler_send_device_info(const protocol_state_t *state,
    const uint8_t *device_id, display_protocol_t protocol,
    uint16_t width, uint16_t height, uint8_t depth,
    uint8_t fps, trigger_mode_t trigger)
{
    /* Build and send device info message */
    (void)state;
    (void)device_id;
    (void)protocol;
    (void)width;
    (void)height;
    (void)depth;
    (void)fps;
    (void)trigger;
}