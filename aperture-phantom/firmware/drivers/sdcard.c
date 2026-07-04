/*
 * aperture-phantom / firmware / drivers / sdcard.c
 *
 * Minimal SDMMC1 driver in polled mode for the microSD capture/replay
 * storage. Supports SD v2 high-capacity cards, block read/write, and a
 * tiny FAT32 file API oriented toward the .APF capture files and replay
 * library listing. Real builds use FatFs; here we provide a small file
 * table in the first sector for the replay library.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"

#define SD_BLOCK_SIZE 512u

static int g_initialized;
static uint8_t g_block_buf[SD_BLOCK_SIZE];
static int    g_open_write;
static int    g_open_read;
static char   g_open_name[24];
static uint32_t g_open_offset;

/* ---- low-level SDMMC primitives ---- */
static void sd_wait_cmd_done(void) {
    extern volatile uint32_t g_ticks_ms;
    uint32_t t0 = g_ticks_ms;
    while ((SDMMC1_STA & (1u << 11)) == 0) {  /* CMDSENT / CMDREND */
        if ((g_ticks_ms - t0) > 200) return;
    }
    SDMMC1_ICR = 0xFFFFFFFFu; /* clear all */
}
static void sd_cmd(uint8_t idx, uint32_t arg, uint8_t resp) {
    SDMMC1_ARG = arg;
    SDMMC1_CMD = (idx << 0) | ((uint32_t)resp << 6) | (1u << 10); /* CPSMEN */
    sd_wait_cmd_done();
}

int sdcard_init(void) {
    /* Power on, clock 400 kHz for init. */
    SDMMC1_POWER = 3u; /* ON */
    SDMMC1_CLKCR = (uint32_t)(120000000u / 400000u) | (1u << 11); /* PWRSAV */
    for (volatile int i = 0; i < 100000; i++) { }

    /* CMD0 GO_IDLE_STATE. */
    sd_cmd(0, 0, 0);

    /* CMD8 SEND_IF_COND, 0x1AA. */
    sd_cmd(8, 0x1AA, 1);

    /* ACMD41 (CMD55 + ACMD41) with HCS bit. */
    for (int tries = 0; tries < 100; tries++) {
        sd_cmd(55, 0, 1);
        sd_cmd(41, 0x40100000u, 1);
        if (SDMMC1_RESP1 & 0x80000000u) break; /* busy bit set = ready */
    }

    /* CMD2 ALL_SEND_CID, CMD3 SEND_RELATIVE_ADDR. */
    sd_cmd(2, 0, 2);
    sd_cmd(3, 0, 6);
    uint32_t rca = SDMMC1_RESP1 >> 16;

    /* CMD7 SELECT. */
    sd_cmd(7, rca << 16, 6);

    /* Bump clock to 25 MHz. */
    SDMMC1_CLKCR = (uint32_t)(120000000u / 25000000u) | (1u << 11);

    /* CMD16 block size 512. */
    sd_cmd(16, SD_BLOCK_SIZE, 1);

    g_initialized = 1;
    return 0;
}

int sdcard_mount(void) {
    if (!g_initialized) return -1;
    /* Read sector 0: a tiny directory of replay files. */
    if (sdcard_read_block(0, g_block_buf) != 0) return -1;
    /* If signature absent, init the directory. */
    if (g_block_buf[0] != 'A' || g_block_buf[1] != 'P' || g_block_buf[2] != 'D') {
        memset(g_block_buf, 0, SD_BLOCK_SIZE);
        g_block_buf[0]='A'; g_block_buf[1]='P'; g_block_buf[2]='D'; /* dir */
        g_block_buf[3] = 0; /* count */
        sdcard_write_block(0, g_block_buf);
    }
    return 0;
}

int sdcard_read_block(uint32_t blk, uint8_t *buf) {
    SDMMC1_DTIMER = 0xFFFFFFu;
    SDMMC1_DLENR  = SD_BLOCK_SIZE;
    SDMMC1_DCTRL = (9u << 4) | (1u << 1) | (1u << 3); /* DTEN, read */
    sd_cmd(17, blk, 1);
    for (uint32_t i = 0; i < SD_BLOCK_SIZE / 4; i++) {
        while ((SDMMC1_STA & (1u << 25)) == 0) { } /* RXFIFOHF */
        ((uint32_t *)buf)[i] = *(volatile uint32_t *)(SDMMC1_BASE + 0x80u);
    }
    SDMMC1_ICR = 0xFFFFFFFFu;
    return 0;
}

int sdcard_write_block(uint32_t blk, const uint8_t *buf) {
    SDMMC1_DTIMER = 0xFFFFFFu;
    SDMMC1_DLENR  = SD_BLOCK_SIZE;
    SDMMC1_DCTRL = (9u << 4) | (1u << 1); /* DTEN, write */
    sd_cmd(24, blk, 1);
    for (uint32_t i = 0; i < SD_BLOCK_SIZE / 4; i++) {
        while ((SDMMC1_STA & (1u << 24)) == 0) { } /* TXFIFOHE */
        *(volatile uint32_t *)(SDMMC1_BASE + 0x80u) = ((const uint32_t *)buf)[i];
    }
    SDMMC1_ICR = 0xFFFFFFFFu;
    return 0;
}

/* ---- Tiny file API ---- */

