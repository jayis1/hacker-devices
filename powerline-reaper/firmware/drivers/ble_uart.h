/*
 * ble_uart.h — UART framing to nRF52840 BLE module interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef BLE_UART_H
#define BLE_UART_H

#include <stdint.h>
#include "qca7420.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ble_frame_cb_t)(uint8_t type, const uint8_t *payload, uint16_t len);

void ble_uart_init(void);
void ble_uart_set_cb(ble_frame_cb_t cb);
void ble_uart_send(uint8_t type, const uint8_t *payload, uint16_t len);
void ble_uart_irq_handler(void);
void ble_uart_tick(void);

void ble_uart_tunnel_tx(const qca7420_frame_t *f);
int  ble_uart_tunnel_set_key(const uint8_t key[32], const uint8_t nonce[12]);
int  ble_uart_tunnel_up(void);
int  ble_uart_paired(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_UART_H */