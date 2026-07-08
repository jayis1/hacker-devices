/*
 * tpm-phantom — drivers/wire_protocol.c
 * Binary wire protocol between tpm-phantom firmware and companion app.
 *
 * Protocol format (little-endian, over USB CDC and BLE):
 *   [SOF0=0xA5][SOF1=0x5A][LEN u16][CMD u8][PAYLOAD...][CRC16 u16]
 *
 * Commands (host → device):
 *   0x01 GET_STATUS     — returns device status struct
 *   0x02 START_CAPTURE  — payload: {mode u8, flags u8}
 *   0x03 STOP_CAPTURE   — no payload
 *   0x04 GET_STATS      — returns capture statistics
 *   0x05 SET_FILTER     — payload: {addr_mask u32, addr_val u32}
 *   0x06 INJECT_LPC     — payload: {addr u16, data u8}
 *   0x07 INJECT_SPI     — payload: {len u8, data[len]}
 *   0x08 SET_LED        — payload: {led u8, state u8}
 *   0x09 RESET          — soft reset
 *   0x0A SD_STATUS      — returns SD card info
 *   0x0B FLUSH_SD       — flush SD buffer
 *   0x0C GET_VERSION    — returns firmware version string
 *
 * Responses (device → host):
 *   0x81 STATUS          — periodic status
 *   0x82 TRANSACTION     — captured TPM transaction (LPC or SPI)
 *   0x83 STATS           — capture statistics
 *   0x84 SD_INFO         — SD card info
 *   0x85 VERSION         — version string
 *   0x86 ERROR           — error code
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "wire_protocol.h"
#include "../board.h"
#include <string.h>

/* CRC-16/CCITT (poly 0x1021, init 0xFFFF) */
uint16_t wire_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* Pack a frame into buf; returns total length */
uint16_t wire_pack(uint8_t *buf, uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    buf[0] = WIRE_SOF0;
    buf[1] = WIRE_SOF1;
    buf[2] = (uint8_t)(len & 0xFF);
    buf[3] = (uint8_t)(len >> 8);
    buf[4] = cmd;
    if (len)
        memcpy(&buf[5], payload, len);
    uint16_t crc = wire_crc16(&buf[2], 3 + len);
    buf[5 + len] = (uint8_t)(crc & 0xFF);
    buf[6 + len] = (uint8_t)(crc >> 8);
    return 7 + len;
}

/* Parser state */
static volatile uint8_t  g_wp_state = 0;
static volatile uint16_t g_wp_len = 0;
static volatile uint8_t  g_wp_cmd = 0;
static volatile uint16_t g_wp_idx = 0;
static volatile uint16_t g_wp_crc_lo = 0;
static uint8_t  g_wp_payload[256];
static wire_cmd_handler_t g_wp_handler = 0;

void wire_protocol_init(wire_cmd_handler_t h)
{
    g_wp_state = 0;
    g_wp_handler = h;
}

/* Feed a byte into the parser; calls handler when a full frame is received */
void wire_protocol_feed(uint8_t b)
{
    switch (g_wp_state) {
    case 0:
        if (b == WIRE_SOF0) g_wp_state = 1;
        break;
    case 1:
        if (b == WIRE_SOF1) g_wp_state = 2;
        else g_wp_state = 0;
        break;
    case 2:
        g_wp_len = b;
        g_wp_state = 3;
        break;
    case 3:
        g_wp_len |= (uint16_t)b << 8;
        g_wp_state = 4;
        break;
    case 4:
        g_wp_cmd = b;
        g_wp_idx = 0;
        g_wp_state = (g_wp_len > 0) ? 5 : 6;
        break;
    case 5:
        if (g_wp_idx < sizeof(g_wp_payload))
            g_wp_payload[g_wp_idx++] = b;
        if (g_wp_idx >= g_wp_len)
            g_wp_state = 6;
        break;
    case 6:
        g_wp_crc_lo = b;
        g_wp_state = 7;
        break;
    case 7: {
        uint16_t rcvd = g_wp_crc_lo | ((uint16_t)b << 8);
        uint8_t hdr[3];
        hdr[0] = (uint8_t)(g_wp_len & 0xFF);
        hdr[1] = (uint8_t)(g_wp_len >> 8);
        hdr[2] = g_wp_cmd;
        uint16_t calc = wire_crc16(hdr, 3);
        calc = wire_crc16_update(calc, g_wp_payload, g_wp_len);
        if (rcvd == calc && g_wp_handler)
            g_wp_handler(g_wp_cmd, g_wp_payload, g_wp_len);
        g_wp_state = 0;
        break;
    }
    default:
        g_wp_state = 0;
        break;
    }
}

