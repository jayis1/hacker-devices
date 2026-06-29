/*
 * power_profiler.c — Real-time Qi power-side fingerprinting for CoilGrip
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Samples the downstream current (INA240 + ADS131M04) and the coil-voltage
 * envelope (ADL5511) at up to 256 kS/s, then runs a sliding-window FFT and
 * an envelope/feature extractor to classify the target device's state.
 *
 * The classifier is a tiny template-matcher: it correlates the latest
 * feature vector (envelope mean, envelope stddev, FFT bin energies in
 * 8 bands, dominant-bin index) against a small on-board database of
 * fingerprints loaded from the SD card. The match with the highest
 * normalized dot-product above a threshold wins.
 *
 * This is intentionally simple and deterministic so it runs on the
 * Cortex-M7 without external compute; the goal is "did the screen turn
 * on?" and "is this phone X?" rather than full ML.
 */

#include "board.h"
#include "registers.h"
#include <math.h>

#define PROFILER_FS_HZ         256000U
#define FFT_N                  256U
#define FEATURE_BANDS          8U
#define MAX_FINGERPRINTS       32U
#define MATCH_THRESHOLD        0.72f    /* cosine similarity cutoff */

typedef struct {
    char     name[24];
    float    feat[FEATURE_BANDS + 3];  /* 8 bands + mean + std + dom_bin */
} fingerprint_t;

static fingerprint_t g_fp_db[MAX_FINGERPRINTS];
static uint8_t g_fp_count = 0;

/* ADC ring buffer (one window of FFT_N samples, int16) */
static int16_t  g_current_buf[FFT_N];
static int16_t  g_voltage_buf[FFT_N];
static volatile uint16_t g_buf_idx = 0;
static volatile bool g_window_ready = false;
static volatile bool g_running = false;

/* Latest classification result */
static uint8_t  g_last_state = 0;
static float     g_last_conf  = 0.0f;

/* ---- Fixed-point FFT (radix-2, in place, int16 Q15) --------------------- */
/* Reuses a small textbook implementation; good enough for band-energy
 * features. We zero-pad to 256. */

static void fft256_q15(int16_t *real, int16_t *imag)
{
    /* Bit reversal */
    for (uint16_t i = 1, j = 0; i < FFT_N; i++) {
        uint16_t bit = FFT_N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            int16_t tr = real[i]; real[i] = real[j]; real[j] = tr;
            int16_t ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
    }
    /* Butterflies */
    for (uint16_t len = 2; len <= FFT_N; len <<= 1) {
        uint16_t half = len >> 1;
        /* phase step = -2π/len; use lookup of cos/sin in Q15 */
        for (uint16_t k = 0; k < half; k++) {
            int16_t wr = (int16_t)(16384.0f * cosf(-2.0f * 3.14159265f * (float)k / (float)len));
            int16_t wi = (int16_t)(16384.0f * sinf(-2.0f * 3.14159265f * (float)k / (float)len));
            for (uint16_t i = k; i < FFT_N; i += len) {
                uint16_t j = i + half;
                int32_t tr = ((int32_t)real[j] * wr - (int32_t)imag[j] * wi) >> 14;
                int32_t ti = ((int32_t)real[j] * wi + (int32_t)imag[j] * wr) >> 14;
                real[j] = (int16_t)(real[i] - tr);
                imag[j] = (int16_t)(imag[i] - ti);
                real[i] = (int16_t)(real[i] + tr);
                imag[i] = (int16_t)(imag[i] + ti);
            }
        }
    }
}

/* ---- ADC sampling (polling ADC1 channel 4 = current, ch5 = voltage) ---- */

static int16_t adc1_read_chan(uint8_t chan)
{
    /* Configure channel in SQR registers (regular sequence).
     * SQR1 (offset 0x30): L=0 (1 conv), SQ1 = chan. */
    REG32((uint32_t)ADC1 + 0x30) = (0U << 20) | (chan << 6);
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & BIT(2))) { }  /* ADRDY? use EOC=bit2 actually */
    int16_t v = (int16_t)(ADC1->DR & 0xFFFF);
    return v;
}

/* Called from the TIM2 ISR at PROFILER_FS_HZ; we keep it simple here and
 * poll inside profiler_get_window() instead for the reference build. */
static void profiler_sample_tick(void)
{
    if (!g_running) return;
    int16_t i = adc1_read_chan(4);
    int16_t v = adc1_read_chan(5);
    g_current_buf[g_buf_idx] = i;
    g_voltage_buf[g_buf_idx] = v;
    g_buf_idx++;
    if (g_buf_idx >= FFT_N) {
        g_buf_idx = 0;
        g_window_ready = true;
    }
}

/* ---- Feature extraction ------------------------------------------------- */

