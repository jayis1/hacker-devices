/*
 * drivers/frame_capture.c — Frame Capture for Prism-Tap
 *
 * Manages the capture of MIPI frames from the FPGA into the PTF file format
 * on the SD card. Uses ping-pong buffer management: while the FPGA fills one
 * buffer, the MCU reads the other via SPI and writes it to SD.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "frame_capture.h"
#include "fpga_if.h"
#include "sdcard.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Frame buffer in SRAM (one frame at a time) ---- */
static uint8_t frame_buffer[128 * 1024] __attribute__((aligned(32)));

/* ---- Current capture file ---- */
static int current_file_handle = -1;
static char current_filename[32];
static uint32_t frame_counter = 0;
static uint32_t bytes_total = 0;
static uint16_t last_buf_read = 0;
static uint32_t last_frame_time = 0;

/* ---- CRC32 lookup-free implementation (uses hardware CRC if available) ---- */

uint32_t capture_crc32(const uint8_t *data, uint32_t len)
{
    /* Use STM32H7 hardware CRC engine for speed */
    CRC_CR = CRC_CR_RESET;

    /* Process 4 bytes at a time */
    uint32_t i = 0;
    while (i + 4 <= len) {
        uint32_t word = ((uint32_t)data[i]) |
                        ((uint32_t)data[i + 1] << 8) |
                        ((uint32_t)data[i + 2] << 16) |
                        ((uint32_t)data[i + 3] << 24);
        CRC_DR = word;
        i += 4;
    }
    uint32_t crc = CRC_DR;

    /* Handle remaining bytes (1-3) with software CRC */
    while (i < len) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        i++;
    }

    return crc ^ 0xFFFFFFFF;
}

/* ---- PTF header builder ---- */

void capture_build_header(ptf_header_t *hdr, uint32_t frame_idx,
                          uint32_t timestamp, uint16_t w, uint16_t h,
                          uint8_t fmt, uint32_t size)
{
    if (!hdr)
        return;
    memset(hdr, 0, sizeof(ptf_header_t));
    memcpy(hdr->magic, PTF_MAGIC, 16);
    hdr->version = PTF_VERSION;
    hdr->frame_index = frame_idx;
    hdr->timestamp = timestamp;
    hdr->width = w;
    hdr->height = h;
    hdr->format = fmt;
    hdr->frame_size = size;
    hdr->crc32 = 0;  /* filled in after CRC computation */
    hdr->reserved = 0;
}

/* ---- Init ---- */

int capture_init(void)
{
    frame_counter = 0;
    bytes_total = 0;
    last_buf_read = 0;
    last_frame_time = 0;
    return 0;
}

/* ---- Start capture ---- */

int capture_start(const capture_config_t *cfg)
{
    if (!cfg || !cfg->active)
        return -1;

    /* Enable CRC clock */
    RCC_AHB1ENR |= (1U << 0);  /* CRCEN bit */

    /* Create capture directory if needed */
    sdcard_mkdir("/prismtap");

    /* Generate sequential filename */
    uint32_t file_num = 0;
    do {
        /* Simple filename: cap_XXXXX.ptf */
        const char *base = "cap_";
        char num_str[8];
        int pos = 0;

        /* Manual integer to string */
        uint32_t n = file_num;
        char tmp[8];
        int tlen = 0;
        if (n == 0) {
            tmp[tlen++] = '0';
        }
        while (n > 0 && tlen < 7) {
            tmp[tlen++] = '0' + (n % 10);
            n /= 10;
        }
        for (int j = tlen - 1; j >= 0; j--)
            num_str[pos++] = tmp[j];
        num_str[pos] = '\0';

        /* Build path */
        int fi = 0;
        const char *prefix = "/prismtap/cap_";
        while (prefix[fi] && fi < 20) {
            current_filename[fi] = prefix[fi];
            fi++;
        }
        for (int j = 0; j < pos && fi < 28; j++) {
            current_filename[fi++] = num_str[j];
        }
        current_filename[fi++] = '.';
        current_filename[fi++] = 'p';
        current_filename[fi++] = 't';
        current_filename[fi++] = 'f';
        current_filename[fi] = '\0';

        file_num++;
    } while (sdcard_file_exists(current_filename) && file_num < 100000);

    /* Open file for writing */
    current_file_handle = sdcard_open_write(current_filename);
    if (current_file_handle < 0)
        return -2;

    /* Enable FPGA capture */
    fpga_enable_capture(1);

    /* Copy config to global status */
    g_status.capture = *cfg;
    g_status.capture.frames_captured = 0;
    g_status.capture.bytes_written = 0;
    g_status.capture.last_frame_ts = 0;

    frame_counter = 0;
    bytes_total = 0;
    last_buf_read = 0;

    return 0;
}

