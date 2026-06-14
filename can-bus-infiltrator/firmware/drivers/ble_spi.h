/*
 * ble_spi.h — SPI interface to nRF52840 BLE co-processor
 */

#ifndef BLE_SPI_DRIVER_H
#define BLE_SPI_DRIVER_H

#include <stdint.h>
#include "can.h"

#define BLE_SPI_BASE    0x40003C00UL
#define BLE_PKT_MAX    256

/* Command opcodes (STM32 → nRF) */
#define BLE_CMD_CONNECT_SCAN    0x01
#define BLE_CMD_DISCONNECT      0x02
#define BLE_CMD_SEND_DATA       0x03
#define BLE_CMD_GET_STATUS      0x04
#define BLE_CMD_SET_ADV_DATA    0x05
#define BLE_CMD_START_ADV       0x06
#define BLE_CMD_STOP_ADV        0x07

/* Response codes (nRF → STM32) */
#define BLE_RSP_ACK            0x00
#define BLE_RSP_NACK           0x01
#define BLE_RSP_DATA           0x02
#define BLE_RSP_CONNECTED      0x03
#define BLE_RSP_DISCONNECTED   0x04

typedef struct {
    uint8_t  opcode;
    uint8_t  seq;
    uint16_t length;
    uint8_t  data[BLE_PKT_MAX];
} __attribute__((packed)) ble_packet_t;

int  ble_spi_init(void);
int  ble_spi_send_command(uint8_t opcode, const uint8_t *data, uint16_t len);
int  ble_spi_read_response(ble_packet_t *pkt, uint32_t timeout_ms);
void ble_spi_register_callback(void (*cb)(const ble_packet_t *));
int  ble_spi_start_advertising(const char *name);
int  ble_spi_connect(const uint8_t *addr);
int  ble_send_can_frame(const can_frame_t *frame);

#endif /* BLE_SPI_DRIVER_H */