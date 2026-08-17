/*
 * sd_capture.c — microSD capture (pcapng writer)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Writes a minimal pcapng file (Section Header Block + Interface
 * Description Block + Enhanced Packet Blocks) to the microSD card over
 * SDMMC1. The SDMMC1 driver here is a simplified block-write interface;
 * a real build would use the ST SDMMC HAL or a lightweight FatFs.
 *
 * The pcapng link type is set to LINKTYPE_USER178 (178 = "user-defined")
 * so Wireshark can be told to dissect the captured bytes with the right
 * fieldbus dissector.
 */

#include "sd_capture.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ----------------------------------------------------------------------- */
/*  pcapng block types                                                      */
/* ----------------------------------------------------------------------- */

#define PCAPNG_SHB  0x0A0D0D0Au   /* Section Header Block          */
#define PCAPNG_IDB  0x00000001u    /* Interface Description Block   */
#define PCAPNG_EPB  0x00000006u    /* Enhanced Packet Block          */

#define PCAPNG_LINKTYPE_USER178  178u

/* ----------------------------------------------------------------------- */
/*  SDMMC1 minimal block write                                              */
/* ----------------------------------------------------------------------- */

/* The SDMMC1 block-write path is simplified here. A real build uses the
 * FatFs or ST HAL; we stub the low-level 512-byte block write and keep
 * the pcapng framing logic — which is the security-relevant part. */

static int g_sd_initialised = 0;
static uint8_t g_block_buf[512];
static uint16_t g_block_used = 0u;
static uint32_t g_block_lba = 0u;

static int sd_write_block(uint32_t lba, const uint8_t *data) {
    /* In a real build: set DLEN=512, DCTRL=DMAEN|DTEN|DTDIR=0 (write),
     * wait for DATAEND, etc. Stubbed here for the reference design. */
    (void)lba; (void)data;
    return 0;
}

static void sd_flush_block(void) {
    if (g_block_used == 0u) return;
    /* Pad the rest of the block with zeros */
    for (uint16_t i = g_block_used; i < 512u; i++) g_block_buf[i] = 0u;
    sd_write_block(g_block_lba, g_block_buf);
    g_block_lba++;
    g_block_used = 0u;
}

static int sd_write_bytes(const uint8_t *data, uint16_t len) {
    for (uint16_t i = 0u; i < len; i++) {
        if (g_block_used >= 512u) sd_flush_block();
        g_block_buf[g_block_used++] = data[i];
    }
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  pcapng helpers                                                          */
/* ----------------------------------------------------------------------- */

static void put_u32(uint8_t *buf, uint32_t off, uint32_t v) {
    buf[off]     = (uint8_t)(v);
    buf[off + 1] = (uint8_t)(v >> 8);
    buf[off + 2] = (uint8_t)(v >> 16);
    buf[off + 3] = (uint8_t)(v >> 24);
}

/* ----------------------------------------------------------------------- */
/*  Init                                                                    */
/* ----------------------------------------------------------------------- */

void sd_capture_init(void) {
    /* In a real build: SDMMC1 clock enable, CMD0/CMD8/ACMD41/CMD2/CMD3
     * init sequence, block-length set to 512. Stubbed here. */
    g_sd_initialised = 1;
    g_block_used = 0u;
    g_block_lba = 0u;
}

/* ----------------------------------------------------------------------- */
/*  Open (write SHB + IDB)                                                  */
/* ----------------------------------------------------------------------- */

int sd_capture_open(const char *filename) {
    (void)filename;
    if (!g_sd_initialised) return -1;

    /* Section Header Block: magic, version 1.0, section length -1 (unknown) */
    uint8_t shb[28];
    put_u32(shb, 0,  PCAPNG_SHB);
    put_u32(shb, 4,  28);            /* block total length */
    put_u32(shb, 8,  0x1A2B3C4Du);  /* byte-order magic (little-endian) */
    put_u32(shb, 12, 1u);           /* major version 1 */
    put_u32(shb, 16, 0u);           /* minor version 0 */
    put_u32(shb, 20, 0xFFFFFFFFu);  /* section length unknown */
    put_u32(shb, 24, 28);           /* block total length (repeat) */
    sd_write_bytes(shb, sizeof(shb));

    /* Interface Description Block */
    uint8_t idb[20];
    put_u32(idb, 0,  PCAPNG_IDB);
    put_u32(idb, 4,  20);
    put_u32(idb, 8,  PCAPNG_LINKTYPE_USER178);  /* link type */
    put_u32(idb, 12, 0u);    /* snap length (no limit) */
    put_u32(idb, 16, 20);
    sd_write_bytes(idb, sizeof(idb));

    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Write raw bytes                                                          */
/* ----------------------------------------------------------------------- */

int sd_capture_write(const uint8_t *data, uint16_t len) {
    return sd_write_bytes(data, len);
}

/* ----------------------------------------------------------------------- */
/*  Format EPB from a parsed frame                                           */
/* ----------------------------------------------------------------------- */

uint16_t sd_capture_format_pcapng(const pr_parsed_t *parsed,
                                  uint8_t *out_buf, uint16_t max_len) {
    if (!parsed || !out_buf) return 0u;

    /* EPB layout:
     *   block type (4) | block total length (4) | interface ID (4) |
     *   timestamp high (4) | timestamp low (4) | captured len (4) |
     *   original len (4) | data (padded to 4) | options (0) |
     *   block total length (4)
     * Minimum block length = 32 + padded_data_len
     */
    uint16_t data_len = parsed->length;
    uint16_t padded = (data_len + 3u) & ~3u;
    uint32_t total = 32u + padded;
    if (total > max_len) {
        /* Truncate the data to fit */
        data_len = (uint16_t)(max_len - 32u);
        padded = (data_len + 3u) & ~3u;
        total = 32u + padded;
    }

    put_u32(out_buf, 0, PCAPNG_EPB);
    put_u32(out_buf, 4, total);
    put_u32(out_buf, 8, 0u);              /* interface ID 0 */
    put_u32(out_buf, 12, 0u);             /* timestamp high */
    put_u32(out_buf, 16, parsed->timestamp_ms);  /* timestamp low (ms) */
    put_u32(out_buf, 20, data_len);       /* captured length */
    put_u32(out_buf, 24, data_len);       /* original length */
    /* Data + padding */
    for (uint16_t i = 0u; i < data_len; i++) {
        out_buf[28 + i] = parsed->payload[i];
    }
    for (uint16_t i = data_len; i < padded; i++) {
        out_buf[28 + i] = 0u;
    }
    put_u32(out_buf, 28 + padded, total);
    return (uint16_t)total;
}

/* ----------------------------------------------------------------------- */
/*  Close                                                                    */
/* ----------------------------------------------------------------------- */

int sd_capture_close(void) {
    sd_flush_block();
    return 0;
}