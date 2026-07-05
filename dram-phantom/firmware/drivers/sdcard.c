/*
 * sdcard.c — minimal SDIO + FAT-free file writer for DRAM-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * We do not implement a full FAT filesystem; instead we write raw files into
 * a flat namespace on the card using a simple append-only scheme. The
 * companion app can reconstruct the dumps. Each file open appends at the
 * next free sector. This keeps the firmware tiny and dependency-free.
 */

#include "board.h"
#include "registers.h"
#include <string.h>
#include <stdio.h>

#define SD_BLOCK 512

static int sd_cmd(uint8_t idx, uint32_t arg, uint8_t *resp, uint8_t resp_len);

int sdcard_init(void) {
    /* Send CMD0 (GO_IDLE), CMD8 (SEND_IF_COND), ACMD41 (SD_SEND_OP_COND),
     * CMD2 (ALL_SEND_CID), CMD3 (SEND_RELATIVE_ADDR). This is a skeleton. */
    uint8_t r[6];
    if (sd_cmd(0, 0, r, 1) != 0) return -1;
    if (sd_cmd(8, 0x000001AA, r, 4) != 0) return -2;
    /* ACMD41 sequence omitted for brevity */
    return 0;
}

static int sd_cmd(uint8_t idx, uint32_t arg, uint8_t *resp, uint8_t resp_len) {
    (void)idx; (void)arg; (void)resp; (void)resp_len;
    /* Real implementation drives CMD line and reads resp tokens. */
    return 0;
}

/* Append-only file table: name -> next sector. We keep a small in-RAM table. */
typedef struct {
    char name[24];
    uint32_t next_sector;
} file_entry_t;

#define MAX_FILES 8
static file_entry_t g_files[MAX_FILES];
static int g_files_n = 0;

static file_entry_t *file_get_or_create(const char *name) {
    for (int i = 0; i < g_files_n; i++) {
        if (strncmp(g_files[i].name, name, sizeof(g_files[i].name)) == 0)
            return &g_files[i];
    }
    if (g_files_n >= MAX_FILES) return NULL;
    file_entry_t *f = &g_files[g_files_n++];
    strncpy(f->name, name, sizeof(f->name) - 1);
    f->name[sizeof(f->name) - 1] = 0;
    f->next_sector = 0x1000 + g_files_n * 2048; /* arbitrary start sectors */
    return f;
}

int sdcard_write_file(const char *name, const void *buf, uint32_t len) {
    file_entry_t *f = file_get_or_create(name);
    if (!f) return -1;
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t off = 0;
    while (off < len) {
        uint8_t blk[SD_BLOCK];
        memset(blk, 0, sizeof(blk));
        uint32_t chunk = len - off; if (chunk > SD_BLOCK) chunk = SD_BLOCK;
        memcpy(blk, p + off, chunk);
        /* Write one block at f->next_sector. Real SDIO write omitted. */
        (void)blk;
        f->next_sector++;
        off += chunk;
    }
    return 0;
}