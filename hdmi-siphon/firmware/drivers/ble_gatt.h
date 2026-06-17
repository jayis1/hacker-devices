/**
 * @file    ble_gatt.h
 * @brief   BLE GATT service — header
 * @author  jayis1
 */

#ifndef BLE_GATT_H
#define BLE_GATT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

/**
 * @brief Initialize BLE GATT server
 */
esp_err_t ble_gatt_init(void);

/**
 * @brief Check if a BLE client is connected
 */
bool ble_gatt_is_connected(void);

/**
 * @brief Send JSON status notification to connected client
 * @param json_status Null-terminated JSON string
 */
esp_err_t ble_gatt_send_status(const char *json_status);

/**
 * @brief Send frame data to connected client
 * @param data Frame buffer
 * @param len Data length
 */
esp_err_t ble_gatt_send_frame(const uint8_t *data, size_t len);

/**
 * @brief FreeRTOS task for BLE management
 */
void ble_task(void *pvParameters);

#endif /* BLE_GATT_H */
