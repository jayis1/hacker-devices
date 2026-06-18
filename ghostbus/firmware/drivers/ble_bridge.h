/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * BLE Bridge — encrypted BLE 5.0 transport to operator phone
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GHOSTBUS_BLE_BRIDGE_H
#define GHOSTBUS_BLE_BRIDGE_H

#include "registers.h"
#include "board.h"

/* GHOSTBUS GATT service UUID (custom) */
#define GB_BLE_SERVICE_UUID_128 \
    0x8a, 0x7b, 0x1c, 0x2d, 0x3e, 0x4f, 0x50, 0x61, \
    0x72, 0x83, 0x94, 0xa5, 0xb6, 0xc7, 0xd8, 0xe9

/* Characteristic UUID suffixes */
#define GB_CHAR_COMMAND   0x01
#define GB_CHAR_TELEMETRY  0x02
#define GB_CHAR_DATA      0x03
#define GB_CHAR_AUTH      0x04

typedef struct {
    uint8_t  session_id;
    uint16_t sequence;
    uint32_t total_length;
    uint8_t  sha256[8]; /* truncated SHA-256 for per-chunk integrity */
} gb_chunk_header_t;

typedef enum {
    GB_MSG_DISCOVER = 0x01,
    GB_MSG_SWD_READ = 0x02,
    GB_MSG_SWD_WRITE = 0x03,
    GB_MSG_SWD_HALT = 0x04,
    GB_MSG_SWD_RESUME = 0x05,
    GB_MSG_FLASH_EXTRACT = 0x06,
    GB_MSG_JTAG_SCAN = 0x07,
    GB_MSG_JTAG_READ = 0x08,
    GB_MSG_POWER_ON = 0x09,
    GB_MSG_POWER_OFF = 0x0A,
    GB_MSG_RDP_EXPLOIT = 0x0B,
    GB_MSG_STATUS = 0x0C,
    GB_MSG_LOCK = 0x0D,
    GB_MSG_MEM_READ = 0x0E,
    GB_MSG_MEM_WRITE = 0x0F
} gb_msg_type_t;

typedef struct {
    gb_msg_type_t type;
    uint8_t       *payload;
    uint32_t      payload_len;
} gb_message_t;

/* Public API */
void          ble_bridge_init(void);
gb_status_t   ble_bridge_send_telemetry(const uint8_t *data, uint16_t len);
gb_status_t   ble_bridge_send_data(uint8_t session_id, const uint8_t *data,
                                     uint32_t len);
gb_status_t   ble_bridge_send_event(const char *json_event);
int           ble_bridge_recv_message(gb_message_t *msg, uint32_t timeout_ms);
void          ble_bridge_set_session_key(const uint8_t *key, uint32_t key_len);
gb_status_t   ble_bridge_ecdh_handshake(const uint8_t *peer_pubkey,
                                         uint8_t *session_key);
uint8_t       ble_bridge_is_connected(void);
uint8_t       ble_bridge_is_authenticated(void);
void          ble_bridge_disconnect(void);

#endif /* GHOSTBUS_BLE_BRIDGE_H */