int sdcard_open_write(const char *name) {
    /* Find a free slot in the directory (sector 0), add entry. */
    if (sdcard_read_block(0, g_block_buf) != 0) return -1;
    uint8_t count = g_block_buf[3];
    /* Each entry: 24 bytes name + 4 bytes start blk + 4 bytes len. */
    for (uint8_t i = 0; i < count; i++) {
        char *e = (char *)&g_block_buf[4 + i * 32];
        if (strncmp(e, name, 24) == 0) {
            g_open_write = 1; g_open_read = 0;
            strncpy(g_open_name, name, sizeof(g_open_name));
            g_open_offset = 0;
            return 0;
        }
    }
    if (count >= MAX_REPLAY_FILES) return -1;
    /* Append entry: start block = 1 + count*64 (give each file 32 KB). */
    uint32_t start = 1u + (uint32_t)count * 64u;
    char *e = (char *)&g_block_buf[4 + count * 32];
    memset(e, 0, 32);
    strncpy(e, name, 24);
    *(uint32_t *)(e + 24) = start;
    *(uint32_t *)(e + 28) = 0;
    g_block_buf[3] = count + 1;
    sdcard_write_block(0, g_block_buf);

    g_open_write = 1; g_open_read = 0;
    strncpy(g_open_name, name, sizeof(g_open_name));
    g_open_offset = 0;
    return 0;
}

int sdcard_write(const uint8_t *buf, uint32_t len) {
    if (!g_open_write) return -1;
    /* Find entry in dir. */
    if (sdcard_read_block(0, g_block_buf) != 0) return -1;
    uint8_t count = g_block_buf[3];
    uint32_t start = 0, total = 0;
    for (uint8_t i = 0; i < count; i++) {
        char *e = (char *)&g_block_buf[4 + i * 32];
        if (strncmp(e, g_open_name, 24) == 0) {
            start = *(uint32_t *)(e + 24);
            total = *(uint32_t *)(e + 28);
            break;
        }
    }
    if (!start) return -1;
    /* Append to file: write at start + total. */
    uint32_t off = g_open_offset;
    uint32_t blk = start + (off / SD_BLOCK_SIZE);
    uint32_t boff = off % SD_BLOCK_SIZE;
    while (len) {
        uint32_t chunk = SD_BLOCK_SIZE - boff;
        if (chunk > len) chunk = len;
        if (boff || chunk < SD_BLOCK_SIZE) {
            sdcard_read_block(blk, g_block_buf);
            memcpy(&g_block_buf[boff], buf, chunk);
            sdcard_write_block(blk, g_block_buf);
        } else {
            sdcard_write_block(blk, buf);
        }
        buf    += chunk;
        len    -= chunk;
        off    += chunk;
        blk    += 1;
        boff    = 0;
        total  += chunk;
    }
    g_open_offset = off;
    /* Update total in dir. */
    if (sdcard_read_block(0, g_block_buf) == 0) {
        for (uint8_t i = 0; i < count; i++) {
            char *e = (char *)&g_block_buf[4 + i * 32];
            if (strncmp(e, g_open_name, 24) == 0) {
                *(uint32_t *)(e + 28) = total;
                sdcard_write_block(0, g_block_buf);
                break;
            }
        }
    }
    return 0;
}

int sdcard_close(void) {
    g_open_write = 0; g_open_read = 0;
    return 0;
}

int sdcard_open_read(const char *name) {
    if (sdcard_read_block(0, g_block_buf) != 0) return -1;
    uint8_t count = g_block_buf[3];
    for (uint8_t i = 0; i < count; i++) {
        char *e = (char *)&g_block_buf[4 + i * 32];
        if (strncmp(e, name, 24) == 0) {
            g_open_write = 0; g_open_read = 1;
            strncpy(g_open_name, name, sizeof(g_open_name));
            g_open_offset = 0;
            return 0;
        }
    }
    return -1;
}

int sdcard_read(uint8_t *buf, uint32_t len) {
    if (!g_open_read) return -1;
    if (sdcard_read_block(0, g_block_buf) != 0) return -1;
    uint8_t count = g_block_buf[3];
    uint32_t start = 0, total = 0;
    for (uint8_t i = 0; i < count; i++) {
        char *e = (char *)&g_block_buf[4 + i * 32];
        if (strncmp(e, g_open_name, 24) == 0) {
            start = *(uint32_t *)(e + 24);
            total = *(uint32_t *)(e + 28);
            break;
        }
    }
    if (!start) return -1;
    if (g_open_offset + len > total) return -1;

    uint32_t off = g_open_offset;
    uint32_t blk = start + (off / SD_BLOCK_SIZE);
    uint32_t boff = off % SD_BLOCK_SIZE;
    uint32_t want = len;
    while (want) {
        uint32_t chunk = SD_BLOCK_SIZE - boff;
        if (chunk > want) chunk = want;
        if (boff || chunk < SD_BLOCK_SIZE) {
            sdcard_read_block(blk, g_block_buf);
            memcpy(buf, &g_block_buf[boff], chunk);
        } else {
            sdcard_read_block(blk, buf);
        }
        buf  += chunk;
        want -= chunk;
        off  += chunk;
        blk  += 1;
        boff = 0;
    }
    g_open_offset = off;
    return 0;
}

int sdcard_list_replay(char names[][24], int max) {
    if (sdcard_read_block(0, g_block_buf) != 0) return 0;
    uint8_t count = g_block_buf[3];
    int n = (count < max) ? count : max;
    for (int i = 0; i < n; i++) {
        memcpy(names[i], &g_block_buf[4 + i * 32], 24);
    }
    return n;
}

int sdcard_present(void) { return board_sd_present(); }