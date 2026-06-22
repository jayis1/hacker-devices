/*
 * sdcard.c — WireReaper SD card driver for capture logging
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * SDIO block-level driver for microSD card on SDMMC1.
 * Provides block read/write and a simple FAT32 file writer
 * for logging captured bus transactions offline.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* SDMMC1 register offsets */
#define SDMMC_BASE   SDMMC1_BASE
#define SDMMC_CTRL   REG32(SDMMC_BASE + 0x00)
#define SDMMC_CLKCR  REG32(SDMMC_BASE + 0x04)
#define SDMMC_ARG    REG32(SDMMC_BASE + 0x08)
#define SDMMC_CMD    REG32(SDMMC_BASE + 0x0C)
#define SDMMC_RESP1  REG32(SDMMC_BASE + 0x10)
#define SDMMC_RESP2  REG32(SDMMC_BASE + 0x14)
#define SDMMC_RESP3  REG32(SDMMC_BASE + 0x18)
#define SDMMC_RESP4  REG32(SDMMC_BASE + 0x1C)
#define SDMMC_DTIMER REG32(SDMMC_BASE + 0x24)
#define SDMMC_DLEN   REG32(SDMMC_BASE + 0x28)
#define SDMMC_DCTRL  REG32(SDMMC_BASE + 0x2C)
#define SDMMC_STA    REG32(SDMMC_BASE + 0x34)
#define SDMMC_ICR    REG32(SDMMC_BASE + 0x38)
#define SDMMC_FIFOCNT REG32(SDMMC_BASE + 0x48)
#define SDMMC_FIFO   REG32(SDMMC_BASE + 0x80)

/* SDMMC status bits */
#define SDMMC_STA_CMDREND   (1U << 6)
#define SDMMC_STA_CMDSENT   (1U << 7)
#define SDMMC_STA_DATAEND   (1U << 8)
#define SDMMC_STA_DBCKEND   (1U << 10)
#define SDMMC_STA_RXDAVL    (1U << 21)
#define SDMMC_STA_TXDAVL    (1U << 22)
#define SDMMC_STA_CTIMEOUT  (1U << 0)
#define SDMMC_STA_CCRCFAIL  (1U << 1)
#define SDMMC_STA_DCRCFAIL  (1U << 2)

/* SD card commands */
#define SD_CMD_GO_IDLE       0
#define SD_CMD_SEND_OP_COND  41   /* ACMD41 */
#define SD_CMD_APP_CMD       55
#define SD_CMD_ALL_SEND_CID  2
#define SD_CMD_SET_REL_ADDR  3
#define SD_CMD_SELECT_CARD   7
#define SD_CMD_SET_BLOCKLEN  16
#define SD_CMD_READ_SINGLE   17
#define SD_CMD_WRITE_SINGLE  24
#define SD_CMD_APP_OP_COND   41

static uint8_t sd_rca_high, sd_rca_low;
static int sd_initialized = 0;

/* ---- GPIO init for SDMMC1 ---- */
static void sd_gpio_init(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD;
    gpio_t *c = GPIO(GPIOC_BASE);
    gpio_t *d = GPIO(GPIOD_BASE);
    uint32_t af = 12; /* AF12 = SDMMC1 */

    /* PC8(D0), PC9(D1), PC10(D2), PC11(D3), PC12(CLK) */
    int cpins[] = {8, 9, 10, 11, 12};
    for (int i = 0; i < 5; i++) {
        int p = cpins[i];
        c->MODER &= ~(3U << (p * 2));
        c->MODER |= (GPIO_MODE_AF << (p * 2));
        c->OSPEEDR |= (GPIO_OSPEED_VHIGH << (p * 2));
        c->PUPDR |= (GPIO_PUPD_PULLUP << (p * 2));
        c->AFR[p / 8] &= ~(0xFU << ((p % 8) * 4));
        c->AFR[p / 8] |= (af << ((p % 8) * 4));
    }

    /* PD2(CMD) */
    d->MODER &= ~(3U << (2 * 2));
    d->MODER |= (GPIO_MODE_AF << (2 * 2));
    d->OSPEEDR |= (GPIO_OSPEED_VHIGH << (2 * 2));
    d->PUPDR |= (GPIO_PUPD_PULLUP << (2 * 2));
    d->AFR[0] &= ~(0xFU << (2 * 4));
    d->AFR[0] |= (af << (2 * 4));
}

/* ---- Send SD command ---- */
static int sd_send_cmd(uint8_t cmd, uint32_t arg, int has_resp) {
    SDMMC_ICR = 0xFFFFFFFF; /* Clear all flags */
    SDMMC_ARG = arg;
    SDMMC_CMD = cmd | (has_resp ? (1U << 6) : 0) | (1U << 10); /* CPSMEN */

    uint32_t timeout = 100000;
    if (has_resp) {
        while (!(SDMMC_STA & (SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT |
                               SDMMC_STA_CCRCFAIL)) && timeout > 0)
            timeout--;
    } else {
        while (!(SDMMC_STA & SDMMC_STA_CMDSENT) && timeout > 0)
            timeout--;
    }

    if (timeout == 0 || (SDMMC_STA & SDMMC_STA_CTIMEOUT))
        return -1;
    return 0;
}

