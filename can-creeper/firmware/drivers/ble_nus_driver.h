/**
 * @file ble_nus_driver.h
 * @brief BLE Nordic UART Service (NUS) Driver API
 *
 * Provides wireless communication between CAN Creeper and the companion app.
 * Uses Nordic UART Service with BLE 5.2 Data Length Extension for up to 244-byte packets.
 *
 * SoftDevice: S140 v7.3.0
 * PHY: 2M preferred
 * MTU: 247 bytes
 */

#ifndef BLE_NUS_DRIVER_H
#define BLE_NUS_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * CALLBACK TYPES
 *===========================================================================*/

/** Callback for received BLE data (app → device) */
typedef void (*ble_nus_rx_callback_t)(const uint8_t *data, uint16_t len);

/** Callback for completed BLE transmission (device → app) */
typedef void (*ble_nus_tx_complete_callback_t)(void);

/** Callback for BLE connection state changes */
typedef void (*ble_nus_conn_callback_t)(bool connected);

/*===========================================================================
 * CONFIGURATION
 *===========================================================================*/

typedef struct {
    ble_nus_rx_callback_t          rx_callback;
    ble_nus_tx_complete_callback_t tx_complete_callback;
    ble_nus_conn_callback_t        conn_callback;
    uint16_t                       max_tx_size;
    uint16_t                       max_rx_size;
} ble_nus_config_t;

/*===========================================================================
 * ERROR CODES
 *===========================================================================*/

#define BLE_ERR_OK                0
#define BLE_ERR_NOT_INIT         -1
#define BLE_ERR_SD               -2
#define BLE_ERR_PARAM            -3
#define BLE_ERR_NOT_CONNECTED    -4
#define BLE_ERR_TX_BUSY          -5
#define BLE_ERR_TX_FULL          -6

/*===========================================================================
 * API
 *===========================================================================*/

/**
 * @brief Initialize BLE NUS service
 *
 * Registers with SoftDevice, creates GATT service, and prepares advertising.
 *
 * @param config Pointer to configuration
 * @return BLE_ERR_OK on success
 */
int ble_nus_init(const ble_nus_config_t *config);

/**
 * @brief Deinitialize BLE NUS
 *
 * @return BLE_ERR_OK on success
 */
int ble_nus_deinit(void);

/**
 * @brief Start BLE advertising
 *
 * Begins advertising with device name "CAN Creeper" and NUS service UUID.
 *
 * @return BLE_ERR_OK on success
 */
int ble_nus_start_advertising(void);

/**
 * @brief Stop BLE advertising
 *
 * @return BLE_ERR_OK on success
 */
int ble_nus_stop_advertising(void);

/**
 * @brief Disconnect current BLE connection
 *
 * @return BLE_ERR_OK on success
 */
int ble_nus_disconnect(void);

/**
 * @brief Transmit data over BLE NUS (device → app)
 *
 * Queues data for notification on the TX characteristic.
 *
 * @param data Data to transmit
 * @param len  Data length (max 244 bytes)
 * @return BLE_ERR_OK on success
 */
int ble_nus_tx(const uint8_t *data, uint16_t len);

/**
 * @brief Get number of pending TX packets
 *
 * @return Number of queued TX packets
 */
int ble_nus_tx_pending(void);

/**
 * @brief Check if BLE is connected
 *
 * @return true if connected
 */
bool ble_nus_is_connected(void);

/**
 * @brief Get current BLE MTU size
 *
 * @return MTU size in bytes
 */
int ble_nus_get_mtu(void);

/**
 * @brief Set BLE TX power
 *
 * @param dbm TX power in dBm (-40, -20, -16, -12, -8, -4, 0, +3, +4, +8)
 * @return BLE_ERR_OK on success
 */
int ble_nus_set_tx_power(int8_t dbm);

/**
 * @brief Handle SoftDevice BLE events
 *
 * Called from the SoftDevice event dispatcher.
 *
 * @param event BLE event from SoftDevice
 */
void ble_nus_event_handler(const void *event);

#ifdef __cplusplus
}
#endif

#endif /* BLE_NUS_DRIVER_H */
