/*
 * ACOUSTIC-PHANTOM — BLE bridge driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * UART-based communication with the CYW20819 BLE module. Implements
 * the GATT profile protocol, encrypted event streaming, and file
 * transfer to the companion app.
 */
#ifndef BLE_BRIDGE_H
#define BLE_BRIDGE_H

#include <stdint.h>
#include "tflm_inference.h"

/* GATT command opcodes (app → device) */
#define BLE_CMD_SET_PROFILE          0x01
#define BLE_CMD_ARM                  0x02
#define BLE_CMD_DISARM               0x03
#define BLE_CMD_START_CALIBRATION    0x04
#define BLE_CMD_CALIBRATION_SAMPLE   0x05
#define BLE_CMD_FINISH_CALIBRATION   0x06
#define BLE_CMD_ENABLE_STORAGE       0x07
#define BLE_CMD_DISABLE_STORAGE      0x08
#define BLE_CMD_LIST_FILES           0x09
#define BLE_CMD_DOWNLOAD_FILE        0x0A
#define BLE_CMD_SET_BEAM             0x0B
#define BLE_CMD_GET_STATUS           0x0C
#define BLE_CMD_ERASE_CALIBRATION    0x0D
#define BLE_CMD_SET_ENCRYPTION_KEY   0x0E

/* GATT notification opcodes (device → app) */
#define BLE_NOTIF_EVENT              0x81
#define BLE_NOTIF_AUDIO              0x82
#define BLE_NOTIF_STATUS             0x83
#define BLE_NOTIF_FILE_LIST          0x84
#define BLE_NOTIF_FILE_CHUNK         0x85
#define BLE_NOTIF_CALIB_PROGRESS     0x86

/* Callback types */
typedef void (*ble_connect_cb_t)(void);
typedef void (*ble_disconnect_cb_t)(void);
typedef void (*ble_command_cb_t)(uint8_t cmd, const uint8_t *data, uint16_t len);

void  ble_bridge_init(const char *passkey);
void  ble_bridge_set_callbacks(ble_connect_cb_t on_connect,
                                ble_disconnect_cb_t on_disconnect,
                                ble_command_cb_t on_command);
void  ble_bridge_poll(void);
void  ble_bridge_send_event(const classification_t *result);
void  ble_bridge_send_audio(const int16_t *samples, uint16_t count);
void  ble_bridge_send_status(uint8_t state, uint8_t profile,
                             uint32_t event_count, uint8_t battery);
void  ble_bridge_send_file_list(void);
void  ble_bridge_send_file_chunk(uint32_t file_id, uint32_t offset,
                                  const uint8_t *data, uint16_t len);
void  ble_bridge_send_calibration_progress(uint8_t current, uint8_t total);

#endif /* BLE_BRIDGE_H */