/* CRC helper for multi-segment */
uint16_t wire_crc16_update(uint16_t crc, const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* Serialize a captured LPC transaction as a TRANSACTION frame */
uint16_t wire_pack_lpc_tx(uint8_t *buf, const lpc_transaction_t *t)
{
    uint8_t payload[16];
    payload[0] = 0;  /* bus = LPC */
    payload[1] = t->direction;
    payload[2] = t->cycle_type;
    payload[3] = t->is_tpm;
    payload[4] = (uint8_t)(t->address & 0xFF);
    payload[5] = (uint8_t)(t->address >> 8);
    payload[6] = (uint8_t)(t->address >> 16);
    payload[7] = (uint8_t)(t->address >> 24);
    payload[8] = (uint8_t)(t->data & 0xFF);
    payload[9] = (uint8_t)(t->timestamp & 0xFF);
    payload[10] = (uint8_t)(t->timestamp >> 8);
    payload[11] = (uint8_t)(t->timestamp >> 16);
    payload[12] = (uint8_t)(t->timestamp >> 24);
    return wire_pack(buf, WIRE_RSP_TRANSACTION, payload, 13);
}

/* Serialize a captured SPI TPM transaction */
uint16_t wire_pack_spi_tx(uint8_t *buf, const spi_tpm_transaction_t *t)
{
    uint8_t payload[80];
    payload[0] = 1;  /* bus = SPI */
    payload[1] = t->command;
    payload[2] = t->is_tpm;
    payload[3] = t->data_len;
    payload[4] = (uint8_t)(t->address & 0xFF);
    payload[5] = (uint8_t)(t->address >> 8);
    payload[6] = (uint8_t)(t->address >> 16);
    payload[7] = (uint8_t)(t->timestamp & 0xFF);
    payload[8] = (uint8_t)(t->timestamp >> 8);
    payload[9] = (uint8_t)(t->timestamp >> 16);
    payload[10] = (uint8_t)(t->timestamp >> 24);
    uint8_t dlen = t->data_len > 64 ? 64 : t->data_len;
    memcpy(&payload[11], t->data, dlen);
    return wire_pack(buf, WIRE_RSP_TRANSACTION, payload, 11 + dlen);
}

/* Serialize a status response */
uint16_t wire_pack_status(uint8_t *buf, uint8_t mode, uint8_t active,
                          uint32_t total_tx, uint32_t tpm_tx)
{
    uint8_t payload[12];
    payload[0] = mode;
    payload[1] = active;
    payload[2] = (uint8_t)(total_tx & 0xFF);
    payload[3] = (uint8_t)(total_tx >> 8);
    payload[4] = (uint8_t)(total_tx >> 16);
    payload[5] = (uint8_t)(total_tx >> 24);
    payload[6] = (uint8_t)(tpm_tx & 0xFF);
    payload[7] = (uint8_t)(tpm_tx >> 8);
    payload[8] = (uint8_t)(tpm_tx >> 16);
    payload[9] = (uint8_t)(tpm_tx >> 24);
    payload[10] = sd_capture_ready();
    payload[11] = usb_cdc_configured();
    return wire_pack(buf, WIRE_RSP_STATUS, payload, 12);
}

/* Serialize statistics */
uint16_t wire_pack_stats(uint8_t *buf, const wire_stats_t *s)
{
    uint8_t payload[40];
    int p = 0;
    payload[p++] = (uint8_t)(s->lpc_total & 0xFF);
    payload[p++] = (uint8_t)(s->lpc_total >> 8);
    payload[p++] = (uint8_t)(s->lpc_total >> 16);
    payload[p++] = (uint8_t)(s->lpc_total >> 24);
    payload[p++] = (uint8_t)(s->lpc_tpm & 0xFF);
    payload[p++] = (uint8_t)(s->lpc_tpm >> 8);
    payload[p++] = (uint8_t)(s->lpc_tpm >> 16);
    payload[p++] = (uint8_t)(s->lpc_tpm >> 24);
    payload[p++] = (uint8_t)(s->spi_total & 0xFF);
    payload[p++] = (uint8_t)(s->spi_total >> 8);
    payload[p++] = (uint8_t)(s->spi_total >> 16);
    payload[p++] = (uint8_t)(s->spi_total >> 24);
    payload[p++] = (uint8_t)(s->spi_reads & 0xFF);
    payload[p++] = (uint8_t)(s->spi_reads >> 8);
    payload[p++] = (uint8_t)(s->spi_writes & 0xFF);
    payload[p++] = (uint8_t)(s->spi_writes >> 8);
    payload[p++] = (uint8_t)(s->spi_bytes & 0xFF);
    payload[p++] = (uint8_t)(s->spi_bytes >> 8);
    payload[p++] = (uint8_t)(s->spi_bytes >> 16);
    payload[p++] = (uint8_t)(s->spi_bytes >> 24);
    payload[p++] = (uint8_t)(s->sd_blocks & 0xFF);
    payload[p++] = (uint8_t)(s->sd_blocks >> 8);
    payload[p++] = (uint8_t)(s->sd_blocks >> 16);
    payload[p++] = (uint8_t)(s->sd_blocks >> 24);
    payload[p++] = (uint8_t)(s->sd_bytes & 0xFF);
    payload[p++] = (uint8_t)(s->sd_bytes >> 8);
    payload[p++] = (uint8_t)(s->sd_bytes >> 16);
    payload[p++] = (uint8_t)(s->sd_bytes >> 24);
    payload[p++] = (uint8_t)(s->lpc_serirq & 0xFF);
    payload[p++] = (uint8_t)(s->lpc_serirq >> 8);
    payload[p++] = (uint8_t)(s->lpc_serirq >> 16);
    payload[p++] = (uint8_t)(s->lpc_serirq >> 24);
    return wire_pack(buf, WIRE_RSP_STATS, payload, p);
}

/* Serialize version info */
uint16_t wire_pack_version(uint8_t *buf)
{
    const char *ver = "tpm-phantom v1.0 (jayis1) " __DATE__;
    uint8_t len = (uint8_t)strlen(ver);
    uint8_t payload[64];
    payload[0] = len;
    memcpy(&payload[1], ver, len);
    return wire_pack(buf, WIRE_RSP_VERSION, payload, 1 + len);
}