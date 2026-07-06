/*
 * lumen-tap/firmware/drivers/dsp.c
 * DSP pipeline implementation for LumenTap.
 *
 * Pipeline: DC block → bandpass (biquad cascade) → demod (AM or FM) →
 *           noise suppression → AGC → decimate 4x → output
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#include "dsp.h"
#include "../board.h"
#include "../registers.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------ */
/*  Filter coefficient generation                                     */
/* ------------------------------------------------------------------ */

/* Biquad coefficients stored as [b0, b1, b2, a1, a2] per stage
 * (normalized a0 = 1). We implement direct-form-II transposed.
 */
typedef struct {
    float b0, b1, b2, a1, a2;
} biquad_t;

static biquad_t g_bq[DSP_BIQUAD_STAGES];

/* Generate one second-order low-pass biquad (for the high half of the BP). */
static void biquad_lp(biquad_t *b, float fc, float fs, float Q)
{
    float w0 = 2.0f * (float)M_PI * fc / fs;
    float c  = cosf(w0);
    float s  = sinf(w0);
    float alpha = s / (2.0f * Q);
    float a0 = 1.0f + alpha;
    b->b0 = ((1.0f - c) / 2.0f) / a0;
    b->b1 = (1.0f - c) / a0;
    b->b2 = ((1.0f - c) / 2.0f) / a0;
    b->a1 = (-2.0f * c) / a0;
    b->a2 = (1.0f - alpha) / a0;
}

/* Generate one second-order high-pass biquad (low half of BP). */
static void biquad_hp(biquad_t *b, float fc, float fs, float Q)
{
    float w0 = 2.0f * (float)M_PI * fc / fs;
    float c  = cosf(w0);
    float s  = sinf(w0);
    float alpha = s / (2.0f * Q);
    float a0 = 1.0f + alpha;
    b->b0 = ((1.0f + c) / 2.0f) / a0;
    b->b1 = -(1.0f + c) / a0;
    b->b2 = ((1.0f + c) / 2.0f) / a0;
    b->a1 = (-2.0f * c) / a0;
    b->a2 = (1.0f - alpha) / a0;
}

/* Apply the 4-stage biquad cascade in place. */
static void biquad_cascade_run(float *x, int n)
{
    for (int s = 0; s < DSP_BIQUAD_STAGES; ++s) {
        float w1 = g_bq[s].a1;  /* reuse as -a1, -a2 from DF-II */
        float w2 = g_bq[s].a2;
        float z1 = 0.0f, z2 = 0.0f;
        for (int i = 0; i < n; ++i) {
            float in = x[i];
            float w = in - w1 * z1 - w2 * z2;
            float out = g_bq[s].b0 * w + g_bq[s].b1 * z1 + g_bq[s].b2 * z2;
            z2 = z1; z1 = w;
            x[i] = out;
        }
        (void)w1; (void)w2;
    }
}

/* ------------------------------------------------------------------ */
/*  DC blocker — 31-tap high-pass FIR at very low cutoff (~5 Hz)      */
/* ------------------------------------------------------------------ */
static float g_dc_coeffs[DSP_DC_TAPS];

static void dc_blocker_design(float fs)
{
    float fc = 5.0f;                 /* -3 dB at ~5 Hz */
    int N = DSP_DC_TAPS;
    int M = (N - 1) / 2;
    float sum = 0.0f;
    for (int n = 0; n < N; ++n) {
        int k = n - M;
        /* sinc LP impulse */
        float sinc;
        if (k == 0)
            sinc = 2.0f * fc / fs;
        else
            sinc = sinf(2.0f * (float)M_PI * fc * k / fs) /
                   ((float)M_PI * k);
        /* Hamming window */
        float w = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * n / (N - 1));
        /* High-pass = delta - LP */
        float lp = sinc * w;
        g_dc_coeffs[n] = (k == 0 ? 1.0f - lp : -lp);
        sum += g_dc_coeffs[n];
    }
    /* normalize */
    for (int n = 0; n < N; ++n) g_dc_coeffs[n] /= sum;
}

