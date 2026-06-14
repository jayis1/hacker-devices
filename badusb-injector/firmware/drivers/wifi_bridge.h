/*
 * PHANTOM — WiFi Bridge Driver Header
 * ESP32-C3-MINI-1 AT Command Interface via UART
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 */

#ifndef WIFI_BRIDGE_H
#define WIFI_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* WiFi operation modes */
typedef enum {
    WIFI_MODE_STA = 1,       /* Station mode */
    WIFI_MODE_AP = 2,        /* SoftAP mode */
    WIFI_MODE_STA_AP = 3,    /* Station + SoftAP */
} wifi_mode_t;

/* WiFi scan result */
typedef struct {
    char ssid[33];            /* Network SSID */
    int8_t rssi;              /* Signal strength */
    uint8_t channel;          /* Channel number */
    uint8_t encryption;       /* 0=open, 1=WEP, 2=WPA-PSK, 3=WPA2-PSK, 4=WPA/WPA2 */
    uint8_t bssid[6];         /* Access point MAC address */
} wifi_scan_result_t;

/* WiFi status */
typedef struct {
    bool connected;           /* Connected to AP */
    bool ble_connected;       /* BLE client connected */
    char ip_addr[16];         /* IP address */
    char ssid[33];            /* Connected SSID */
    int8_t rssi;              /* Signal strength */
} wifi_status_t;

/* AT command response buffer */
#define AT_RESPONSE_BUF_SIZE    1024
#define AT_TIMEOUT_MS            5000

/* Function prototypes */
int wifi_bridge_init(void);
void wifi_bridge_task(void);
int wifi_send_at_cmd(const char *cmd, char *response, uint32_t response_size, uint32_t timeout_ms);
int wifi_set_mode(wifi_mode_t mode);
int wifi_connect(const char *ssid, const char *password);
int wifi_disconnect(void);
int wifi_scan(wifi_scan_result_t *results, uint8_t max_results);
bool wifi_scan_for_ssid(const char *target_ssid);
int wifi_start_ap(const char *ssid, const char *password, uint8_t channel);
int wifi_start_http_server(uint16_t port);
int wifi_send_http_response(int conn_id, const char *data, uint32_t len);
int wifi_close_connection(int conn_id);
wifi_status_t wifi_get_status(void);
int wifi_reset(void);

/* BLE functions */
int ble_start_advertising(const char *device_name);
int ble_stop_advertising(void);
int ble_send_notification(uint16_t handle, const uint8_t *data, uint16_t len);
int ble_receive_data(uint8_t *data, uint16_t *len);

#endif /* WIFI_BRIDGE_H */