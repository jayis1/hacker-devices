/*
 * ble_bridge.h — nRF52820 BLE backhaul bridge for AxleTap
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 */
#ifndef AXLETAP_BLE_BRIDGE_H
#define AXLETAP_BLE_BRIDGE_H

#include <stdint.h>

#define BLE_MTU 247
#define BLE_HEADER_LEN 4
#define BLE_MAX_PAYLOAD (BLE_MTU - BLE_HEADER_LEN)

/* Frame types for the BLE protocol */
#define BLE_MSG_STATUS     0x01
#define BLE_MSG_FRAME_SUM  0x02
#define BLE_MSG_GPTP_STATE 0x03
#define BLE_MSG_SVC_MAP    0x04
#define BLE_MSG_ARM        0x05
#define BLE_MSG_DISARM     0x06
#define BLE_MSG_CMD        0x07
#define BLE_MSG_LOG        0x08

void ble_init(void);
void ble_poll(void);
int  ble_send(uint8_t msg_type, const uint8_t *payload, uint16_t len);

/* Called when a command is received over BLE from the operator app. */
typedef void (*ble_cmd_cb_t)(const uint8_t *payload, uint16_t len);
void ble_set_cmd_callback(ble_cmd_cb_t cb);

/* Build an encrypted frame (AES-CCM with session key). The session key
 * is derived from a shared operator passphrase via PBKDF2-HMAC-SHA256.
 */
int  ble_encrypt(uint8_t *buf, uint16_t *len);
int  ble_decrypt(uint8_t *buf, uint16_t len);

uint8_t ble_is_connected(void);

#endif /* AXLETAP_BLE_BRIDGE_H */