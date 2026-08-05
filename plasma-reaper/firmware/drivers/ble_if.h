/*
 * ble_if.h — BLE interface driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef BLE_IF_H
#define BLE_IF_H

#include <stdint.h>
#include <stdbool.h>

void ble_if_init(void);
void ble_if_poll(void);
uint32_t ble_if_rx(uint8_t *buf, uint32_t max_len);
void ble_if_tx(const uint8_t *buf, uint32_t len);
bool ble_if_connected(void);
void ble_if_advertise(bool on);

#endif /* BLE_IF_H */