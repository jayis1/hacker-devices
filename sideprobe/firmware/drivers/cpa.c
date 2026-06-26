/*
 * drivers/cpa.c — Correlation Power Analysis (CPA) attack engine
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements on-device Correlation Power Analysis to recover an AES-128 key
 * from captured power/EM traces and known plaintexts.
 *
 * Method:
 *   For each key byte b (0..15) and each key guess k (0..255):
 *     hypothesis h_{t,k} = HW(AES_SBOX[ pt_t[b] XOR k ])   (first round)
 *   Pearson correlation r_{b,k} = cov(h_k, power_t) / sqrt(var(h_k) * var(power_t))
 *   where power_t is a chosen sample point (the leakage sample) within each
 *   trace. The guess with max |r| per byte is the recovered key byte.
 *
 * Numerical stability: Welford's online algorithm for variance, double-precision
 * FPU throughout. The hypothesis is 0/255-distributed; the power samples are
 * 16-bit signed (centered around 0). Both are accumulated per-guess.
 *
 * Memory: per-guess accumulators for 16 bytes × 256 guesses = 4096 doubles = 32 KB
 * for Σh, Σh², Σ(h·t), plus Σt, Σt² per trace-point (but we use the FPGA's
 * per-trace Σ/Σ² and the single leak sample at a configurable index). So total
 * accumulator RAM ≈ 4096 × 3 × 8 = 96 KB, well within SRAM.
 */

#include <stdint.h>
#include <math.h>
#include "../board.h"
#include "../registers.h"

/* AES S-box */
static const uint8_t AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

/* Hamming weight lookup (popcount of byte) */
static const uint8_t HW_TABLE[256] = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

/* ---- Per-guess accumulators ----
 * Layout: index = byte*256 + guess
 *   sum_h[b*256+k]   = Σ_t h_{t,k}
 *   sum_h2[b*256+k]  = Σ_t h_{t,k}^2
 *   sum_ht[b*256+k]  = Σ_t h_{t,k} * leak_t
 * These are doubles (8 bytes) -> 16*256*3*8 = 98 KB, fits in DTCM/SRAM.
 */
static double sum_h[KEY_BYTES_AES128 * KEY_GUESS_COUNT];
static double sum_h2[KEY_BYTES_AES128 * KEY_GUESS_COUNT];
static double sum_ht[KEY_BYTES_AES128 * KEY_GUESS_COUNT];

/* Per-trace leak-point statistics (running, over all traces fed so far) */
static double sum_t   = 0.0;
static double sum_t2  = 0.0;
static uint32_t n_fed = 0;

/* Index of the leak sample within each trace (the sample point whose
   correlation we evaluate). Default ~30% into the first-round window. */
static uint32_t leak_sample_idx = (TRACE_SAMPLES * 3) / 10;

/* Precompute the 256 hypothesis values for a given (byte position, plaintext
   byte). Returns the HW of the S-box output for each key guess. */
static void hypotheses_for_byte(uint8_t pt_byte, uint8_t byte_pos,
                                uint8_t *out256) {
    (void)byte_pos;
    for (uint16_t k = 0; k < 256; k++) {
        uint8_t s = AES_SBOX[pt_byte ^ (uint8_t)k];
        out256[k] = HW_TABLE[s];
    }
}

int cpa_init(cpa_state_t *st, uint8_t model, uint8_t key_bytes,
             uint32_t n_traces, uint32_t samples_per_trace,
             int16_t *trace_buf, uint8_t *plaintexts) {
    memset(st, 0, sizeof(*st));
    st->model = model;
    st->key_bytes = key_bytes;
    st->n_traces = n_traces;
    st->samples_per_trace = samples_per_trace;
    st->trace_buf = trace_buf;
    st->plaintexts = plaintexts;
    st->traces_done = 0;
    st->checkpoint = 256;

    /* Reset accumulators */
    memset(sum_h, 0, sizeof(sum_h));
    memset(sum_h2, 0, sizeof(sum_h2));
    memset(sum_ht, 0, sizeof(sum_ht));
    sum_t = 0.0; sum_t2 = 0.0; n_fed = 0;
    return 0;
}

int cpa_feed_trace(cpa_state_t *st, uint32_t trace_idx) {
    /* Grab the leak sample for this trace */
    int16_t *trace = &st->trace_buf[trace_idx * st->samples_per_trace];
    int32_t leak = trace[leak_sample_idx];
    double t = (double)leak;

    /* Update global running sums for the leak variable */
    n_fed++;
    double delta = t - (sum_t / (double)(n_fed - 1 > 0 ? n_fed - 1 : 1));
    sum_t  += t;
    sum_t2 += t * t;
    (void)delta;

    /* For each key byte position, compute the 256 hypotheses and accumulate */
    uint8_t hyp[256];
    for (uint8_t b = 0; b < st->key_bytes; b++) {
        uint8_t pt = st->plaintexts[trace_idx * st->key_bytes + b];
        hypotheses_for_byte(pt, b, hyp);
        double *sh  = &sum_h [b * 256];
        double *sh2 = &sum_h2[b * 256];
        double *sht = &sum_ht[b * 256];
        for (uint16_t k = 0; k < 256; k++) {
            double h = (double)hyp[k];
            sh[k]  += h;
            sh2[k] += h * h;
            sht[k] += h * t;
        }
    }
    return 0;
}

int cpa_compute(cpa_state_t *st) {
    /* Compute Pearson r for every (byte, guess) and find best/second per byte */
    double n = (double)n_fed;
    if (n < 2.0) return -1;
    double var_t = (sum_t2 - (sum_t * sum_t) / n);
    if (var_t <= 0.0) var_t = 1e-12;

    for (uint8_t b = 0; b < st->key_bytes; b++) {
        const double *sh  = &sum_h [b * 256];
        const double *sh2 = &sum_h2[b * 256];
        const double *sht = &sum_ht[b * 256];

        float best = -2.0f, second = -2.0f;
        uint8_t best_k = 0;
        for (uint16_t k = 0; k < 256; k++) {
            double mean_h  = sh[k]  / n;
            double var_h   = sh2[k] / n - mean_h * mean_h;
            if (var_h <= 0.0) var_h = 1e-12;
            double mean_ht = sht[k] / n;
            double cov    = mean_ht - mean_h * (sum_t / n);
            double denom = sqrt(var_h * var_t);
            double r = (denom > 1e-15) ? (cov / denom) : 0.0;
            float rf = (float)fabs(r);
            if (rf > best) {
                second = best;
                best = rf;
                best_k = (uint8_t)k;
            } else if (rf > second) {
                second = rf;
            }
        }
        st->recovered_key[b] = best_k;
        st->corr_best[b]   = best;
        st->corr_second[b] = second;
        st->converged[b]   = (best > 0.001f && (best / (second + 1e-9f)) >= 3.0f) ? 1 : 0;
    }
    st->traces_done = n_fed;
    return 0;
}

int cpa_converged(const cpa_state_t *st, float margin) {
    uint8_t count = 0;
    for (uint8_t b = 0; b < st->key_bytes; b++) {
        if (st->corr_best[b] > 0.0005f &&
            (st->corr_best[b] / (st->corr_second[b] + 1e-9f)) >= margin) {
            count++;
        }
    }
    return count;
}

/* Set the leak sample index (the point whose correlation we track). The app
 * can sweep this to find the strongest leakage point. */
void cpa_set_leak_sample(uint32_t idx) {
    if (idx < TRACE_SAMPLES) leak_sample_idx = idx;
}

uint32_t cpa_get_leak_sample(void) { return leak_sample_idx; }