/*
 * TRACE-REAPER — BLE bridge driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef BLE_BRIDGE_H
#define BLE_BRIDGE_H

#include <stdint.h>
#include "../board.h"

/* Command set (MCU <- app). Mirrored in ble_bridge.c. */
#define CMD_HELLO        0x01
#define CMD_AUTH_PASSKEY 0x02
#define CMD_GET_STATUS   0x03
#define CMD_CONFIGURE    0x04
#define CMD_ARM          0x05
#define CMD_STOP         0x06
#define CMD_GET_RESULT   0x07
#define CMD_DUMP_TRACE   0x08
#define CMD_LIST_SESSIONS 0x09
#define CMD_OPEN_SESSION 0x0A
#define CMD_SET_INPUT    0x0B
#define CMD_SET_GAIN     0x0C
#define CMD_SET_TRIG     0x0D
#define CMD_WIPE         0x0E

void ble_bridge_init(void);
int  ble_bridge_poll(uint8_t *out_cmd, uint8_t *out_payload, uint8_t *out_len);
int  ble_bridge_is_secure(void);
void ble_bridge_set_encrypted(int en);
int  ble_bridge_authenticate(const uint8_t passkey[6]);
void ble_bridge_disconnect(void);

void ble_bridge_notify_status(uint32_t uptime_s, uint8_t battery_pct,
                               device_mode_t mode, uint32_t traces);
void ble_bridge_notify_live_trace(const int16_t *samples, uint16_t n);
void ble_bridge_notify_corr_best(const uint8_t best[KEY_BYTES_AES256],
                                  const float rho[KEY_BYTES_AES256],
                                  uint8_t nbytes);
void ble_bridge_notify_result(const session_result_t *r, uint8_t nbytes);
void ble_bridge_notify_tamper(void);
void ble_bridge_notify_fault(const char *msg);

#endif /* BLE_BRIDGE_H */