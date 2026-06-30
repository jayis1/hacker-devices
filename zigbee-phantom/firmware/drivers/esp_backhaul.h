/*
 * drivers/esp_backhaul.h — ESP32-S3 backhaul interface (SPI slave)
 * Author: jayis1
 * License: GPL-2.0
 *
 * The CC2652 streams captured frames to the ESP32-S3 over an SPI slave link.
 * The ESP32 then forwards them over USB-CDC (KillerBee-compatible) or BLE to
 * the mobile app. This driver handles the CC2652-side SPI slave protocol.
 */
#ifndef ZIGBEE_PHANTOM_ESP_BACKHAUL_H
#define ZIGBEE_PHANTOM_ESP_BACKHAUL_H

#include <stdint.h>
#include <stdbool.h>

int  esp_backhaul_init(void);
bool esp_link_up(void);

/* Send a captured frame to the ESP32 for forwarding to host/app. */
int  esp_send_frame(uint8_t channel, int8_t rssi, uint8_t lqi,
                    uint32_t ts_us, const uint8_t *frame, uint8_t len);

/* Send a status/event message (e.g., KEY captured, mode change). */
int  esp_send_event(uint8_t event_id, const uint8_t *payload, uint8_t len);

/* Receive a command from the ESP32 (non-blocking; returns -1 if none). */
int  esp_recv_cmd(uint8_t *cmd_out, uint8_t *payload, uint8_t *len_out, uint8_t maxlen);

/* Event IDs */
#define ESP_EVT_KEY_CAPTURED   0x01
#define ESP_EVT_MODE_CHANGE     0x02
#define ESP_EVT_CHANNEL_CHANGE 0x03
#define ESP_EVT_JAM_TRIGGER     0x04
#define ESP_EVT_BATT_LOW        0x05
#define ESP_EVT_ERROR           0xFF

/* Command IDs (from host/app to CC2652) */
#define ESP_CMD_SET_MODE        0x10
#define ESP_CMD_SET_CHANNEL     0x11
#define ESP_CMD_SET_PAN_FILTER  0x12
#define ESP_CMD_SET_EUI_FILTER  0x13
#define ESP_CMD_INJECT_ZCL      0x20
#define ESP_CMD_INJECT_BEACON    0x21
#define ESP_CMD_INJECT_REJOIN    0x22
#define ESP_CMD_BRUTE_KEY       0x30
#define ESP_CMD_ENERGY_SCAN     0x40
#define ESP_CMD_GET_STATUS      0x50

#endif