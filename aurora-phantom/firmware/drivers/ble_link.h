/*
 * ble_link.h — BLE 5.0 rendezvous protocol
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_BLE_LINK_H
#define AURORA_BLE_LINK_H
#include <stdint.h>
#include <stdbool.h>

int  ble_link_start(void);
void ble_link_stop(void);
bool ble_link_is_connected(void);

/* Push frame data to the companion app. */
void ble_link_send_header(uint16_t total_frames);
void ble_link_send_frame(const uint16_t *words, uint32_t n);
void ble_link_send_trailer(void);

/* Command parser (app -> device). */
typedef void (*ble_cmd_cb_t)(const uint8_t *data, uint32_t len);
void ble_link_set_cmd_callback(ble_cmd_cb_t cb);

#endif