static void extract_features(const int16_t *buf, float *feat_out)
{
    /* Mean & std */
    int32_t sum = 0;
    for (uint16_t i = 0; i < FFT_N; i++) sum += buf[i];
    float mean = (float)sum / (float)FFT_N;
    float acc = 0.0f;
    for (uint16_t i = 0; i < FFT_N; i++) {
        float d = (float)buf[i] - mean;
        acc += d * d;
    }
    float std = sqrtf(acc / (float)FFT_N);

    /* FFT */
    int16_t real[FFT_N], imag[FFT_N];
    for (uint16_t i = 0; i < FFT_N; i++) { real[i] = buf[i]; imag[i] = 0; }
    fft256_q15(real, imag);

    /* Magnitude in 8 log-spaced bands (bins 1..127) */
    uint16_t band_lo[FEATURE_BANDS] = {1, 2, 3, 5, 8, 13, 21, 34};
    uint16_t band_hi[FEATURE_BANDS] = {1, 2, 4, 7, 12, 20, 33, 127};
    float bands[FEATURE_BANDS];
    float dom_energy = 0.0f;
    int   dom_bin = 0;
    for (uint8_t b = 0; b < FEATURE_BANDS; b++) {
        float e = 0.0f;
        for (uint16_t k = band_lo[b]; k <= band_hi[b]; k++) {
            float mag = sqrtf((float)real[k] * real[k] + (float)imag[k] * imag[k]);
            e += mag;
            if (mag > dom_energy) { dom_energy = mag; dom_bin = k; }
        }
        bands[b] = e / (float)(band_hi[b] - band_lo[b] + 1);
    }

    /* Normalize bands by std to make the feature scale-invariant */
    float norm = (std > 1.0f) ? 1.0f / std : 1.0f;
    for (uint8_t b = 0; b < FEATURE_BANDS; b++) feat_out[b] = bands[b] * norm;
    feat_out[FEATURE_BANDS + 0] = mean * norm;
    feat_out[FEATURE_BANDS + 1] = std;
    feat_out[FEATURE_BANDS + 2] = (float)dom_bin;
}

/* ---- Cosine similarity ------------------------------------------------- */

static float cosine(const float *a, const float *b, uint8_t n)
{
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (uint8_t i = 0; i < n; i++) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    if (na < 1e-9f || nb < 1e-9f) return 0.0f;
    return dot / (sqrtf(na) * sqrtf(nb));
}

/* ---- Public API -------------------------------------------------------- */

int profiler_start(uint32_t sample_rate_hz)
{
    (void)sample_rate_hz;  /* fixed at PROFILER_FS_HZ for now */
    g_buf_idx = 0;
    g_window_ready = false;
    g_running = true;
    /* In a full build we'd configure TIM2 to fire an ISR at sample_rate_hz
     * and trigger the ADC. For the reference design we sample on demand. */
    return 0;
}

void profiler_stop(void)
{
    g_running = false;
}

int profiler_get_sample(int32_t *current_ua, int32_t *voltage_mv)
{
    if (!g_running) return -1;
    int16_t i = adc1_read_chan(4);
    int16_t v = adc1_read_chan(5);
    /* INA240 gain 50 V/V across 10 mΩ shunt → 1 LSB ≈ (3.3/4096)/(50*0.01) A
     *                                          = 0.000161 mA = 161 µA      */
    if (current_ua) *current_ua = (int32_t)i * 161;
    /* ADL5511 envelope: 0–5 V → 0–20 V coil envelope, 1 LSB ≈ 4.88 mV */
    if (voltage_mv)  *voltage_mv = (int32_t)v * 5;
    return 0;
}

int profiler_classify_state(uint8_t *state_out, float *confidence)
{
    if (!g_running) return -1;
    /* Fill the buffer (blocking) so we always have a fresh window */
    for (uint16_t i = 0; i < FFT_N; i++) {
        g_current_buf[i] = adc1_read_chan(4);
        g_voltage_buf[i] = adc1_read_chan(5);
    }

    float feat[FEATURE_BANDS + 3];
    extract_features(g_current_buf, feat);

    /* Match against DB */
    float best = -1.0f;
    int   best_idx = -1;
    for (uint8_t i = 0; i < g_fp_count; i++) {
        float sim = cosine(feat, g_fp_db[i].feat, FEATURE_BANDS + 3);
        if (sim > best) { best = sim; best_idx = i; }
    }
    if (best >= MATCH_THRESHOLD && best_idx >= 0) {
        *state_out = (uint8_t)best_idx;
        *confidence = best;
        g_last_state = (uint8_t)best_idx;
        g_last_conf = best;
        return 0;
    }
    *state_out = 0xFF;  /* unknown */
    *confidence = best;
    g_last_state = 0xFF;
    g_last_conf = best;
    return 1;
}

int profiler_add_fingerprint(const char *name, const uint8_t *sig, uint16_t len)
{
    if (g_fp_count >= MAX_FINGERPRINTS) return -1;
    if (len != sizeof(fingerprint_t) - 24) return -1;  /* expect raw feat bytes */
    fingerprint_t *fp = &g_fp_db[g_fp_count];
    /* Copy name (truncate to 23) */
    uint8_t i = 0;
    for (; i < 23 && name[i]; i++) fp->name[i] = name[i];
    fp->name[i] = 0;
    /* Copy features as floats from raw bytes */
    const float *f = (const float *)sig;
    for (uint8_t k = 0; k < FEATURE_BANDS + 3; k++)
        fp->feat[k] = f[k];
    g_fp_count++;
    return 0;
}

int profiler_match_fingerprint(const uint8_t *sig, uint16_t len,
                               char *name_out, uint16_t name_sz)
{
    if (len != sizeof(fingerprint_t) - 24) return -1;
    const float *qf = (const float *)sig;
    float best = -1.0f; int best_idx = -1;
    for (uint8_t i = 0; i < g_fp_count; i++) {
        float sim = cosine(qf, g_fp_db[i].feat, FEATURE_BANDS + 3);
        if (sim > best) { best = sim; best_idx = i; }
    }
    if (best >= MATCH_THRESHOLD && best_idx >= 0) {
        uint16_t n = 0;
        for (; n < name_sz - 1 && g_fp_db[best_idx].name[n]; n++)
            name_out[n] = g_fp_db[best_idx].name[n];
        name_out[n] = 0;
        return 0;
    }
    return 1;
}

uint8_t profiler_get_fp_count(void) { return g_fp_count; }
uint8_t profiler_get_last_state(void) { return g_last_state; }
float   profiler_get_last_conf(void)  { return g_last_conf; }