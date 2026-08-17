/*
 * ble_c2.h — BLE C2 protocol (framed, AES-256-CTR encrypted)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_BLE_C2_H
#define PULSEREAPER_BLE_C2_H

#include <stdint.h>
#include "protocol_detect.h"

/* Command opcodes (from app to device) */
#define BLE_CMD_PING              0x01u
#define BLE_CMD_GET_STATUS        0x02u
#define BLE_CMD_ENTER_MODE        0x03u
#define BLE_CMD_FIRE_TDR          0x04u
#define BLE_CMD_SET_GAIN          0x05u
#define BLE_CMD_SET_COUPLING      0x06u
#define BLE_CMD_INJECT_FRAME      0x07u
#define BLE_CMD_STOP_SNIFF        0x08u
#define BLE_CMD_FIRMWARE_VERSION  0x09u

/* Notification opcodes (from device to app) */
#define BLE_NOTIF_STATUS         0x80u
#define BLE_NOTIF_FRAME           0x81u
#define BLE_NOTIF_TDR_RESULT      0x82u

void ble_c2_init(void);
void ble_c2_start_advertising(const char *name);
int  ble_c2_is_connected(void);

/* Poll for received bytes. Returns number available; *out_len is set. */
uint16_t ble_c2_poll_rx(uint8_t *buf, uint16_t max, uint16_t *out_len);

/* Send a status string (null-terminated). */
void ble_c2_send_status(const char *msg);

/* Send a parsed frame as a notification. */
void ble_c2_send_frame(const pr_parsed_t *parsed);

/* Send raw bytes (used by the TDR engine to stream the reflectogram). */
void ble_c2_send_raw(const uint8_t *data, uint16_t len);

#endif