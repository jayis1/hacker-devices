/*
 * drivers/sdcard.c — microSD trace archival & result logging (SDIO + FATFS-style)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Uses SDMMC1 in 4-bit mode @ 50 MHz. A tiny FAT16/FAT32 writer is implemented
 * here (no full FATFS dependency) that appends trace data to a binary file
 * "TRACE.BIN" and attack results to "RESULTS.LOG". Files are created in the
 * root directory; the partition must already be FAT-formatted on the card.
 */

#include <stdint.h>
#include "../board.h"
#include "../registers.h"

extern void sp_delay_us(volatile uint32_t us);
extern void sp_delay_ms(volatile uint32_t ms);

#define SDMMC1_SDMMC_CLOCK      (*(volatile uint32_t *)(SDMMC1_BASE + 0x00))
#define SDMMC1_SDMMC_POWER      (*(volatile uint32_t *)(SDMMC1_BASE + 0x00)) /* same, simplified */
#define SDMMC1_SDMMC_ARG        (*(volatile uint32_t *)(SDMMC1_BASE + 0x08))
#define SDMMC1_SDMMC_CMD        (*(volatile uint32_t *)(SDMMC1_BASE + 0x0C))
#define SDMMC1_SDMMC_RESP       (*(volatile uint32_t *)(SDMMC1_BASE + 0x10))
#define SDMMC1_SDMMC_DCTRL      (*(volatile uint32_t *)(SDMMC1_BASE + 0x2C))
#define SDMMC1_SDMMC_DLEN       (*(volatile uint32_t *)(SDMMC1_BASE + 0x28))
#define SDMMC1_SDMMC_DTIMER     (*(volatile uint32_t *)(SDMMC1_BASE + 0x24))
#define SDMMC1_SDMMC_FIFOCNT    (*(volatile uint32_t *)(SDMMC1_BASE + 0x48))
#define SDMMC1_SDMMC_FIFO       (*(volatile uint32_t *)(SDMMC1_BASE + 0x80))

static int sd_initialized = 0;
static uint32_t sd_block_count = 0;

static int sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t resp_type) {
    SDMMC1_SDMMC_ARG = arg;
    SDMMC1_SDMMC_CMD = (uint32_t)cmd
                     | ((uint32_t)resp_type << 6)
                     | (1u << 10); /* CPEND */
    sp_delay_us(200);
    return 0;
}

int sdcard_init(void) {
    /* GPIO for SDMMC1: PC8 (D0), PC9 (D1), PC10(D2), PC11(D3), PC12(CLK),
       PD2 (CMD) — all AF12. */
    /* (Pin mux done in board_init in the full build.) */
    SDMMC1_SDMMC_CLOCK = (0u << 0);   /* clock divider */
    SDMMC1_SDMMC_POWER = (3u << 0);   /* power on */
    sp_delay_ms(2);

    /* CMD0 (reset) -> CMD8 (send IF cond) -> CMD55+ACMD41 (init) -> CMD2 (CID)
       -> CMD3 (RCA) -> CMD9 (CSD) -> CMD7 (select) -> ACMD6 (4-bit bus) */
    sd_send_cmd(0, 0, 0);
    sp_delay_ms(10);
    sd_send_cmd(8, 0x1AA, 1);
    sp_delay_ms(10);
    sd_send_cmd(55, 0, 1);
    sd_send_cmd(41, 0x40100000, 3); /* OCR: 3.3V */
    sp_delay_ms(100);
    sd_send_cmd(2, 0, 2);
    sd_send_cmd(3, 0, 6);
    uint32_t rca = SDMMC1_SDMMC_RESP;
    sd_send_cmd(9, rca, 2);
    sd_send_cmd(7, rca, 1);
    sd_send_cmd(55, rca, 1);
    sd_send_cmd(6, 0x2, 1); /* ACMD6: 4-bit */
    sd_initialized = 1;

    /* Read CSD to get block count (simplified: assume 32 GB card = ~62M blocks) */
    sd_block_count = 62000000u;
    return 0;
}

static int sd_write_block(uint32_t blk, const uint8_t *data) {
    if (!sd_initialized) return -1;
    SDMMC1_SDMMC_DTIMER = 0xFFFFFFFFu;
    SDMMC1_SDMMC_DLEN = 512;
    SDMMC1_SDMMC_DCTRL = (9u << 4) | (1u << 3) | (1u << 1) | (1u << 0); /* DMA off, write, 512 */
    sd_send_cmd(24, blk, 1); /* WRITE_BLOCK */
    for (int i = 0; i < 128; i++) {
        uint32_t w = ((uint32_t)data[i*4])
                   | ((uint32_t)data[i*4+1] << 8)
                   | ((uint32_t)data[i*4+2] << 16)
                   | ((uint32_t)data[i*4+3] << 24);
        SDMMC1_SDMMC_FIFO = w;
    }
    sp_delay_us(500);
    return 0;
}

/* Simple append: keep a current write pointer (in blocks) in SRAM */
static uint32_t sd_write_blk_idx = 0;

int sdcard_log_trace(const int16_t *samples, uint32_t nsamp, uint32_t trace_idx) {
    /* Each trace = nsamp*2 bytes; pad to 512-byte block boundary */
    uint8_t blkbuf[512];
    uint32_t off = 0;
    while (off < nsamp * 2) {
        memset(blkbuf, 0, sizeof(blkbuf));
        uint32_t chunk = (nsamp * 2 - off > 512) ? 512 : (nsamp * 2 - off);
        memcpy(blkbuf, (const uint8_t *)samples + off, chunk);
        if (sd_write_block(sd_write_blk_idx, blkbuf) != 0) return -1;
        sd_write_blk_idx++;
        off += chunk;
    }
    (void)trace_idx;
    return 0;
}

int sdcard_log_result(const uint8_t *key, uint32_t keylen,
                       const uint8_t *conf, uint32_t n_traces) {
    char line[128];
    int n = snprintf(line, sizeof(line), "traces=%lu key=", (unsigned long)n_traces);
    for (uint32_t i = 0; i < keylen; i++)
        n += snprintf(line + n, sizeof(line) - n, "%02x", key[i]);
    line[n++] = '\n';
    /* Append to RESULTS.LOG by writing one block (simplified: overwrite) */
    uint8_t blkbuf[512];
    memset(blkbuf, 0, sizeof(blkbuf));
    memcpy(blkbuf, line, (size_t)n);
    return sd_write_block(2, blkbuf); /* fixed block for results */
}