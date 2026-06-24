/*
 * lora-phantom / drivers/sdcard.c
 * microSD card driver: FAT32 logging, PCAP-LoRa capture writer.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The SD card stores packet captures in a PCAP-compatible format with a
 * custom LoRa link-layer pseudo-header. This allows offline analysis in
 * Wireshark with the LoRaWAN dissector after loading a DLT plugin.
 *
 * PCAP-LoRa format:
 *   Global header (24 bytes): magic=0xA1B2C3D4, version=2.4, thiszone=0,
 *     sigfigs=0, snaplen=65535, network=DLT_USER0 (147)
 *   Per-packet record header (16 bytes): ts_sec, ts_usec, incl_len, orig_len
 *   Per-packet pseudo-header (20 bytes): freq_hz, sf, bw, rssi, snr, reserved
 *   Per-packet payload: raw LoRa PHY payload bytes
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* SDMMC command indices */
#define SD_CMD0    0
#define SD_CMD8    8
#define SD_CMD55   55
#define SD_ACMD41  41
#define SD_CMD2    2
#define SD_CMD3    3
#define SD_CMD7    7
#define SD_CMD16   16
#define SD_CMD17   17
#define SD_CMD24   24
#define SD_CMD18   18
#define SD_CMD25   25

#define SD_R1_IDLE       0x01
#define SD_R1_ILLEGAL    0x04

static int s_sd_mounted = 0;
static int s_capture_open = 0;
static uint32_t s_capture_sector = 0;  /* current write sector */
static uint32_t s_capture_offset = 0;  /* offset within sector */
static uint8_t s_sector_buf[512];
static uint32_t s_total_sectors = 0;
static uint32_t s_free_sectors = 0;

/* ---- SDMMC low-level (simplified) ---- */
static int sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t resp_type, uint32_t *resp) {
    SDMMC1->ARG = arg;
    SDMMC1->CMD = (cmd & 0x3F) | SDMMC_CMD_CPSMEN | resp_type;
    /* Wait for command complete */
    uint32_t timeout = 1000000;
    while (!(SDMMC1->STA & (1U << 7)) && timeout--) { }  /* CMDSENT or CCRCFAIL */
    if (resp_type && resp) {
        *resp = SDMMC1->RESP1;
    }
    /* Clear status */
    SDMMC1->ICR = 0xFFFFFFFF;
    return (timeout > 0) ? 0 : -1;
}

static int sd_wait_data(uint32_t timeout) {
    while (!(SDMMC1->STA & (1U << 8)) && timeout--) { }  /* DBCKEND or RXOVERR */
    SDMMC1->ICR = 0xFFFFFFFF;
    return (timeout > 0) ? 0 : -1;
}

/* ---- Initialize SD card (simplified SPI/SD mode) ---- */
void sdcard_init(void) {
    /* Configure SDMMC1: power on, clock ~400 kHz for init */
    SDMMC1->POWER = SDMMC_POWER_PWRCTRL;
    SDMMC1->CLKCR = (0x76 << 0) | (1U << 11);  /* ~400 kHz, power-saving */

    /* Send CMD0 (reset) */
    uint32_t resp;
    sd_send_cmd(SD_CMD0, 0, SDMMC_CMD_WAITRESP_SHORT, &resp);
    /* CMD8 (check voltage) */
    sd_send_cmd(SD_CMD8, 0x1AA, SDMMC_CMD_WAITRESP_SHORT, &resp);
    /* ACMD41 (initialize) — CMD55 + ACMD41 */
    uint32_t timeout = 1000000;
    do {
        sd_send_cmd(SD_CMD55, 0, SDMMC_CMD_WAITRESP_SHORT, &resp);
        sd_send_cmd(SD_ACMD41, 0x40100000, SDMMC_CMD_WAITRESP_SHORT, &resp);
    } while ((resp & 0x80000000) == 0 && timeout--);

    if (timeout == 0) {
        s_sd_mounted = 0;
        return;
    }

    /* CMD2 (get CID) + CMD3 (get RCA) */
    sd_send_cmd(SD_CMD2, 0, SDMMC_CMD_WAITRESP_LONG, &resp);
    sd_send_cmd(SD_CMD3, 0, SDMMC_CMD_WAITRESP_SHORT, &resp);
    uint32_t rca = resp >> 16;

    /* CMD7 (select card) */
    sd_send_cmd(SD_CMD7, rca << 16, SDMMC_CMD_WAITRESP_SHORT, &resp);

    /* CMD16 (set block size = 512) */
    sd_send_cmd(SD_CMD16, 512, SDMMC_CMD_WAITRESP_SHORT, &resp);

    /* Switch to high clock (25 MHz) */
    SDMMC1->CLKCR = (1 << 0) | (1U << 11);  /* /2 = 60 MHz... simplified */

    /* Read CSD to get total sectors (CMD9) */
    sd_send_cmd(9, rca << 16, SDMMC_CMD_WAITRESP_LONG, &resp);
    /* Simplified: assume 32 GB card = ~62 M sectors */
    s_total_sectors = 62000000;
    s_free_sectors = s_total_sectors;

    s_sd_mounted = 1;
}