/* ---- Initialize SD card ---- */
void wr_sdcard_init(void) {
    sd_gpio_init();

    /* Enable SDMMC1 clock */
    RCC_AHB4ENR |= RCC_AHB4ENR_SDMMC1;

    /* Set clock to 400 kHz for init (clock divider) */
    SDMMC_CLKCR = (APB2_HZ / 400000) - 1;
    SDMMC_CTRL = 1; /* Enable SDMMC */

    /* CMD0: GO_IDLE (reset) */
    sd_send_cmd(SD_CMD_GO_IDLE, 0, 0);
    /* small delay */
    for (volatile int i = 0; i < 10000; i++);

    /* CMD8: SEND_IF_COND (check SD v2) */
    sd_send_cmd(8, 0x1AA, 1);
    (void)SDMMC_RESP1;

    /* ACMD41: SEND_OP_COND (initiate initialization) */
    for (int tries = 0; tries < 100; tries++) {
        sd_send_cmd(SD_CMD_APP_CMD, 0, 1);
        sd_send_cmd(SD_CMD_SEND_OP_COND, 0x40FF8000, 1);
        if (SDMMC_RESP1 & (1U << 31)) /* Power-up done */
            break;
        for (volatile int i = 0; i < 100000; i++);
    }

    /* CMD2: ALL_SEND_CID */
    sd_send_cmd(2, 0, 1);

    /* CMD3: SEND_REL_ADDR (get RCA) */
    sd_send_cmd(3, 0, 1);
    sd_rca_high = (SDMMC_RESP1 >> 24) & 0xFF;
    sd_rca_low = (SDMMC_RESP1 >> 16) & 0xFF;

    /* CMD7: SELECT_CARD */
    sd_send_cmd(7, ((uint32_t)sd_rca_high << 24) |
               ((uint32_t)sd_rca_low << 16), 1);

    /* Increase clock to 25 MHz */
    SDMMC_CLKCR = (APB2_HZ / 25000000) - 1;

    sd_initialized = 1;
}

/* ---- Read a single 512-byte block ---- */
int sd_read_block(uint32_t block, uint8_t *buf) {
    if (!sd_initialized)
        return -1;

    SDMMC_DTIMER = 0xFFFFFFFF;
    SDMMC_DLEN = 512;
    SDMMC_DCTRL = (9U << 4) | (1U << 1) | (1U << 3); /* 512 bytes, read */

    sd_send_cmd(SD_CMD_READ_SINGLE, block * 512, 1);

    for (int i = 0; i < 128; i++) {
        while (!(SDMMC_STA & SDMMC_STA_RXDAVL));
        uint32_t word = SDMMC_FIFO;
        buf[i * 4]     = word & 0xFF;
        buf[i * 4 + 1] = (word >> 8) & 0xFF;
        buf[i * 4 + 2] = (word >> 16) & 0xFF;
        buf[i * 4 + 3] = (word >> 24) & 0xFF;
    }

    while (!(SDMMC_STA & SDMMC_STA_DATAEND));
    SDMMC_ICR = 0xFFFFFFFF;
    return 0;
}

/* ---- Write a single 512-byte block ---- */
int sd_write_block(uint32_t block, const uint8_t *buf) {
    if (!sd_initialized)
        return -1;

    SDMMC_DTIMER = 0xFFFFFFFF;
    SDMMC_DLEN = 512;
    SDMMC_DCTRL = (9U << 4) | (1U << 1) | (1U << 3) | (1U << 2); /* write */

    sd_send_cmd(SD_CMD_WRITE_SINGLE, block * 512, 1);

    for (int i = 0; i < 128; i++) {
        while (!(SDMMC_STA & SDMMC_STA_TXDAVL));
        uint32_t word = buf[i * 4] |
                        ((uint32_t)buf[i * 4 + 1] << 8) |
                        ((uint32_t)buf[i * 4 + 2] << 16) |
                        ((uint32_t)buf[i * 4 + 3] << 24);
        SDMMC_FIFO = word;
    }

    while (!(SDMMC_STA & SDMMC_STA_DATAEND));
    SDMMC_ICR = 0xFFFFFFFF;
    return 0;
}

/* ---- Simple file append (raw sector based, not real FAT32) ---- */
/* For production, a lightweight FAT32 implementation would parse the
 * filesystem. This simplified version writes sequential capture files
 * to dedicated sectors starting at a fixed offset. */
#define CAPTURE_FILE_START_SECTOR 1000
static uint32_t capture_sector_offset = 0;

void wr_sdcard_write(const char *fn, const uint8_t *data, int len) {
    (void)fn; /* Filename ignored in simplified version */

    if (!sd_initialized)
        return;

    /* Write data in 512-byte blocks */
    int blocks = (len + 511) / 512;
    uint8_t block_buf[512];

    for (int b = 0; b < blocks; b++) {
        memset(block_buf, 0, 512);
        int chunk = len - b * 512;
        if (chunk > 512)
            chunk = 512;
        memcpy(block_buf, data + b * 512, chunk);
        sd_write_block(CAPTURE_FILE_START_SECTOR +
                       capture_sector_offset + b, block_buf);
    }
    capture_sector_offset += blocks;
}

/* ---- Reset capture log ---- */
void wr_sdcard_reset_log(void) {
    capture_sector_offset = 0;
}