static void fir_run(const float *coeffs, int ntaps, const float *in,
                    float *out, int n, float *state)
{
    /* simple FIR with persistent state buffer of length ntaps+n */
    /* state[0..ntaps-1] = last ntaps of previous block */
    int M = ntaps - 1;
    for (int i = 0; i < n; ++i) {
        float acc = 0.0f;
        for (int t = 0; t < ntaps; ++t) {
            int idx = i - t;
            float v;
            if (idx < 0)
                v = state[M + idx + 1];   /* previous block tail */
            else
                v = in[idx];
            acc += coeffs[t] * v;
        }
        out[i] = acc;
    }
    /* save tail for next block */
    memmove(state, state + n, M * sizeof(float));
    memcpy(state + M, in + (n - 1), sizeof(float));
    /* (slightly simplified; acceptable for research firmware) */
}

/* ------------------------------------------------------------------ */
/*  Hilbert transformer — 64-tap type-III FIR                         */
/* ------------------------------------------------------------------ */
static float g_hb_coeffs[DSP_HILBERT_TAPS];

static void hilbert_design(void)
{
    int N = DSP_HILBERT_TAPS;
    int M = N / 2;          /* even, type III */
    for (int n = 0; n < N; ++n) {
        int k = n - M;
        if (k == 0) {
            g_hb_coeffs[n] = 0.0f;
        } else {
            float h;
            if (k & 1) {
                h = 2.0f / ((float)M_PI * k);   /* ideal Hilbert */
            } else {
                h = 0.0f;
            }
            /* Hamming window */
            float w = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * n / (N - 1));
            g_hb_coeffs[n] = h * w;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Wiener-style single-channel noise suppression                     */
/*  Simple spectral subtraction on 64-bin overlapping blocks.        */
/* ------------------------------------------------------------------ */
static void noise_suppress(float *x, int n, float *noise_floor,
                           float depth)
{
    /* We use a 64-point Hann window and a naive DFT (N is small). */
    const int N = 64;
    float re[64], im[64];
    float win[64];
    for (int i = 0; i < N; ++i)
        win[i] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (N - 1));

    int hop = N / 2;
    for (int pos = 0; pos + N <= n; pos += hop) {
        /* window */
        for (int i = 0; i < N; ++i) re[i] = x[pos + i] * win[i];
        memset(im, 0, sizeof(im));
        /* naive DFT */
        for (int k = 0; k < N; ++k) {
            float acc_r = 0.0f, acc_i = 0.0f;
            for (int i = 0; i < N; ++i) {
                float ang = -2.0f * (float)M_PI * k * i / N;
                acc_r += re[i] * cosf(ang);
                acc_i += re[i] * sinf(ang);
            }
            re[k] = acc_r / N;
            im[k] = acc_i / N;
        }
        /* update noise floor (running min) + subtract */
        for (int k = 0; k < N; ++k) {
            float mag = sqrtf(re[k] * re[k] + im[k] * im[k]);
            if (mag < noise_floor[k] || noise_floor[k] == 0.0f)
                noise_floor[k] = mag;
            float mag_clean = mag - depth * noise_floor[k];
            if (mag_clean < 0.0f) mag_clean = 0.0f;
            /* phase preserved via ratio */
            float g;
            if (mag > 1e-9f) g = mag_clean / mag;
            else             g = 0.0f;
            re[k] *= g;
            im[k] *= g;
        }
        /* inverse DFT */
        for (int i = 0; i < N; ++i) {
            float acc = 0.0f;
            for (int k = 0; k < N; ++k) {
                float ang = 2.0f * (float)M_PI * k * i / N;
                acc += re[k] * cosf(ang) + im[k] * sinf(ang);
            }
            x[pos + i] += acc * win[i] / N;   /* overlap-add */
        }
    }
}

/* ------------------------------------------------------------------ */
/*  AGC — peak/RMS follower with attack/release                       */
/* ------------------------------------------------------------------ */
static float agc_update(ltm_dsp_state_t *st, const ltm_dsp_config_t *cfg,
                        const float *x, int n)
{
    /* compute RMS */
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += x[i] * x[i];
    float rms = sqrtf(sum / n);

    /* error against target */
    float target = cfg->agc_target > 0.0f ? cfg->agc_target : DSP_AGC_TARGET_RMS;
    float err = target / (rms + 1e-6f);
    /* choose attack or release coefficient */
    float fs = (float)OUTPUT_SAMPLE_RATE_HZ;
    float block_s = (float)n / fs;
    float tau = (err > 1.0f) ? DSP_AGC_ATTACK_S : DSP_AGC_RELEASE_S;
    float alpha = 1.0f - expf(-block_s / tau);
    st->agc_gain += (err - st->agc_gain) * alpha;
    /* clamp */
    if (st->agc_gain > DSP_AGC_MAX_GAIN) st->agc_gain = DSP_AGC_MAX_GAIN;
    if (st->agc_gain < DSP_AGC_MIN_GAIN) st->agc_gain = DSP_AGC_MIN_GAIN;
    return rms;
}

/* ------------------------------------------------------------------ */
/*  FM quadrature demodulation                                         */
/* ------------------------------------------------------------------ */
static void fm_demod(ltm_dsp_state_t *st, const float *in,
                     float *out, int n)
{
    /* Q = Hilbert(in); I = delayed in */
    /* Run Hilbert FIR to produce Q in st->hb_out */
    fir_run(g_hb_coeffs, DSP_HILBERT_TAPS, in, st->hb_out, n, st->hb_state);
    /* I path: delay by (N/2) samples using a simple buffer */
    int delay = DSP_HILBERT_TAPS / 2;
    for (int i = 0; i < n; ++i) {
        float I;
        if (i - delay < 0)
            I = st->hb_delay[(n + i - delay) % n];  /* circular from prev */
        else
            I = in[i - delay];
        st->hb_delay[i] = in[i];
        float Q = st->hb_out[i];
        /* atan2 of (Q, I) gives instantaneous phase; differentiate */
        float phase = atan2f(Q, I);
        float dphase = phase - st->prev_i;   /* reuse prev_i as prev phase */
        /* unwrap */
        if (dphase > (float)M_PI)  dphase -= 2.0f * (float)M_PI;
        if (dphase < -(float)M_PI) dphase += 2.0f * (float)M_PI;
        st->prev_i = phase;
        out[i] = dphase * 4000.0f;   /* scale to audio range */
    }
}

/* ------------------------------------------------------------------ */
/*  AM envelope demodulation                                           */
/* ------------------------------------------------------------------ */
static void am_demod(const float *in, float *out, int n)
{
    /* full-wave rectify + low-pass */
    float prev_lp = 0.0f;
    float alpha = 0.05f;   /* ~1 kHz envelope LP for 48k */
    for (int i = 0; i < n; ++i) {
        float rect = in[i] < 0.0f ? -in[i] : in[i];
        float lp = prev_lp + alpha * (rect - prev_lp);
        prev_lp = lp;
        out[i] = lp;
    }
}

/* ------------------------------------------------------------------ */
/*  Decimate by 4 with simple averaging                                */
/* ------------------------------------------------------------------ */
static void decimate_4x(const float *in, float *out, int n)
{
    int m = n / DECIMATION_FACTOR;
    for (int i = 0; i < m; ++i) {
        float acc = 0.0f;
        for (int j = 0; j < DECIMATION_FACTOR; ++j)
            acc += in[i * DECIMATION_FACTOR + j];
        out[i] = acc / (float)DECIMATION_FACTOR;
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
void dsp_init(ltm_dsp_state_t *st, ltm_dsp_config_t *cfg)
{
    memset(st, 0, sizeof(*st));
    cfg->demod_mode   = DSP_DEMOD_FM;
    cfg->bp_low_hz    = 20.0f;
    cfg->bp_high_hz   = 16000.0f;
    cfg->noise_depth  = DSP_NOISE_DEPTH_DEFAULT;
    cfg->agc_target   = DSP_AGC_TARGET_RMS;
    cfg->bypass       = 0;

    dc_blocker_design((float)ADC_SAMPLE_RATE_HZ);
    hilbert_design();
    dsp_set_bandpass(st, cfg, cfg->bp_low_hz, cfg->bp_high_hz);
    st->agc_gain = 1.0f;
}

void dsp_set_bandpass(ltm_dsp_state_t *st, ltm_dsp_config_t *cfg,
                      float low_hz, float high_hz)
{
    float fs = (float)ADC_SAMPLE_RATE_HZ;
    cfg->bp_low_hz  = low_hz;
    cfg->bp_high_hz = high_hz;
    /* 2 HP stages + 2 LP stages for a steep-ish bandpass */
    biquad_hp(&g_bq[0], low_hz, fs, 0.707f);
    biquad_hp(&g_bq[1], low_hz, fs, 0.707f);
    biquad_lp(&g_bq[2], high_hz, fs, 0.707f);
    biquad_lp(&g_bq[3], high_hz, fs, 0.707f);
    (void)st;
}

float dsp_process_block(ltm_dsp_state_t *st, ltm_dsp_config_t *cfg,
                        const float *input, float *output)
{
    static float buf_a[DSP_BLOCK_SAMPLES];
    static float buf_b[DSP_BLOCK_SAMPLES];
    int n = DSP_BLOCK_SAMPLES;

    if (cfg->bypass) {
        memcpy(output, input, n * sizeof(float));
        st->last_rms_in = 0.0f;
        st->last_rms_out = 0.0f;
        return 0.0f;
    }

    /* 1. DC block */
    fir_run(g_dc_coeffs, DSP_DC_TAPS, input, buf_a, n, st->dc_state);

    /* 2. Bandpass biquad cascade */
    biquad_cascade_run(buf_a, n);

    /* 3. Demodulation */
    if (cfg->demod_mode == DSP_DEMOD_FM) {
        fm_demod(st, buf_a, buf_b, n);
    } else {
        am_demod(buf_a, buf_b, n);
    }

    /* 4. Noise suppression */
    if (cfg->noise_depth > 0.01f) {
        noise_suppress(buf_b, n, st->noise_floor, cfg->noise_depth);
    }

    /* 5. AGC */
    float rms_in = agc_update(st, cfg, buf_b, n);
    for (int i = 0; i < n; ++i) buf_b[i] *= st->agc_gain;
    st->last_rms_in = rms_in;

    /* 6. Decimate 4x → 48k */
    decimate_4x(buf_b, output, n);

    /* output RMS for display */
    float sum = 0.0f;
    int m = n / DECIMATION_FACTOR;
    for (int i = 0; i < m; ++i) sum += output[i] * output[i];
    st->last_rms_out = sqrtf(sum / m);
    st->last_snr_db = dsp_estimate_snr(st);
    return st->last_rms_out;
}

void dsp_int16_to_float(const int16_t *in, float *out, int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = (float)in[i] / 32768.0f;
}

void dsp_float_to_uac24(const float *in, uint8_t *out, int n)
{
    for (int i = 0; i < n; ++i) {
        /* clamp to [-1, 1] */
        float v = in[i];
        if (v > 1.0f)  v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        /* 24-bit signed */
        int32_t s = (int32_t)(v * 8388607.0f);
        /* pack 24-bit little-endian into 4-byte container (MSB=0 sign ext) */
        out[i * 4 + 0] = (uint8_t)(s & 0xFF);
        out[i * 4 + 1] = (uint8_t)((s >> 8) & 0xFF);
        out[i * 4 + 2] = (uint8_t)((s >> 16) & 0xFF);
        out[i * 4 + 3] = (s < 0) ? 0xFF : 0x00;
    }
}

float dsp_estimate_snr(const ltm_dsp_state_t *st)
{
    /* crude: 20 log10 (signal_rms / noise_floor_avg) */
    float nf = 0.0f;
    for (int i = 0; i < 64; ++i) nf += st->noise_floor[i];
    nf /= 64.0f;
    if (nf < 1e-6f) nf = 1e-6f;
    return 20.0f * log10f((st->last_rms_out + 1e-6f) / nf);
}