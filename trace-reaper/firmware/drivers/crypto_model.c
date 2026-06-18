/*
 * TRACE-REAPER — crypto_model driver
 * Leakage models and S-box tables for Correlation Power Analysis.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "crypto_model.h"
#include <string.h>

/* ---- AES S-box ---- */
static const uint8_t aes_sbox[256] = {
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
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
};

/* ---- DES S-boxes (S1..S8) compact row-encoded form ----
 * Each S-box is 4 rows of 16 entries. We store flat: row-major.
 */
static const uint8_t des_sbox[8][64] = {
    /* S1 */
    {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7, 0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,
     4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0, 15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13},
    /* S2 */
    {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10, 3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,
     0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15, 13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9},
    /* S3 */
    {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8, 13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
     13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7, 1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12},
    /* S4 */
    {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15, 13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
     10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4, 3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14},
    /* S5 */
    {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9, 14,11,2,12,4,7,13,1,5,0,15,10,9,3,8,6,
     4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14, 11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3},
    /* S6 */
    {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11, 10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
     9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6, 4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13},
    /* S7 */
    {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1, 13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
     1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2, 6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12},
    /* S8 */
    {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7, 1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
     7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8, 2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11},
};

/* ---- Popcount (Hamming weight) ---- */
uint8_t hw8(uint8_t x)
{
    x = (uint8_t)((x & 0x55) + ((x >> 1) & 0x55));
    x = (uint8_t)((x & 0x33) + ((x >> 2) & 0x33));
    x = (uint8_t)((x & 0x0F) + ((x >> 4) & 0x0F));
    return x;
}

/* ---- Model evaluation ----
 * For a given leak model, key byte index i (0..15 for AES-128), key hypothesis
 * k (0..255), and plaintext/ciphertext byte t, return the predicted value
 * (typically 0..8 for HW or 0..8 for HD).
 */
uint8_t crypto_model_eval(leak_model_t m, uint8_t i, uint8_t k, uint8_t t)
{
    uint8_t s;
    switch (m) {
    case LEAK_HW_SBOX_OUT:
        s = aes_sbox[(uint8_t)(t ^ k)];
        return hw8(s);
    case LEAK_HW_SBOX_IN:
        return hw8((uint8_t)(t ^ k));
    case LEAK_HD_SBOX_OUT:
        s = aes_sbox[(uint8_t)(t ^ k)];
        return hw8((uint8_t)(s ^ (t ^ k)));
    case LEAK_HW_MIXCOL: {
        /* simplified: HW of one column of MixColumns output
         * For byte i in column c, this is HW of (S(t^k) * {02} XOR ...).
         * We approximate with HW of 2*S(t^k) for the byte-of-interest.
         */
        s = aes_sbox[(uint8_t)(t ^ k)];
        uint8_t v = (uint8_t)((s << 1) ^ ((s & 0x80) ? 0x1b : 0x00));
        return hw8(v);
    }
    case LEAK_HW_LASTROUND: {
        /* HD of last-round T-table byte vs ciphertext byte.
         * Simplified: HW(S(t^k) ^ t) where t is ciphertext byte.
         */
        s = aes_sbox[(uint8_t)(t ^ k)];
        return hw8((uint8_t)(s ^ t));
    }
    default:
        return 0;
    }
}

/* DES: HW of S-box output for given round key hypothesis (6-bit in, 4-bit out).
 * round selects which S-box set (0..7); k6 is 6-bit hypothesis.
 */
uint8_t crypto_model_des(uint8_t sbox_idx, uint8_t k6)
{
    if (sbox_idx > 7) return 0;
    uint8_t row = (uint8_t)(((k6 >> 4) & 0x02) | (k6 & 0x01));
    uint8_t col = (uint8_t)((k6 >> 1) & 0x0F);
    return hw8(des_sbox[sbox_idx][row * 16 + col]);
}

/* ---- Per-byte hypothesis generator for AES-128 standard CPA ----
 * Fills out[256] with model values for hypothesis byte k over a fixed
 * plaintext byte t. This is the precomputed hypothesis vector used by the
 * FPGA correlation engine.
 */
void crypto_model_hyp_vector(leak_model_t m, uint8_t i, uint8_t t,
                              uint8_t out[256])
{
    for (uint16_t k = 0; k < 256; k++) {
        out[k] = crypto_model_eval(m, i, (uint8_t)k, t);
    }
}

/* ---- Pearson rho from running sums ----
 * rho = (n*sum_xy - sum_x*sum_y) /
 *       sqrt( (n*sum_xx - sum_x^2) * (n*sum_yy - sum_y^2) )
 */
float corr_accum_rho(const corr_accum_t *a)
{
    if (a->n < 2) return 0.0f;
    int64_t num = (int64_t)a->n * a->sum_xy - a->sum_x * a->sum_y;
    int64_t dx  = (int64_t)a->n * a->sum_xx - a->sum_x * a->sum_x;
    int64_t dy  = (int64_t)a->n * a->sum_yy - a->sum_y * a->sum_y;
    if (dx <= 0 || dy <= 0) return 0.0f;
    /* integer sqrt approximations are avoided; use float */
    float fn = (float)num;
    float fdx = (float)dx;
    float fdy = (float)dy;
    return fn / (fdx * fdy > 0.0f ? sqrtf(fdx * fdy) : 1.0f);
}

/* sqrtf is from libm; declare here to avoid extra includes */
extern float sqrtf(float x);

/* ---- Pick best per-byte hypothesis from a corr_byte_t table ----
 * Writes best hypothesis to *out_hyp, returns rho.
 */
float crypto_model_best(const corr_byte_t *b, uint8_t *out_hyp)
{
    float best = -2.0f;
    uint8_t bk = 0;
    for (uint16_t k = 0; k < 256; k++) {
        float r = corr_accum_rho(&b->cells[k]);
        if (r > best) { best = r; bk = (uint8_t)k; }
    }
    if (out_hyp) *out_hyp = bk;
    return best;
}

/* ---- Fold one trace into the per-byte correlation table ----
 * For each key byte i (0..nbytes-1) and each hypothesis k (0..255):
 *   x = model value (precomputed into hyp[][])
 *   y = sample value at sample index s for byte i (the "POI")
 * Update the running sums.
 *
 * For simplicity the per-byte POI (point of interest) sample index is given
 * by poi[i]. The caller precomputes poi[] from the trace structure or just
 * uses a fixed offset (e.g. poi[i] = i * stride).
 */
void crypto_model_fold(corr_byte_t *table, uint8_t nbytes,
                       const uint8_t hyp[KEY_BYTES_AES256][256],
                       const int16_t *trace, uint16_t nsamp,
                       const uint16_t poi[KEY_BYTES_AES256])
{
    for (uint8_t i = 0; i < nbytes; i++) {
        uint16_t s = poi[i];
        if (s >= nsamp) continue;
        int16_t y = trace[s];
        corr_byte_t *bt = &table[i];
        for (uint16_t k = 0; k < 256; k++) {
            corr_accum_t *c = &bt->cells[k];
            int32_t x = hyp[i][k];
            c->n += 1;
            c->sum_x  += x;
            c->sum_xx += (int64_t)x * x;
            c->sum_y  += y;
            c->sum_yy += (int64_t)y * y;
            c->sum_xy += (int64_t)x * y;
        }
    }
}

/* ---- Reset a table ---- */
void crypto_model_reset(corr_byte_t *table, uint8_t nbytes)
{
    memset(table, 0, sizeof(corr_byte_t) * nbytes);
}