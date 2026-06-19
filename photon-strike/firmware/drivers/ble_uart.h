/**
 * drivers/ble_uart.h — framed binary protocol over UART3 to ESP32-C3
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_BLE_UART_H
#define PHOTONSTRIKE_BLE_UART_H

#include <stdint.h>
#include "board.h"

void      ble_init(void);
void      ble_send_hello(uint32_t version);
void      ble_send_ack(uint8_t op, uint32_t arg);
void      ble_send_nack(uint8_t op, uint32_t arg);
void      ble_send_status(uint8_t op, uint32_t a, uint32_t b, uint32_t c);
void      ble_send_shot(const ps_shot_t *shot);
void      ble_send_dfa(uint8_t faults, uint8_t unique, const uint8_t key[16]);
uint16_t  ble_poll(uint8_t *op, uint8_t *buf, uint16_t maxlen);
bool      ble_take_output(uint8_t *buf, uint16_t *len, uint16_t maxlen, uint32_t timeout_ms);

#endif