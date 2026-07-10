/*
 * inject.c — Audio Injection Engine for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Manages audio injection clips stored in flash memory.
 * Clips are loaded into SRAM ring buffers for real-time playback
 * through the SAI1_B (host) interface.
 *
 * Flash layout (Bank 2, starting at INJECT_FLASH_BASE):
 *   Sector 0:  Clip 0 (up to 64 KB)
 *   Sector 1:  Clip 1 (up to 64 KB)
 *   ...
 *   Sector 15: Clip 15 (up to 64 KB)
 *
 * Each clip starts with a 16-byte header:
 *   [0-3]  Magic: 0x45434831 ("ECH1")
 *   [4-7]  Sample rate (Hz)
 *   [8]    Bit depth (16/24/32)
 *   [9]    Channels
 *   [10-11] Reserved
 *   [12-15] Sample count
 * Followed by raw 32-bit PCM audio data.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ========================================================================
 *  Injection state
 * ======================================================================== */

#define CLIP_MAGIC  0x45434831U  /* "ECH1" */

typedef struct {
    uint32_t magic;
    uint32_t sample_rate;
    uint8_t  bit_depth;
    uint8_t  channels;
    uint16_t reserved;
    uint32_t sample_count;
} clip_header_t;

typedef struct {
    uint8_t   active;
    uint8_t   clip_id;
    inject_mode_t mode;
    uint8_t   gain;

    /* Playback state */
    uint32_t  flash_addr;      /* Current read position in flash */
    uint32_t  samples_played;
    uint32_t  total_samples;

    /* SRAM staging buffer (loaded from flash in chunks) */
    int32_t   stage_buf[AUDIO_FRAME_SAMPLES * 2];
    uint16_t  stage_read;     /* Read index within stage buffer */
    uint16_t  stage_fill;     /* Number of valid samples in stage buffer */
} inject_state_t;

static inject_state_t g_inject;

/* ========================================================================
 *  Initialize injection engine
 * ======================================================================== */

int inject_init(void)
{
    memset(&g_inject, 0, sizeof(g_inject));
    return 0;
}

/* ========================================================================
 *  Load clip header from flash
 * ======================================================================== */

static int read_clip_header(uint8_t clip_id, clip_header_t *hdr)
{
    if (clip_id >= INJECT_MAX_CLIPS)
        return -1;

    uint32_t addr = INJECT_FLASH_BASE + clip_id * 65536U;
    memcpy(hdr, (const void *)addr, sizeof(clip_header_t));

    if (hdr->magic != CLIP_MAGIC)
        return -1;

    return 0;
}

/* ========================================================================
 *  Load clip data from flash into SRAM staging buffer
 * ======================================================================== */

static void refill_stage_buffer(void)
{
    if (g_inject.stage_fill > 0 && g_inject.stage_read > 0) {
        /* Compact: move remaining samples to start of buffer */
        memmove(g_inject.stage_buf,
                &g_inject.stage_buf[g_inject.stage_read],
                g_inject.stage_fill * sizeof(int32_t));
        g_inject.stage_fill -= g_inject.stage_read;
        g_inject.stage_read = 0;
    }

    uint16_t space = (AUDIO_FRAME_SAMPLES * 2) - g_inject.stage_fill;
    if (space == 0)
        return;

    uint32_t remaining = g_inject.total_samples - g_inject.samples_played;
    if (remaining == 0) {
        /* Clip exhausted — do nothing */
        return;
    }

    uint16_t to_load = (space < remaining) ? space : (uint16_t)remaining;

    /* Copy from flash to SRAM staging buffer */
    /* Flash is memory-mapped, so we can read it directly */
    uint32_t data_addr = g_inject.flash_addr;
    const int32_t *flash_samples = (const int32_t *)data_addr;

    memcpy(&g_inject.stage_buf[g_inject.stage_fill], flash_samples, to_load * sizeof(int32_t));

    g_inject.stage_fill += to_load;
    g_inject.flash_addr += to_load * sizeof(int32_t);
    g_inject.samples_played += to_load;
}

/* ========================================================================
 *  Upload clip data to flash
 *
 *  This function writes incoming clip data to the flash memory.
 *  The caller must erase the sector before writing.
 * ======================================================================== */