int sdcard_mount(void) {
    if (s_sd_mounted) return 0;
    sdcard_init();
    return s_sd_mounted ? 0 : -1;
}

/* ---- Write a single sector ---- */
static int sd_write_sector(uint32_t sector, const uint8_t *data) {
    if (!s_sd_mounted) return -1;
    sd_send_cmd(SD_CMD24, sector * 512, SDMMC_CMD_WAITRESP_SHORT, 0);
    SDMMC1->DLEN = 512;
    SDMMC1->DCTRL = SDMMC_DCTRL_DTEN | SDMMC_DCTRL_DBLOCKSIZE(9);
    /* Write 128 32-bit words to FIFO */
    for (int i = 0; i < 128; i++) {
        while (!(SDMMC1->STA & (1U << 1))) { }  /* TXFIFOHE */
        SDMMC1->BUFF = ((uint32_t)data[i*4]) | ((uint32_t)data[i*4+1] << 8) |
                       ((uint32_t)data[i*4+2] << 16) | ((uint32_t)data[i*4+3] << 24);
    }
    return sd_wait_data(1000000);
}

/* ---- Read a single sector ---- */
static int sd_read_sector(uint32_t sector, uint8_t *data) {
    if (!s_sd_mounted) return -1;
    sd_send_cmd(SD_CMD17, sector * 512, SDMMC_CMD_WAITRESP_SHORT, 0);
    SDMMC1->DLEN = 512;
    SDMMC1->DCTRL = SDMMC_DCTRL_DTEN | SDMMC_DCTRL_DBLOCKSIZE(9);
    for (int i = 0; i < 128; i++) {
        while (!(SDMMC1->STA & (1U << 2))) { }  /* RXFIFOHF */
        uint32_t w = SDMMC1->BUFF;
        data[i*4]   = (uint8_t)(w);
        data[i*4+1] = (uint8_t)(w >> 8);
        data[i*4+2] = (uint8_t)(w >> 16);
        data[i*4+3] = (uint8_t)(w >> 24);
    }
    return sd_wait_data(1000000);
}

/* ---- PCAP-LoRa global header ---- */
static const uint8_t pcap_global_header[24] = {
    0xD4, 0xC3, 0xB2, 0xA1,  /* magic (little-endian) */
    0x02, 0x00, 0x04, 0x00,  /* version 2.4 */
    0x00, 0x00, 0x00, 0x00,  /* thiszone */
    0x00, 0x00, 0x00, 0x00,  /* sigfigs */
    0xFF, 0xFF, 0x00, 0x00,  /* snaplen = 65535 */
    0x93, 0x00, 0x00, 0x00   /* network = 147 (DLT_USER0) */
};

