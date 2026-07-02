/*
 * reconstruct.c — compressed-sensing / sparse reconstruction
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * With only 16x16 spatial pixels we cannot image a high-res screen directly.
 * We exploit sparsity: a screen is mostly blank with localized bright regions.
 * We pull the 256 per-pixel |IQ| magnitudes from the FPGA frame buffer, then
 * run a lightweight upsample + sparse-sharpening pass to produce a more
 * visually useful operator preview.
 *
 * The real heavy reconstruction (basis pursuit / OMP over a scan-model
 * dictionary) is done offline in the host toolchain from raw events; the
 * ESP32 only needs a good-enough live preview.
 */
#include <stdint.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"
#include "reconstruct.h"

#define PX 16
#define PY 16
#define N  (PX*PY)

static uint16_t g_raw[N];        /* raw 16x16 mags from FPGA */
static uint16_t g_up[N * 16];    /* upsampled buffer (max 4x -> 64x64) */
static uint8_t  g_upsample = 2;  /* 1,2,4 */
static uint32_t g_words = N;

/* Bilinear upsample an NxN source into (N*f)x(N*f) dest. */
static void bilinear_upsample(const uint16_t *src, uint16_t *dst,
                              uint8_t f)
{
    if (f == 1) { memcpy(dst, src, sizeof(uint16_t) * N); return; }
    for (int y = 0; y < PX * f; y++) {
        int sy0 = y / f, sy1 = (sy0 + 1 < PX) ? sy0 + 1 : sy0;
        int fy  = y % f;
        for (int x = 0; x < PX * f; x++) {
            int sx0 = x / f, sx1 = (sx0 + 1 < PX) ? sx0 + 1 : sx0;
            int fx  = x % f;
            uint32_t a = src[sy0 * PX + sx0];
            uint32_t b = src[sy0 * PX + sx1];
            uint32_t c = src[sy1 * PX + sx0];
            uint32_t d = src[sy1 * PX + sx1];
            uint32_t v = a * (f - fx) * (f - fy)
                       + b * fx * (f - fy)
                       + c * (f - fx) * fy
                       + d * fx * fy;
            dst[y * PX * f + x] = (uint16_t)(v / (f * f));
        }
    }
}

/* Simple sparse sharpen: suppress values below k*mean, boost the rest.
 * This enhances the high-contrast structure operator cares about. */
static void sparse_sharpen(uint16_t *buf, uint32_t n)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < n; i++) sum += buf[i];
    uint32_t mean = sum / n;
    uint32_t thr = mean + (mean >> 1);   /* 1.5 * mean */
    for (uint32_t i = 0; i < n; i++) {
        if (buf[i] < thr) buf[i] = buf[i] >> 2;   /* attenuate background */
        else {
            uint32_t v = buf[i] + (buf[i] >> 1);  /* boost signal */
            if (v > 0xFFFF) v = 0xFFFF;
            buf[i] = (uint16_t)v;
        }
    }
}

int reconstruct_init(void)
{
    memset(g_raw, 0, sizeof(g_raw));
    memset(g_up,  0, sizeof(g_up));
    g_upsample = 2;
    g_words = N * g_upsample * g_upsample;
    return 0;
}

void reconstruct_set_upsample(uint8_t f)
{
    if (f != 1 && f != 2 && f != 4) f = 2;
    g_upsample = f;
    g_words = N * f * f;
}

void reconstruct_pull_frame(void)
{
    /* Auto-increment read of 256 IQ magnitudes. */
    fpga_write(REG_IQ_AUTOIDX, 0);
    for (int i = 0; i < N; i++) {
        uint16_t i_acc = fpga_read(REG_IQ_AUTO_I);
        uint16_t q_acc = fpga_read(REG_IQ_AUTO_Q);
        /* magnitude approx: |z| ~ max(|I|,|Q|) + 0.5*min(|I|,|Q|) (alpha-max) */
        uint16_t mx = i_acc > q_acc ? i_acc : q_acc;
        uint16_t mn = i_acc > q_acc ? q_acc : i_acc;
        uint16_t mag = mx + (mn >> 1);
        g_raw[i] = mag;
    }
    /* Upsample + sharpen into the live preview buffer. */
    bilinear_upsample(g_raw, g_up, g_upsample);
    sparse_sharpen(g_up, g_words);
}

const uint16_t *reconstruct_frame_buffer(void) { return g_up; }
uint32_t reconstruct_frame_words(void) { return g_words; }