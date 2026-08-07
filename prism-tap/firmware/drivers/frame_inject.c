/*
 * drivers/frame_inject.c — Frame Injection for Prism-Tap
 *
 * Controls the frame injection engine in the FPGA. Pre-loads a frame into
 * FPGA Block RAM, then enables injection to replace, overlay, or selectively
 * modify frames passing through the tap.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "frame_inject.h"
#include "fpga_if.h"
#include "board.h"
#include <string.h>

/* ---- Injection frame buffer in SRAM ---- */
static uint8_t inject_frame[256 * 1024] __attribute__((aligned(32)));
static uint32_t inject_frame_len = 0;
static uint16_t inject_frame_w = 0;
static uint16_t inject_frame_h = 0;
static uint8_t  inject_frame_fmt = 0;
static uint32_t inject_count = 0;

/* ---- Simple PRNG for pattern generation ---- */
static uint32_t prng_state = 0xDEADBEEF;

static uint32_t prng_next(void)
{
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 17;
    prng_state ^= prng_state << 5;
    return prng_state;
}

/* ---- Init ---- */

int inject_init(void)
{
    inject_frame_len = 0;
    inject_frame_w = 0;
    inject_frame_h = 0;
    inject_frame_fmt = 0;
    inject_count = 0;
    return 0;
}

/* ---- Load a frame for injection ---- */

int inject_load_frame(const uint8_t *data, uint32_t length,
                       uint16_t width, uint16_t height, uint8_t format)
{
    if (!data || length == 0 || length > sizeof(inject_frame))
        return -1;

    /* Copy frame to local buffer */
    memcpy(inject_frame, data, length);
    inject_frame_len = length;
    inject_frame_w = width;
    inject_frame_h = height;
    inject_frame_fmt = format;

    /* Upload to FPGA */
    int ret = fpga_upload_inject_frame(inject_frame, length,
                                       width, height, format);
    if (ret)
        return -2;

    return 0;
}

/* ---- Start injection ---- */

int inject_start(const inject_config_t *cfg)
{
    if (!cfg || inject_frame_len == 0)
        return -1;

    /* Set injection mode in FPGA */
    fpga_write_reg(FPGA_REG_INJECT_MODE, (uint16_t)cfg->mode);

    /* Set overlay position if overlay mode */
    if (cfg->mode == INJECT_OVERLAY) {
        fpga_write_reg(FPGA_REG_OVERLAY_X, cfg->overlay_x);
        fpga_write_reg(FPGA_REG_OVERLAY_Y, cfg->overlay_y);
        fpga_write_reg(FPGA_REG_OVERLAY_W, cfg->overlay_w);
        fpga_write_reg(FPGA_REG_OVERLAY_H, cfg->overlay_h);
    }

    /* Set trigger interval for selective mode */
    if (cfg->mode == INJECT_SELECTIVE) {
        fpga_write_reg(FPGA_REG_INJECT_CTRL, cfg->trigger_interval);
    }

    /* Enable injection in FPGA */
    fpga_enable_inject(1);

    g_status.inject = *cfg;
    g_status.inject.active = 1;
    g_status.inject.frame_count = 0;
    inject_count = 0;

    /* Turn on inject LED */
    GPIO_RESET(LED_INJECT_PORT, LED_INJECT_PIN);

    return 0;
}

/* ---- Stop injection ---- */

void inject_stop(void)
{
    fpga_enable_inject(0);

    g_status.inject.active = 0;

    /* Turn off inject LED */
    GPIO_SET(LED_INJECT_PORT, LED_INJECT_PIN);
}

int inject_is_active(void)
{
    return g_status.inject.active;
}

uint32_t inject_get_count(void)
{
    /* Read frame count from FPGA status */
    uint16_t fpga_status = fpga_get_status();
    if (fpga_status & FPGA_STATUS_INJECT_ACTIVE) {
        inject_count = fpga_get_frame_index();
        g_status.inject.frame_count = inject_count;
    }
    return inject_count;
}

/* ---- Generate a solid-color test frame ---- */

int inject_generate_test_frame(uint16_t width, uint16_t height, uint8_t format,
                                 uint8_t r, uint8_t g, uint8_t b,
                                 uint8_t *out_buf, uint32_t buf_size,
                                 uint32_t *out_len)
{
    if (!out_buf || !out_len)
        return -1;

    uint32_t needed = 0;
    switch (format) {
    case FMT_RAW8:
        /* Bayer pattern: approximate color with interleaved values */
        needed = (uint32_t)width * height;
        if (needed > buf_size)
            return -2;
        for (uint32_t i = 0; i < needed; i++) {
            uint16_t x = i % width;
            uint16_t y = i / width;
            /* Simple Bayer mosaic approximation */
            if ((x % 2 == 0) && (y % 2 == 0))
                out_buf[i] = r;  /* R site */
            else if ((x % 2 == 1) && (y % 2 == 1))
                out_buf[i] = b;  /* B site */
            else
                out_buf[i] = g;  /* G site */
        }
        break;
    case FMT_YUV422:
        needed = (uint32_t)width * height * 2;
        if (needed > buf_size)
            return -2;
        /* Convert RGB to YUV422 (UYVY format) */
        {
            uint8_t Y = (uint8_t)((0.299f * r + 0.587f * g + 0.114f * b));
            uint8_t U = (uint8_t)(128 - 0.169f * r - 0.331f * g + 0.5f * b);
            uint8_t V = (uint8_t)(128 + 0.5f * r - 0.419f * g - 0.081f * b);
            for (uint32_t i = 0; i < needed; i += 4) {
                out_buf[i] = U;
                out_buf[i + 1] = Y;
                out_buf[i + 2] = V;
                out_buf[i + 3] = Y;
            }
        }
        break;
    case FMT_RGB888:
        needed = (uint32_t)width * height * 3;
        if (needed > buf_size)
            return -2;
        for (uint32_t i = 0; i < needed; i += 3) {
            out_buf[i] = r;
            out_buf[i + 1] = g;
            out_buf[i + 2] = b;
        }
        break;
    case FMT_RAW10:
        /* RAW10: 4 pixels packed in 5 bytes */
        needed = (uint32_t)(width * height * 10 + 7) / 8;
        if (needed > buf_size)
            return -2;
        for (uint32_t i = 0; i < needed; i++)
            out_buf[i] = (uint8_t)(prng_next() & 0xFF);  /* approximate */
        break;
    default:
        return -3;
    }

    *out_len = needed;
    return 0;
}