int sdcard_open_capture(void) {
    if (!s_sd_mounted) return -1;
    /* Find a free sector to start the capture (simplified: start at sector 1000) */
    s_capture_sector = 1000;
    s_capture_offset = 0;
    /* Write PCAP global header */
    memset(s_sector_buf, 0, sizeof(s_sector_buf));
    memcpy(s_sector_buf, pcap_global_header, sizeof(pcap_global_header));
    s_capture_offset = sizeof(pcap_global_header);
    s_capture_open = 1;
    return 0;
}

/* ---- Write a PCAP-LoRa packet record ---- */
int sdcard_write_packet(uint32_t ts_ms, uint32_t freq_hz, uint8_t sf, uint8_t bw,
                        int16_t rssi, int8_t snr, const uint8_t *payload, uint16_t len) {
    if (!s_capture_open) return -1;
    if (len > PCAP_MAX_PAYLOAD) len = PCAP_MAX_PAYLOAD;

    /* Record header: ts_sec(4) + ts_usec(4) + incl_len(4) + orig_len(4) = 16 bytes */
    /* Pseudo-header: freq(4) + sf(1) + bw(1) + rssi(2) + snr(1) + reserved(3) = 12 bytes */
    uint16_t total = 16 + 12 + len;

    /* Check if we need to flush the current sector */
    if (s_capture_offset + total > 512) {
        sd_write_sector(s_capture_sector, s_sector_buf);
        s_capture_sector++;
        s_capture_offset = 0;
        memset(s_sector_buf, 0, 512);
    }

    uint16_t p = s_capture_offset;
    /* ts_sec */
    s_sector_buf[p++] = (uint8_t)(ts_ms / 1000);
    s_sector_buf[p++] = (uint8_t)((ts_ms / 1000) >> 8);
    s_sector_buf[p++] = (uint8_t)((ts_ms / 1000) >> 16);
    s_sector_buf[p++] = (uint8_t)((ts_ms / 1000) >> 24);
    /* ts_usec */
    uint32_t us = (ts_ms % 1000) * 1000;
    s_sector_buf[p++] = (uint8_t)us;
    s_sector_buf[p++] = (uint8_t)(us >> 8);
    s_sector_buf[p++] = (uint8_t)(us >> 16);
    s_sector_buf[p++] = (uint8_t)(us >> 24);
    /* incl_len */
    s_sector_buf[p++] = (uint8_t)(12 + len);
    s_sector_buf[p++] = (uint8_t)((12 + len) >> 8);
    s_sector_buf[p++] = 0; s_sector_buf[p++] = 0;
    /* orig_len */
    s_sector_buf[p++] = (uint8_t)(12 + len);
    s_sector_buf[p++] = (uint8_t)((12 + len) >> 8);
    s_sector_buf[p++] = 0; s_sector_buf[p++] = 0;
    /* Pseudo-header */
    s_sector_buf[p++] = (uint8_t)(freq_hz);
    s_sector_buf[p++] = (uint8_t)(freq_hz >> 8);
    s_sector_buf[p++] = (uint8_t)(freq_hz >> 16);
    s_sector_buf[p++] = (uint8_t)(freq_hz >> 24);
    s_sector_buf[p++] = sf;
    s_sector_buf[p++] = bw;
    s_sector_buf[p++] = (uint8_t)(rssi);
    s_sector_buf[p++] = (uint8_t)(rssi >> 8);
    s_sector_buf[p++] = (uint8_t)snr;
    s_sector_buf[p++] = 0; s_sector_buf[p++] = 0; s_sector_buf[p++] = 0;
    /* Payload */
    memcpy(s_sector_buf + p, payload, len);
    p += len;
    s_capture_offset = p;

    return 0;
}

void sdcard_close(void) {
    if (s_capture_open && s_capture_offset > 0) {
        sd_write_sector(s_capture_sector, s_sector_buf);
    }
    s_capture_open = 0;
}

uint32_t sdcard_free_kb(void) {
    if (!s_sd_mounted) return 0;
    /* Simplified: total sectors minus used */
    s_free_sectors = s_total_sectors - s_capture_sector;
    return s_free_sectors / 2;  /* sectors to KB (512 bytes each) */
}