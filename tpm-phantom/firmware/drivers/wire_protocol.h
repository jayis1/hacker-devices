/*
 * tpm-phantom — drivers/wire_protocol.h
 * Binary wire protocol between firmware and companion app.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TPM_PHANTOM_WIRE_PROTOCOL_H
#define TPM_PHANTOM_WIRE_PROTOCOL_H

#include <stdint.h>
#include "lpc_driver.h"
#include "spi_tpm_driver.h"
#include "sd_capture.h"
#include "usb_cdc.h"

#define WIRE_SOF0           0xA5
#define WIRE_SOF1           0x5A

/* Commands (host → device) */
#define WIRE_CMD_GET_STATUS   0x01
#define WIRE_CMD_START_CAP    0x02
#define WIRE_CMD_STOP_CAP     0x03
#define WIRE_CMD_GET_STATS    0x04
#define WIRE_CMD_SET_FILTER   0x05
#define WIRE_CMD_INJECT_LPC   0x06
#define WIRE_CMD_INJECT_SPI   0x07
#define WIRE_CMD_SET_LED      0x08
#define WIRE_CMD_RESET        0x09
#define WIRE_CMD_SD_STATUS    0x0A
#define WIRE_CMD_FLUSH_SD     0x0B
#define WIRE_CMD_GET_VERSION  0x0C

/* Responses (device → host) */
#define WIRE_RSP_STATUS        0x81
#define WIRE_RSP_TRANSACTION  0x82
#define WIRE_RSP_STATS         0x83
#define WIRE_RSP_SD_INFO       0x84
#define WIRE_RSP_VERSION       0x85
#define WIRE_RSP_ERROR         0x86

typedef struct {
    uint32_t lpc_total;
    uint32_t lpc_tpm;
    uint32_t spi_total;
    uint32_t spi_reads;
    uint32_t spi_writes;
    uint32_t spi_bytes;
    uint32_t sd_blocks;
    uint32_t sd_bytes;
    uint32_t lpc_serirq;
} wire_stats_t;

typedef void (*wire_cmd_handler_t)(uint8_t cmd, uint8_t *payload, uint16_t len);

uint16_t wire_crc16(const uint8_t *data, uint16_t len);
uint16_t wire_crc16_update(uint16_t crc, const uint8_t *data, uint16_t len);

void wire_protocol_init(wire_cmd_handler_t h);
void wire_protocol_feed(uint8_t b);

uint16_t wire_pack(uint8_t *buf, uint8_t cmd, const uint8_t *payload, uint16_t len);
uint16_t wire_pack_lpc_tx(uint8_t *buf, const lpc_transaction_t *t);
uint16_t wire_pack_spi_tx(uint8_t *buf, const spi_tpm_transaction_t *t);
uint16_t wire_pack_status(uint8_t *buf, uint8_t mode, uint8_t active,
                          uint32_t total_tx, uint32_t tpm_tx);
uint16_t wire_pack_stats(uint8_t *buf, const wire_stats_t *s);
uint16_t wire_pack_version(uint8_t *buf);

#endif /* TPM_PHANTOM_WIRE_PROTOCOL_H */