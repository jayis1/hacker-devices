/*
 * capture.c — Audio Capture Engine for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Manages audio capture to SDRAM, SD card (WAV), or streaming
 * over BLE/USB. Uses a ring buffer in external SDRAM for
 * continuous capture, with optional SD card offloading.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ========================================================================
 *  Capture state
 * ======================================================================== */

typedef struct {
    uint8_t  active;
    cap_dest_t dest;
    uint32_t frames_captured;
    uint32_t bytes_written;

    /* SDRAM ring buffer */
    int32_t *sdram_base;
    uint32_t sdram_write_offset;  /* In samples */
    uint32_t sdram_capacity;       /* In samples */

    /* SD card WAV file */
    uint8_t  sd_file_open;
    uint32_t sd_file_size;

    /* BLE/USB streaming */
    uint8_t  stream_packet_count;
} capture_state_t;

static capture_state_t g_cap;

/* WAV header structure (44 bytes) */
typedef struct __attribute__((packed)) {
    char     riff[4];       /* "RIFF" */
    uint32_t riff_size;
    char     wave[4];       /* "WAVE" */
    char     fmt[4];        /* "fmt " */
    uint32_t fmt_size;     /* 16 */
    uint16_t audio_fmt;    /* 1 = PCM */
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data[4];      /* "data" */
    uint32_t data_size;
} wav_header_t;

/* ========================================================================
 *  Initialize capture engine
 * ======================================================================== */

int capture_init(cap_dest_t dest)
{
    memset(&g_cap, 0, sizeof(g_cap));
    g_cap.dest = dest;
    g_cap.sdram_base = (int32_t *)SDRAM_BASE_ADDR;
    g_cap.sdram_capacity = CAPTURE_BUF_SAMPLES;
    return 0;
}

/* ========================================================================
 *  Start capture
 * ======================================================================== */

int capture_start(void)
{
    g_cap.active = 1;
    g_cap.frames_captured = 0;
    g_cap.bytes_written = 0;
    g_cap.sdram_write_offset = 0;

    if (g_cap.dest == CAP_DEST_SD) {
        /* Create WAV file on SD card */
        extern audio_format_t g_format;
        if (sdcard_wav_open("echo_cap.wav", &g_format) == 0) {
            g_cap.sd_file_open = 1;
        }
    }

    return 0;
}

/* ========================================================================
 *  Stop capture
 * ======================================================================== */

void capture_stop(void)
{
    g_cap.active = 0;

    if (g_cap.sd_file_open) {
        sdcard_wav_close();
        g_cap.sd_file_open = 0;
    }
}

/* ========================================================================
 *  Feed audio samples to capture engine
 *
 *  Called from the main loop for each DMA buffer.
 * ======================================================================== */

void capture_feed(const int32_t *buf, uint16_t samples)
{
    if (!g_cap.active)
        return;

    switch (g_cap.dest) {
    case CAP_DEST_SDRAM:
        /* Write to SDRAM ring buffer */
        {
            uint32_t remaining = g_cap.sdram_capacity - g_cap.sdram_write_offset;
            if (remaining >= samples) {
                memcpy(&g_cap.sdram_base[g_cap.sdram_write_offset], buf, samples * sizeof(int32_t));
                g_cap.sdram_write_offset += samples;
            } else {
                /* Wrap around */
                memcpy(&g_cap.sdram_base[g_cap.sdram_write_offset], buf, remaining * sizeof(int32_t));
                uint32_t leftover = samples - remaining;
                memcpy(g_cap.sdram_base, buf + remaining, leftover * sizeof(int32_t));
                g_cap.sdram_write_offset = leftover;
            }
        }
        break;

    case CAP_DEST_SD:
        /* Write to SD card WAV file */
        if (g_cap.sd_file_open) {
            sdcard_wav_write(buf, samples);
        }
        break;

    case CAP_DEST_BLE:
        /* Streaming is handled in main.c stream_audio() */
        break;

    case CAP_DEST_USB:
        /* Streaming over USB CDC handled in main loop */
        if (usb_cdc_connected()) {
            /* Send raw audio samples as binary data */
            usb_cdc_send((const uint8_t *)buf, samples * sizeof(int32_t));
        }
        break;
    }

    g_cap.frames_captured++;
    g_cap.bytes_written += samples * sizeof(int32_t);
}

/* ========================================================================
 *  Get capture statistics
 * ======================================================================== */

uint32_t capture_get_frames(void)
{
    return g_cap.frames_captured;
}

uint32_t capture_get_bytes(void)
{
    return g_cap.bytes_written;
}

/* ========================================================================
 *  Read from SDRAM capture buffer (for download to host)
 * ======================================================================== */

int capture_read_sdram(uint32_t offset, int32_t *buf, uint16_t samples)
{
    if (offset + samples > g_cap.sdram_capacity)
        return -1;

    memcpy(buf, &g_cap.sdram_base[offset], samples * sizeof(int32_t));
    return 0;
}

/* ========================================================================
 *  Get SDRAM buffer fill level (0-100%)
 * ======================================================================== */

uint32_t capture_get_fill_pct(void)
{
    if (g_cap.sdram_capacity == 0)
        return 0;

    uint32_t used = g_cap.sdram_write_offset;
    return (used * 100U) / g_cap.sdram_capacity;
}