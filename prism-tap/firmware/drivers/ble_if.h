/*
 * drivers/ble_if.h — BLE C2 Interface for Prism-Tap
 *
 * UART link to nRF52840 BLE module. Handles encrypted command/response
 * protocol with AES-256-CTR.
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_BLE_IF_H
#define PRISM_TAP_BLE_IF_H

#include <stdint.h>
#include <stddef.h>

/* ---- BLE command opcodes ---- */
#define CMD_PING            0x01
#define CMD_GET_STATUS      0x02
#define CMD_SET_MODE        0x03
#define CMD_START_CAPTURE   0x10
#define CMD_STOP_CAPTURE    0x11
#define CMD_GET_FRAME_THUMB 0x12
#define CMD_GET_FRAME_LIST  0x13
#define CMD_LOAD_INJECT     0x20
#define CMD_START_INJECT    0x21
#define CMD_STOP_INJECT     0x22
#define CMD_SET_INJECT_MODE 0x23
#define CMD_SET_TIMING      0x30
#define CMD_DROP_FRAMES     0x31
#define CMD_GET_CONFIG      0x40
#define CMD_SET_CONFIG      0x41
#define CMD_ERASE_FRAMES    0x50
#define CMD_EXPORT_FRAMES   0x51
#define CMD_FW_UPDATE       0x60

#define CMD_RESPONSE        0x80
#define CMD_ERROR           0xFF

/* ---- BLE packet structure ---- */
#define BLE_MAX_PAYLOAD  244   /* BLE characteristic max (MTU dependent) */
#define BLE_AES_KEY_LEN  32    /* AES-256 key */

typedef struct __attribute__((packed)) {
    uint8_t  opcode;
    uint8_t  seq;
    uint16_t length;   /* payload length (little-endian) */
    uint16_t crc;      /* CRC-16 of opcode + seq + length + payload */
    /* payload follows */
} ble_header_t;

/* ---- Initialization ---- */
int  ble_init(void);
int  ble_is_connected(void);
void ble_poll(void);

/* ---- Command handling ---- */
typedef int (*ble_cmd_handler_t)(const uint8_t *payload, uint8_t len,
                                  uint8_t *response, uint8_t *resp_len);

void ble_register_handler(uint8_t opcode, ble_cmd_handler_t handler);
void ble_send_response(uint8_t opcode, uint8_t seq,
                        const uint8_t *payload, uint8_t len);
void ble_send_error(uint8_t opcode, uint8_t seq, uint8_t error_code);

/* ---- AES-256-CTR encryption ---- */
void ble_set_key(const uint8_t key[32]);
void ble_encrypt(const uint8_t *in, uint8_t *out, uint32_t len,
                 const uint8_t nonce[16]);
void ble_decrypt(const uint8_t *in, uint8_t *out, uint32_t len,
                 const uint8_t nonce[16]);

/* ---- CRC-16-CCITT ---- */
uint16_t ble_crc16(const uint8_t *data, uint32_t len);

#endif /* PRISM_TAP_BLE_IF_H */
/* Author: jayis1 */