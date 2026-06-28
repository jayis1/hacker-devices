/*
 * comm.h - Communication protocol header (USB CDC + BLE UART)
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef COMM_H
#define COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

typedef void (*command_handler_t)(uint8_t cmd, const uint8_t *data, uint16_t len);

void comm_init(void);
void comm_process(void);
void comm_send_response(uint8_t resp_type, const uint8_t *data, uint16_t len);
void comm_send_status(void);
void comm_send_log(const char *msg);
void comm_register_handler(command_handler_t handler);
uint32_t comm_get_rx_count(void);
uint32_t comm_get_tx_count(void);
bool comm_is_usb_connected(void);
bool comm_is_ble_connected(void);

#endif /* COMM_H */