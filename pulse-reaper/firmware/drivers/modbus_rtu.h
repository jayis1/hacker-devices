/*
 * modbus_rtu.h — Modbus RTU parser/injector
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_MODBUS_RTU_H
#define PULSEREAPER_MODBUS_RTU_H

#include "protocol_detect.h"

extern const pr_protocol_t pr_proto_modbus_rtu;

void modbus_rtu_init(void);
void modbus_inject_test_frame(void);

/* CRC-16 (Modbus polynomial 0xA001) */
uint16_t modbus_crc16(const uint8_t *data, uint16_t len);

#endif