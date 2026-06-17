/**
 * @file    wifi_manager.h
 * @brief   WiFi/HTTP management — header
 * @author  jayis1
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi_types.h"

/**
 * @brief Initialize WiFi manager
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Start WiFi in AP (access point) mode
 */
esp_err_t wifi_manager_start_ap(void);

/**
 * @brief Start WiFi in STA (station/client) mode
 * @param ssid Network SSID
 * @param password Network password
 */
esp_err_t wifi_manager_start_sta(const char *ssid, const char *password);

/**
 * @brief Get current WiFi mode
 */
wifi_mode_t wifi_manager_get_mode(void);

/**
 * @brief Check if device has active connection
 */
bool wifi_manager_is_connected(void);

/**
 * @brief FreeRTOS task for WiFi management
 */
void wifi_task(void *pvParameters);

#endif /* WIFI_MANAGER_H */
