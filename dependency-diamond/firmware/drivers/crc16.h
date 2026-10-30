/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef DD_CRC16_H
#define DD_CRC16_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
uint16_t crc16_byte(uint16_t state, uint8_t value);
bool crc16_buffer(const void *data, size_t length, uint16_t *result);
bool crc16_verify(const void *data, size_t length, uint16_t expected);
#endif
