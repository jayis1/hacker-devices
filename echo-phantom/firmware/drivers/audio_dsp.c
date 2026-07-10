/*
 * audio_dsp.c — Real-Time Audio DSP Pipeline for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements a lightweight CMSIS-DSP-compatible audio processing pipeline:
 *   - FIR filter (low-pass, high-pass, band-pass)
 *   - Audio mixing (for injection overlay)
 *   - Gain control
 *   - Noise injection
 *   - Simple format conversion (bit-depth, channel)
 *
 * All processing is done in-place on 32-bit audio samples.
 * The pipeline operates on one DMA buffer (64 samples) at a time,
 * well within the 20µs frame budget at 48kHz.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* Forward declarations for sinf/cosf (defined at bottom of file) */
float sinf(float x);
float cosf(float x);

/* ========================================================================
 *  DSP state
 * ======================================================================== */

#define MAX_FILTER_TAPS  64

typedef struct {
    uint8_t  enabled;
    uint8_t  type;          /* 0=none, 1=LP, 2=HP, 3=BP, 4=FIR custom */
    uint8_t  n_taps;
    uint8_t  gain;          /* 0-200 percent */

    int32_t  coeffs[MAX_FILTER_TAPS];  /* Q15 fixed-point coefficients */
    int32_t  delay_line[MAX_FILTER_TAPS]; /* Filter delay line */
    uint32_t dl_index;

    uint8_t  noise_enabled;
    uint8_t  noise_level;   /* 0-100, amount of white noise to add */
    uint32_t lfsr;          /* 32-bit LFSR for pseudo-random noise */
} dsp_state_t;

static dsp_state_t g_dsp;

/* ========================================================================
 *  Initialize DSP pipeline
 * ======================================================================== */

void audio_dsp_init(void)
{
    memset(&g_dsp, 0, sizeof(g_dsp));
    g_dsp.gain = 100;  /* 100% = unity gain */
    g_dsp.lfsr = 0xACE1u;  /* Seed for LFSR noise generator */

    /* Default: no filter, unity gain */
    g_dsp.enabled = 0;
    g_dsp.type = 0;
    g_dsp.n_taps = 0;
    g_dsp.noise_enabled = 0;
    g_dsp.noise_level = 0;
}

/* ========================================================================
 *  Set filter coefficients
 * ======================================================================== */

void audio_dsp_set_filter(uint8_t type, const int32_t *coeffs, uint8_t n)
{
    if (n > MAX_FILTER_TAPS)
        n = MAX_FILTER_TAPS;

    g_dsp.type = type;
    g_dsp.n_taps = n;

    if (type == 1) {
        /* Low-pass: simple moving average if no custom coefficients */
        if (n == 0 || coeffs == NULL) {
            /* Default 8-tap moving average LP filter */
            g_dsp.n_taps = 8;
            for (int i = 0; i < 8; i++)
                g_dsp.coeffs[i] = 1 << 12;  /* Q15: 1/8 ≈ 4096 */
        } else {
            memcpy(g_dsp.coeffs, coeffs, n * sizeof(int32_t));
        }
        g_dsp.enabled = 1;
    } else if (type == 2) {
        /* High-pass: default simple HP (diff) */
        if (n == 0 || coeffs == NULL) {
            g_dsp.n_taps = 2;
            g_dsp.coeffs[0] = 1 << 15;   /* +1 */
            g_dsp.coeffs[1] = -(1 << 15); /* -1 */
        } else {
            memcpy(g_dsp.coeffs, coeffs, n * sizeof(int32_t));
        }
        g_dsp.enabled = 1;
    } else if (type == 3) {
        /* Band-pass: copy provided coefficients */
        if (n > 0 && coeffs) {
            memcpy(g_dsp.coeffs, coeffs, n * sizeof(int32_t));
            g_dsp.enabled = 1;
        }
    } else if (type == 4) {
        /* Custom FIR filter */
        if (n > 0 && coeffs) {
            memcpy(g_dsp.coeffs, coeffs, n * sizeof(int32_t));
            g_dsp.enabled = 1;
        }
    } else {
        /* Disable filter */
        g_dsp.enabled = 0;
        g_dsp.type = 0;
        g_dsp.n_taps = 0;
    }

    /* Reset delay line */
    memset(g_dsp.delay_line, 0, sizeof(g_dsp.delay_line));
    g_dsp.dl_index = 0;
}

/* ========================================================================
 *  FIR filter processing (one sample at a time)
 *
 *  y[n] = Σ h[k] × x[n-k]
 *
 *  Coefficients are in Q15 fixed-point format.
 *  Input/output are 32-bit signed integers (full-scale ±2^31).
 * ======================================================================== */

static int32_t fir_filter_one(int32_t sample)
{
    if (!g_dsp.enabled || g_dsp.n_taps == 0)
        return sample;

    /* Insert sample into delay line */
    g_dsp.delay_line[g_dsp.dl_index] = sample;
    g_dsp.dl_index = (g_dsp.dl_index + 1) % g_dsp.n_taps;

    /* Compute FIR output */
    int64_t acc = 0;
    uint32_t idx = g_dsp.dl_index;

    for (uint8_t i = 0; i < g_dsp.n_taps; i++) {
        if (idx == 0)
            idx = g_dsp.n_taps - 1;
        else
            idx--;
        acc += (int64_t)g_dsp.delay_line[idx] * g_dsp.coeffs[i];
    }

    /* Scale by Q15 (shift right by 15) */
    int64_t result = acc >> 15;

    /* Clip to 32-bit range */
    if (result > 0x7FFFFFFFLL) result = 0x7FFFFFFFLL;
    if (result < -0x80000000LL) result = -0x80000000LL;

    return (int32_t)result;
}