int inject_load_clip(uint8_t clip_id, const uint8_t *data, uint32_t size)
{
    if (clip_id >= INJECT_MAX_CLIPS)
        return -1;

    uint32_t base_addr = INJECT_FLASH_BASE + clip_id * 65536U;

    /* If this is the first chunk, write the header */
    /* Check if the sector already has the magic */
    uint32_t existing_magic = REG32(base_addr);
    if (existing_magic != CLIP_MAGIC && size >= sizeof(clip_header_t)) {
        /* Erase the sector first */
        flash_erase_sector(base_addr);
        /* Write the data (includes header) */
        flash_write(base_addr, data, size);
    } else {
        /* Append to existing clip (offset within sector) */
        flash_write(base_addr + size, data, size);
    }

    return 0;
}

/* ========================================================================
 *  Start injection playback
 * ======================================================================== */

int inject_start(uint8_t clip_id, inject_mode_t mode, uint8_t gain)
{
    clip_header_t hdr;

    if (read_clip_header(clip_id, &hdr) != 0)
        return -1;

    g_inject.active = 1;
    g_inject.clip_id = clip_id;
    g_inject.mode = mode;
    g_inject.gain = gain;
    g_inject.samples_played = 0;
    g_inject.total_samples = hdr.sample_count;

    /* Set flash read address to just after the header */
    g_inject.flash_addr = INJECT_FLASH_BASE + clip_id * 65536U + sizeof(clip_header_t);

    /* Reset staging buffer */
    g_inject.stage_read = 0;
    g_inject.stage_fill = 0;

    /* Pre-fill the staging buffer */
    refill_stage_buffer();

    return 0;
}

/* ========================================================================
 *  Stop injection
 * ======================================================================== */

void inject_stop(void)
{
    g_inject.active = 0;
}

/* ========================================================================
 *  Get next audio frame from the inject clip
 *
 *  Returns 0 on success, -1 if the clip is exhausted.
 * ======================================================================== */

int inject_get_next_frame(int32_t *buf, uint16_t samples)
{
    if (!g_inject.active)
        return -1;

    /* Refill staging buffer if running low */
    if (g_inject.stage_fill - g_inject.stage_read < samples) {
        refill_stage_buffer();
    }

    /* Check if we have enough samples */
    uint16_t available = g_inject.stage_fill - g_inject.stage_read;
    if (available < samples) {
        /* Not enough — fill remaining with silence */
        uint16_t avail = available;
        memcpy(buf, &g_inject.stage_buf[g_inject.stage_read], avail * sizeof(int32_t));
        memset(buf + avail, 0, (samples - avail) * sizeof(int32_t));
        g_inject.stage_read += avail;
        return -1;  /* Signal clip exhausted */
    }

    /* Copy samples from staging buffer to output */
    memcpy(buf, &g_inject.stage_buf[g_inject.stage_read], samples * sizeof(int32_t));
    g_inject.stage_read += samples;

    /* Apply gain */
    if (g_inject.gain != 100) {
        for (uint16_t i = 0; i < samples; i++) {
            int64_t scaled = (int64_t)buf[i] * g_inject.gain / 100;
            if (scaled > 0x7FFFFFFFLL) scaled = 0x7FFFFFFFLL;
            if (scaled < -0x80000000LL) scaled = -0x80000000LL;
            buf[i] = (int32_t)scaled;
        }
    }

    return 0;
}

/* ========================================================================
 *  Get injection status
 * ======================================================================== */

uint8_t inject_is_active(void)
{
    return g_inject.active;
}

uint32_t inject_get_progress(void)
{
    if (g_inject.total_samples == 0)
        return 0;
    return (g_inject.samples_played * 100U) / g_inject.total_samples;
}

/* ========================================================================
 *  List available clips (returns bitmask of clip IDs that have valid headers)
 * ======================================================================== */

uint16_t inject_list_clips(void)
{
    uint16_t mask = 0;
    for (uint8_t i = 0; i < INJECT_MAX_CLIPS; i++) {
        clip_header_t hdr;
        if (read_clip_header(i, &hdr) == 0) {
            mask |= (1U << i);
        }
    }
    return mask;
}