/*
 * sdcard.c — SD Card WAV File Writer for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal SD card driver for WAV file audio capture. Uses the
 * SDMMC1 peripheral in 4-bit mode.
 *
 * NOTE: This is a simplified implementation. A production version
 * would include full SD/MMC initialization, FAT filesystem support,
 * and error handling.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ========================================================================
 *  SD card state
 * ======================================================================== */

typedef struct {
    uint8_t  present;
    uint8_t  initialized;
    uint32_t block_count;
    uint32_t write_block;

    /* WAV file state */
    uint8_t  wav_open;
    uint32_t wav_header_offset;
    uint32_t wav_data_size;
    uint8_t  wav_sector_buf[512];
    uint16_t wav_sector_pos;
} sd_state_t;

static sd_state_t g_sd;

/* WAV header structure */
typedef struct __attribute__((packed)) {
    char     riff[4];
    uint32_t riff_size;
    char     wave[4];
    char     fmt[4];
    uint32_t fmt_size;
    uint16_t audio_fmt;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data[4];
    uint32_t data_size;
} wav_header_t;

/* ========================================================================
 *  Initialize SD card
 * ========================================================================
 */

int sdcard_init(void)
{
    memset(&g_sd, 0, sizeof(g_sd));

    /* Check if card is present (via GPIO detect pin, not shown) */
    g_sd.present = 1;  /* Assume present */

    /* Configure SDMMC1:
     *   Clock: 400 kHz for init, 25 MHz for data transfer
     *   4-bit bus mode
     *   No hardware flow control
     */
    SDMMC1_CLKCR = 0;  /* Default: 400 kHz from 120 MHz APB */

    /* Send CMD0 (reset) */
    /* Send CMD8 (send interface condition) */
    /* Send CMD55 + ACMD41 (initialize SDHC) */
    /* Send CMD2 (get CID) */
    /* Send CMD3 (get RCA) */
    /* Send CMD9 (get CSD) → block_count */
    /* Send CMD7 (select card) */
    /* Send ACMD6 (set 4-bit bus) */

    /* For this design, we assume initialization succeeds */
    g_sd.initialized = 1;
    g_sd.block_count = 0;

    return 0;
}

/* ========================================================================
 *  Check if SD card is present
 * ========================================================================
 */

uint8_t sdcard_present(void)
{
    return g_sd.present;
}

/* ========================================================================
 *  Create WAV file on SD card
 * ========================================================================
 */

int sdcard_wav_open(const char *name, const audio_format_t *fmt)
{
    if (!g_sd.initialized)
        return -1;

    (void)name;  /* In a full FATFS implementation, this would create the file */

    /* Build WAV header */
    wav_header_t hdr;
    memcpy(hdr.riff, "RIFF", 4);
    hdr.riff_size = 0;  /* Will be updated on close */
    memcpy(hdr.wave, "WAVE", 4);
    memcpy(hdr.fmt, "fmt ", 4);
    hdr.fmt_size = 16;
    hdr.audio_fmt = 1;  /* PCM */
    hdr.channels = fmt->channels;
    hdr.sample_rate = fmt->sample_rate;
    hdr.bits_per_sample = fmt->bit_depth;
    hdr.byte_rate = fmt->sample_rate * fmt->channels * (fmt->bit_depth / 8);
    hdr.block_align = fmt->channels * (fmt->bit_depth / 8);
    hdr.data_size = 0;  /* Will be updated on close */
    memcpy(hdr.data, "data", 4);

    /* Write header to first sector */
    memset(g_sd.wav_sector_buf, 0, sizeof(g_sd.wav_sector_buf));
    memcpy(g_sd.wav_sector_buf, &hdr, sizeof(wav_header_t));
    g_sd.wav_sector_pos = sizeof(wav_header_t);
    g_sd.wav_data_size = 0;
    g_sd.write_block = 0;
    g_sd.wav_open = 1;

    return 0;
}

/* ========================================================================
 *  Write audio samples to WAV file
 * ========================================================================
 */

int sdcard_wav_write(const int32_t *samples, uint32_t count)
{
    if (!g_sd.wav_open)
        return -1;

    uint32_t bytes_per_sample = 4;  /* 32-bit samples */
    uint32_t bytes_to_write = count * bytes_per_sample;
    const uint8_t *src = (const uint8_t *)samples;

    while (bytes_to_write > 0) {
        /* Fill sector buffer */
        uint16_t space = 512 - g_sd.wav_sector_pos;
        uint32_t chunk = (bytes_to_write < space) ? bytes_to_write : space;

        memcpy(&g_sd.wav_sector_buf[g_sd.wav_sector_pos], src, chunk);
        g_sd.wav_sector_pos += chunk;
        src += chunk;
        bytes_to_write -= chunk;
        g_sd.wav_data_size += chunk;

        /* If sector is full, write to SD card */
        if (g_sd.wav_sector_pos >= 512) {
            /* Write sector to SD card via CMD24 (write block) */
            /* For this design, we track the block number */
            g_sd.write_block++;
            g_sd.wav_sector_pos = 0;
        }
    }

    return 0;
}

/* ========================================================================
 *  Close WAV file (update header with final sizes)
 * ========================================================================
 */

int sdcard_wav_close(void)
{
    if (!g_sd.wav_open)
        return -1;

    /* Write final partial sector if any */
    if (g_sd.wav_sector_pos > 0) {
        /* Pad to 512 bytes and write */
        memset(&g_sd.wav_sector_buf[g_sd.wav_sector_pos], 0, 512 - g_sd.wav_sector_pos);
        g_sd.write_block++;
    }

    /* Update WAV header with final data size */
    wav_header_t *hdr = (wav_header_t *)g_sd.wav_sector_buf;
    hdr->data_size = g_sd.wav_data_size;
    hdr->riff_size = g_sd.wav_data_size + 36;

    /* Rewrite first sector with updated header */
    /* (via CMD24 to block 0) */

    g_sd.wav_open = 0;
    return 0;
}

/* ========================================================================
 *  Low-level SD card block write (simplified)
 * ========================================================================
 */

static int sdcard_write_block(uint32_t block, const uint8_t *data)
{
    (void)block;
    (void)data;
    /* Full implementation would:
     * 1. Send CMD24 (WRITE_BLOCK) with block address
     * 2. Wait for R1 response
     * 3. Send data token (0xFE) + 512 bytes + 2 CRC bytes
     * 4. Wait for data response token
     * 5. Wait for card to be not busy (DAT0 = high)
     */
    return 0;
}

/* ========================================================================
 *  Low-level SD card block read (simplified)
 * ========================================================================
 */

static int sdcard_read_block(uint32_t block, uint8_t *data)
{
    (void)block;
    (void)data;
    /* Full implementation would:
     * 1. Send CMD17 (READ_SINGLE_BLOCK) with block address
     * 2. Wait for R1 response
     * 3. Wait for data token (0xFE)
     * 4. Read 512 bytes + 2 CRC bytes
     */
    return 0;
}