/* ---- Stop capture ---- */

void capture_stop(void)
{
    fpga_enable_capture(0);

    if (current_file_handle >= 0) {
        sdcard_close(current_file_handle);
        current_file_handle = -1;
    }

    g_status.capture.active = 0;
}

/* ---- Poll for completed frames ---- */

void capture_poll(void)
{
    if (!g_status.capture.active || current_file_handle < 0)
        return;

    /* Check FPGA status for buffer ready */
    uint16_t fpga_status = fpga_get_status();

    uint16_t buf_to_read = 0xFF;
    if ((fpga_status & FPGA_STATUS_BUF_A_FULL) && last_buf_read != 0) {
        buf_to_read = 0;  /* read buffer A */
    } else if ((fpga_status & FPGA_STATUS_BUF_B_FULL) && last_buf_read != 1) {
        buf_to_read = 1;  /* read buffer B */
    } else {
        return;  /* no new frame */
    }

    /* Check capture interval */
    uint32_t now = g_status.uptime_ms;
    if (g_status.capture.interval_ms > 0) {
        if ((now - last_frame_time) < g_status.capture.interval_ms)
            return;  /* skip this frame per interval setting */
    }

    /* Read frame dimensions from FPGA */
    uint16_t width = fpga_read_reg(FPGA_REG_FRAME_W);
    uint16_t height = fpga_read_reg(FPGA_REG_FRAME_H);
    uint16_t fmt = fpga_read_reg(FPGA_REG_FRAME_FMT);

    /* Calculate frame size based on format */
    uint32_t expected_size = 0;
    switch (fmt) {
    case FMT_RAW8:
        expected_size = (uint32_t)width * height;
        break;
    case FMT_RAW10:
        /* RAW10: 4 pixels in 5 bytes */
        expected_size = (uint32_t)(width * height * 10 + 7) / 8;
        break;
    case FMT_RAW12:
        expected_size = (uint32_t)(width * height * 12 + 7) / 8;
        break;
    case FMT_YUV422:
        expected_size = (uint32_t)width * height * 2;
        break;
    case FMT_RGB888:
        expected_size = (uint32_t)width * height * 3;
        break;
    case FMT_JPEG:
        /* Variable size; read up to buffer limit */
        expected_size = sizeof(frame_buffer);
        break;
    default:
        expected_size = sizeof(frame_buffer);
        break;
    }

    if (expected_size > sizeof(frame_buffer))
        expected_size = sizeof(frame_buffer);

    /* Read frame from FPGA via SPI */
    uint32_t actual_bytes = 0;
    if (fpga_read_frame_dma(buf_to_read, frame_buffer, expected_size,
                            &actual_bytes))
        return;

    if (actual_bytes == 0)
        return;

    last_buf_read = buf_to_read;

    /* Optionally JPEG compress (using hardware JPEG on STM32H7) */
    if (g_status.capture.jpeg_compress && fmt != FMT_JPEG) {
        /* In a real implementation, route through H7 hardware JPEG codec.
           For now, mark as uncompressed. */
        /* TODO: jpeg_encode(frame_buffer, ...) */
    }

    /* Build PTF header */
    ptf_header_t hdr;
    capture_build_header(&hdr, frame_counter, now, width, height,
                         (uint8_t)fmt, actual_bytes);

    /* Compute CRC32 of frame payload */
    hdr.crc32 = capture_crc32(frame_buffer, actual_bytes);

    /* Write header to SD card */
    int written = sdcard_write(current_file_handle, (uint8_t *)&hdr,
                               sizeof(ptf_header_t));
    if (written < 0)
        return;

    /* Write frame data to SD card */
    written = sdcard_write(current_file_handle, frame_buffer, actual_bytes);
    if (written < 0)
        return;

    bytes_total += sizeof(ptf_header_t) + actual_bytes;
    frame_counter++;
    last_frame_time = now;

    g_status.capture.frames_captured = frame_counter;
    g_status.capture.bytes_written = bytes_total;
    g_status.capture.last_frame_ts = now;

    /* Check max frames limit */
    if (g_status.capture.max_frames > 0 &&
        frame_counter >= g_status.capture.max_frames) {
        capture_stop();
    }
}

/* ---- Status helpers ---- */

int capture_is_active(void)
{
    return g_status.capture.active;
}

uint32_t capture_get_frame_count(void)
{
    return frame_counter;
}

uint32_t capture_get_bytes_written(void)
{
    return bytes_total;
}

/* ---- End of frame_capture.c ----
 * Author: jayis1
 */