/* ========================================================================
 *  Pseudo-random noise generation (32-bit LFSR)
 * ======================================================================== */

static int32_t generate_noise(void)
{
    /* Galois LFSR with maximal-length polynomial */
    uint32_t lsb = g_dsp.lfsr & 1;
    g_dsp.lfsr >>= 1;
    if (lsb)
        g_dsp.lfsr ^= 0xB4001293U;

    /* Convert to signed 32-bit noise */
    return (int32_t)g_dsp.lfsr;
}

/* ========================================================================
 *  Process audio buffer (in-place)
 * ======================================================================== */

void audio_dsp_process(int32_t *buf, uint16_t samples)
{
    for (uint16_t i = 0; i < samples; i++) {
        int32_t sample = buf[i];

        /* Apply FIR filter if enabled */
        if (g_dsp.enabled && g_dsp.n_taps > 0) {
            sample = fir_filter_one(sample);
        }

        /* Apply gain */
        if (g_dsp.gain != 100) {
            int64_t scaled = (int64_t)sample * g_dsp.gain / 100;
            if (scaled > 0x7FFFFFFFLL) scaled = 0x7FFFFFFFLL;
            if (scaled < -0x80000000LL) scaled = -0x80000000LL;
            sample = (int32_t)scaled;
        }

        /* Add noise if enabled */
        if (g_dsp.noise_enabled && g_dsp.noise_level > 0) {
            int32_t noise = generate_noise();
            /* Scale noise by noise_level (0-100) */
            int64_t scaled_noise = (int64_t)noise * g_dsp.noise_level / 100;
            int64_t mixed = (int64_t)sample + scaled_noise;
            if (mixed > 0x7FFFFFFFLL) mixed = 0x7FFFFFFFLL;
            if (mixed < -0x80000000LL) mixed = -0x80000000LL;
            sample = (int32_t)mixed;
        }

        buf[i] = sample;
    }
}

/* ========================================================================
 *  Mix two audio buffers (for injection overlay mode)
 *
 *  dst = dst × (gain/100) + src × (1 - gain/100)
 *  This is used when mixing injected audio with the original mic signal.
 * ======================================================================== */

void audio_dsp_mix(int32_t *dst, const int32_t *src, uint16_t samples, uint8_t gain)
{
    for (uint16_t i = 0; i < samples; i++) {
        int64_t mixed = (int64_t)src[i] * gain / 100;
        /* dst already contains the processed original audio */
        int64_t result = mixed;  /* Just use the inject clip at gain */
        if (result > 0x7FFFFFFFLL) result = 0x7FFFFFFFLL;
        if (result < -0x80000000LL) result = -0x80000000LL;
        dst[i] = (int32_t)result;
    }
}

/* ========================================================================
 *  Simple low-pass filter coefficient generator
 *
 *  Generates a windowed-sinc FIR low-pass filter with the given
 *  normalized cutoff frequency (0.0–0.5, where 0.5 = Nyquist).
 * ======================================================================== */

static int32_t Q15(float val)
{
    float scaled = val * 32768.0f;
    if (scaled > 32767.0f) scaled = 32767.0f;
    if (scaled < -32768.0f) scaled = -32768.0f;
    return (int32_t)scaled;
}

void audio_dsp_generate_lp_coeffs(uint8_t n_taps, float cutoff, int32_t *coeffs)
{
    if (n_taps == 0 || n_taps > MAX_FILTER_TAPS || coeffs == NULL)
        return;

    /* Hamming window low-pass FIR */
    int M = n_taps - 1;
    float sum = 0.0f;

    for (int i = 0; i < n_taps; i++) {
        int n = i - M / 2;

        /* Sinc function */
        float h;
        if (n == 0) {
            h = 2.0f * cutoff;
        } else {
            h = sinf(2.0f * 3.14159265f * cutoff * n) / (3.14159265f * n);
        }

        /* Hamming window */
        float w = 0.54f - 0.46f * cosf(2.0f * 3.14159265f * i / M);

        coeffs[i] = Q15(h * w);
        sum += h * w;
    }

    /* Normalize coefficients */
    if (sum != 0.0f) {
        for (int i = 0; i < n_taps; i++) {
            float val = (float)coeffs[i] / 32768.0f / sum;
            coeffs[i] = Q15(val);
        }
    }
}

/* ========================================================================
 *  Set gain (0-200%)
 * ======================================================================== */

void audio_dsp_set_gain(uint8_t gain_pct)
{
    if (gain_pct > 200)
        gain_pct = 200;
    g_dsp.gain = gain_pct;
}

/* ========================================================================
 *  Enable/disable noise injection
 * ======================================================================== */

void audio_dsp_set_noise(uint8_t enabled, uint8_t level)
{
    g_dsp.noise_enabled = enabled ? 1 : 0;
    if (level > 100)
        level = 100;
    g_dsp.noise_level = level;
}

/* ========================================================================
 *  Simple sinf/cosf implementations (avoiding libc for embedded)
 * ======================================================================== */

float sinf(float x)
{
    /* Reduce to [-π, π] */
    while (x > 3.14159265f) x -= 2.0f * 3.14159265f;
    while (x < -3.14159265f) x += 2.0f * 3.14159265f;

    /* Taylor series approximation (7 terms) */
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    float x9 = x7 * x2;

    return x - x3 / 6.0f + x5 / 120.0f - x7 / 5040.0f + x9 / 362880.0f;
}

float cosf(float x)
{
    return sinf(x + 3.14159265f / 2.0f);
}