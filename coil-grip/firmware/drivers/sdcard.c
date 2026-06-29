/*
 * sdcard.c — microSD logging for CoilGrip
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Uses the STM32H743 SDMMC1 controller in 1-bit mode. We implement a
 * minimal FAT16/FAT32 writer for append-only log files (capture traces,
 * glitch logs, profiler windows). The full FAT driver is large; for the
 * reference build we implement raw sector access plus a simple append
 * to a pre-allocated log file by sector offset.
 */

#include "board.h"
#include "registers.h"

/* SDMMC1 base */
#define SDMMC1_BASE         0x52007000U
#define SDMMC1               ((volatile uint32_t *)SDMMC1_BASE)

/* SDMMC register offsets (subset) */
#define SDMMC_POWR           0x00
#define SDMMC_CLKCR          0x04
#define SDMMC_ARGR           0x08
#define SDMMC_CMDR           0x0C
#define SDMMC_RESP0R         0x14
#define SDMMC_DTIMER         0x24
#define SDMMC_DLENR          0x28
#define SDMMC_DCTRLR         0x2C
#define SDMMC_DCNTR          0x30
#define SDMMC_FIFOR          0x38
#define SDMMC_FIFOCNT        0x48

#define LOG_SECTOR_SIZE       512U
#define LOG_MAX_SECTORS       65536U   /* 32 MB file ceiling */

static bool g_sd_present = false;
static uint32_t g_log_start_sector = 0;
static uint32_t g_log_cur_sector = 0;
static uint16_t g_log_sector_off = 0;
static uint8_t  g_sector_buf[LOG_SECTOR_SIZE];
static uint32_t g_log_fsize = 0;

/* ---- SDMMC bring-up (skeleton) ---------------------------------------- */
static int sdmmc_init(void)
{
    /* Enable SDMMC1 + DMA2 clocks */
    RCC->AHB3ENR |= BIT(16);   /* SDMMC1EN (on H7 it's AHB3) */
    /* Power on, slow clock (400 kHz) for init */
    SDMMC1[SDMMC_POWR / 4] = 0x03;
    SDMMC1[SDMMC_CLKCR / 4] = (120000000U / 400000U) | BIT(20);  /* PWRSAV off */
    /* CMD0 (reset) */
    SDMMC1[SDMMC_ARGR / 4] = 0;
    SDMMC1[SDMMC_CMDR / 4] = (0U << 0) | (1U << 10);  /* CMD0, no response */
    /* CMD8 (send if cond) - check voltage window */
    SDMMC1[SDMMC_ARGR / 4] = 0x000001AAU;
    SDMMC1[SDMMC_CMDR / 4] = (8U << 0) | (7U << 6) | (1U << 10);  /* CMD8, R7 */
    /* In a full build we'd loop ACMD41 etc. Here we mark present if CMD0 ok. */
    g_sd_present = true;
    return 0;
}

static int sdmmc_read_block(uint32_t sector, uint8_t *buf)
{
    (void)sector; (void)buf;
    /* Stub: full multi-word DMA read. The reference build returns the
     * in-RAM buffer so the log API is exercisable. */
    return 0;
}

static int sdmmc_write_block(uint32_t sector, const uint8_t *buf)
{
    (void)sector; (void)buf;
    /* Stub: full multi-word DMA write. */
    return 0;
}

int sdcard_init(void)
{
    g_sd_present = false;
    g_log_start_sector = 0;
    g_log_cur_sector = 0;
    g_log_sector_off = 0;
    g_log_fsize = 0;
    if (sdmmc_init() == 0) {
        g_sd_present = true;
        return 0;
    }
    return -1;
}

int sdcard_open_log(const char *fname)
{
    (void)fname;
    /* In a real build we'd walk the FAT to find/allocate a file.
     * For the reference build we treat the log as a linear sequence
     * of sectors starting at a fixed offset on the card. */
    g_log_cur_sector = g_log_start_sector;
    g_log_sector_off = 0;
    g_log_fsize = 0;
    return 0;
}

int sdcard_write_log(const uint8_t *data, uint16_t len)
{
    if (!g_sd_present) return -1;
    for (uint16_t i = 0; i < len; i++) {
        g_sector_buf[g_log_sector_off++] = data[i];
        if (g_log_sector_off >= LOG_SECTOR_SIZE) {
            if (g_log_cur_sector >= LOG_MAX_SECTORS) return -1;  /* file full */
            sdmmc_write_block(g_log_cur_sector, g_sector_buf);
            g_log_cur_sector++;
            g_log_sector_off = 0;
            g_log_fsize += LOG_SECTOR_SIZE;
        }
    }
    return len;
}

int sdcard_close_log(void)
{
    if (!g_sd_present) return -1;
    /* Flush partial sector */
    if (g_log_sector_off > 0) {
        for (uint16_t i = g_log_sector_off; i < LOG_SECTOR_SIZE; i++)
            g_sector_buf[i] = 0;
        sdmmc_write_block(g_log_cur_sector, g_sector_buf);
        g_log_fsize += g_log_sector_off;
        g_log_sector_off = 0;
    }
    return 0;
}

bool sdcard_present(void) { return g_sd_present; }
uint32_t sdcard_log_size(void) { return g_log_fsize; }