/* ---- Generate a pattern frame ---- */

int inject_generate_pattern(uint16_t width, uint16_t height, uint8_t format,
                             uint8_t pattern_type,
                             uint8_t *out_buf, uint32_t buf_size,
                             uint32_t *out_len)
{
    if (!out_buf || !out_len)
        return -1;

    uint32_t pixel_count = (uint32_t)width * height;
    uint32_t bytes_per_pixel = (format == FMT_RGB888) ? 3 :
                               (format == FMT_YUV422) ? 2 : 1;
    uint32_t needed = pixel_count * bytes_per_pixel;
    if (needed > buf_size)
        return -2;

    switch (pattern_type) {
    case PATTERN_GRADIENT:
        for (uint32_t i = 0; i < pixel_count; i++) {
            uint16_t x = i % width;
            uint16_t y = i / width;
            uint8_t val = (uint8_t)(((x + y) * 255) / (width + height - 2));
            if (format == FMT_RGB888) {
                out_buf[i * 3] = val;
                out_buf[i * 3 + 1] = (uint8_t)(255 - val);
                out_buf[i * 3 + 2] = (uint8_t)(val * 3 / 4);
            } else if (format == FMT_YUV422) {
                /* Simplified: fill Y only for gradient */
                if ((i & 1) == 0)
                    out_buf[i * 2] = 128;
                out_buf[i * 2 + 1] = val;
            } else {
                out_buf[i] = val;
            }
        }
        break;

    case PATTERN_NOISE:
        for (uint32_t i = 0; i < needed; i++)
            out_buf[i] = (uint8_t)(prng_next() & 0xFF);
        break;

    case PATTERN_CHECKER: {
        uint8_t block_size = MAX(width / 16, 8);
        for (uint32_t i = 0; i < pixel_count; i++) {
            uint16_t x = i % width;
            uint16_t y = i / width;
            uint8_t bx = x / block_size;
            uint8_t by = y / block_size;
            uint8_t val = ((bx + by) & 1) ? 255 : 0;
            if (format == FMT_RGB888) {
                out_buf[i * 3] = val;
                out_buf[i * 3 + 1] = val;
                out_buf[i * 3 + 2] = val;
            } else if (format == FMT_YUV422) {
                if ((i & 1) == 0)
                    out_buf[i * 2] = 128;
                out_buf[i * 2 + 1] = val;
            } else {
                out_buf[i] = val;
            }
        }
        break;
    }

    case PATTERN_BARS: {
        /* SMPTE-style color bars (simplified) */
        uint8_t bars[8][3] = {
            {255, 255, 255},  /* white  */
            {255, 255, 0},    /* yellow */
            {0, 255, 255},    /* cyan   */
            {0, 255, 0},      /* green  */
            {255, 0, 255},    /* magenta */
            {255, 0, 0},      /* red    */
            {0, 0, 255},      /* blue   */
            {0, 0, 0},        /* black  */
        };
        uint16_t bar_width = width / 8;
        for (uint32_t i = 0; i < pixel_count; i++) {
            uint16_t x = i % width;
            uint8_t bar_idx = (uint8_t)(x / bar_width);
            if (bar_idx > 7) bar_idx = 7;
            if (format == FMT_RGB888) {
                out_buf[i * 3] = bars[bar_idx][0];
                out_buf[i * 3 + 1] = bars[bar_idx][1];
                out_buf[i * 3 + 2] = bars[bar_idx][2];
            } else if (format == FMT_YUV422) {
                uint8_t Y = (uint8_t)((0.299f * bars[bar_idx][0] +
                                       0.587f * bars[bar_idx][1] +
                                       0.114f * bars[bar_idx][2]));
                if ((i & 1) == 0)
                    out_buf[i * 2] = 128;
                out_buf[i * 2 + 1] = Y;
            } else {
                out_buf[i] = (uint8_t)((bars[bar_idx][0] + bars[bar_idx][1] +
                                        bars[bar_idx][2]) / 3);
            }
        }
        break;
    }

    default:
        return -3;
    }

    *out_len = needed;
    return 0;
}

/* ---- End of frame_inject.c ----
 * Author: jayis1
 */