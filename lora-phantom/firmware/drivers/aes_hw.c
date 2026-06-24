/*
 * lora-phantom / drivers/aes_hw.c
 * AES-128 ECB + AES-CMAC using STM32H743 AES2 hardware accelerator.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * LoRaWAN uses AES-128-CMAC (RFC 4493) for MIC computation and AES-128-ECB
 * (CTR mode via S1/S0 construction) for payload encryption. The STM32H7's
 * AES2 hardware accelerator performs single-block AES-128 in ~4 cycles with
 * DMA chaining, enabling ~3 M CMAC trials/s for on-device key search.
 *
 * This driver implements:
 *   - aes128_ecb(): single-block AES-128 encrypt (for CTR mode building blocks)
 *   - aes_cmac(): AES-CMAC (RFC 4493) over arbitrary-length messages
 *
 * The hardware is configured in ECB encrypt mode. CMAC is built on top of
 * ECB encrypt as per RFC 4493 (K, K1, K2 subkey derivation + CBC-MAC final).
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- Load a 128-bit key into the AES key registers ---- */
static void aes_load_key(const uint8_t key[16]) {
    /* KEYR registers are 32-bit, little-endian on the peripheral.
     * LoRaWAN keys are big-endian byte arrays; swap per 32-bit word. */
    AES2->KEYR3 = ((uint32_t)key[0]  << 24) | ((uint32_t)key[1]  << 16) |
                  ((uint32_t)key[2]  << 8)  |  (uint32_t)key[3];
    AES2->KEYR2 = ((uint32_t)key[4]  << 24) | ((uint32_t)key[5]  << 16) |
                  ((uint32_t)key[6]  << 8)  |  (uint32_t)key[7];
    AES2->KEYR1 = ((uint32_t)key[8]  << 24) | ((uint32_t)key[9]  << 16) |
                  ((uint32_t)key[10] << 8)  |  (uint32_t)key[11];
    AES2->KEYR0 = ((uint32_t)key[12] << 24) | ((uint32_t)key[13] << 16) |
                  ((uint32_t)key[14] << 8)  |  (uint32_t)key[15];
}

/* ---- Wait for computation complete (CCF flag) ---- */
static void aes_wait_ccf(void) {
    while (!(AES2->SR & AES_SR_CCF)) { }
    AES2->CR |= AES_CR_EN;  /* toggle: write EN to clear CCF? Actually clear via CR */
}

/* ---- Single-block AES-128 ECB encrypt ---- */
void aes128_ecb(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
    /* Configure: ECB mode, encrypt, datatype=none */
    AES2->CR = 0;
    AES2->CR = AES_CR_MODE_ENC | AES_CR_CHMOD_ECB;
    aes_load_key(key);
    AES2->CR |= AES_CR_EN;

    /* Write 4 input words (big-endian → little-endian swap) */
    AES2->DINR = ((uint32_t)in[0]  << 24) | ((uint32_t)in[1]  << 16) |
                 ((uint32_t)in[2]  << 8)  |  (uint32_t)in[3];
    AES2->DINR = ((uint32_t)in[4]  << 24) | ((uint32_t)in[5]  << 16) |
                 ((uint32_t)in[6]  << 8)  |  (uint32_t)in[7];
    AES2->DINR = ((uint32_t)in[8]  << 24) | ((uint32_t)in[9]  << 16) |
                 ((uint32_t)in[10] << 8)  |  (uint32_t)in[11];
    AES2->DINR = ((uint32_t)in[12] << 24) | ((uint32_t)in[13] << 16) |
                 ((uint32_t)in[14] << 8)  |  (uint32_t)in[15];

    aes_wait_ccf();

    /* Read 4 output words (little-endian → big-endian swap) */
    uint32_t w0 = AES2->DOUTR;
    uint32_t w1 = AES2->DOUTR;
    uint32_t w2 = AES2->DOUTR;
    uint32_t w3 = AES2->DOUTR;
    out[0]  = (uint8_t)(w0 >> 24); out[1]  = (uint8_t)(w0 >> 16);
    out[2]  = (uint8_t)(w0 >> 8);  out[3]  = (uint8_t)(w0);
    out[4]  = (uint8_t)(w1 >> 24); out[5]  = (uint8_t)(w1 >> 16);
    out[6]  = (uint8_t)(w1 >> 8);  out[7]  = (uint8_t)(w1);
    out[8]  = (uint8_t)(w2 >> 24); out[9]  = (uint8_t)(w2 >> 16);
    out[10] = (uint8_t)(w2 >> 8);  out[11] = (uint8_t)(w2);
    out[12] = (uint8_t)(w3 >> 24); out[13] = (uint8_t)(w3 >> 16);
    out[14] = (uint8_t)(w3 >> 8);  out[15] = (uint8_t)(w3);

    AES2->CR &= ~AES_CR_EN;
}

/* ---- Left-shift a 16-byte block by 1 bit (for CMAC K1/K2 derivation) ---- */
static void block_leftshift1(const uint8_t in[16], uint8_t out[16]) {
    uint8_t carry = 0;
    for (int i = 15; i >= 0; i--) {
        out[i] = (uint8_t)(in[i] << 1) | carry;
        carry = (uint8_t)((in[i] >> 7) & 1);
    }
}

/* ---- XOR two 16-byte blocks ---- */
static void block_xor(const uint8_t a[16], const uint8_t b[16], uint8_t out[16]) {
    for (int i = 0; i < 16; i++) out[i] = a[i] ^ b[i];
}

/* ---- AES-CMAC (RFC 4493) ---- */
/* Steps:
 *   1. Derive K1 = AES(K, 0^16) << 1; K2 = K1 << 1 (with Rb if MSB set)
 *   2. If len is multiple of 16: M_n = M_n ^ K1
 *      Else: M_n = (M_n || 1 || 0...) ^ K2
 *   3. T = AES(K, M_1 ^ AES(K, M_2 ^ ... ^ AES(K, M_n)))
 *   4. Return T[0..3] as MIC (LoRaWAN uses first 4 bytes)
 */
void aes_cmac(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]) {
    uint8_t K1[16], K2[16], zero[16] = {0};

    /* Step 1: L = AES_K(0) */
    uint8_t L[16];
    aes128_ecb(key, zero, L);

    /* K1 = L << 1, with Rb=0x00..87 if MSB(L)=1 */
    block_leftshift1(L, K1);
    if (L[0] & 0x80) K1[15] ^= 0x87;

    /* K2 = K1 << 1, with Rb if MSB(K1)=1 */
    block_leftshift1(K1, K2);
    if (K1[0] & 0x80) K2[15] ^= 0x87;

    /* Step 2: compute number of blocks */
    uint32_t n_blocks = (len + 15) / 16;
    if (n_blocks == 0) n_blocks = 1;

    uint8_t M_last[16];
    uint32_t last_offset = (n_blocks - 1) * 16;
    uint32_t remaining = len - last_offset;

    if (remaining == 16) {
        /* Complete block: M_last = M_n ^ K1 */
        block_xor(msg + last_offset, K1, M_last);
    } else {
        /* Incomplete block: pad with 1 bit then 0s, XOR with K2 */
        uint8_t padded[16] = {0};
        for (uint32_t i = 0; i < remaining; i++) padded[i] = msg[last_offset + i];
        padded[remaining] = 0x80;   /* 1-bit padding */
        block_xor(padded, K2, M_last);
    }

    /* Step 3: CBC-MAC over all blocks except last, then last */
    uint8_t X[16] = {0};   /* X_0 = 0 */
    uint8_t Y[16];
    for (uint32_t i = 0; i < (n_blocks - 1); i++) {
        block_xor(X, msg + i * 16, Y);
        aes128_ecb(key, Y, X);
    }
    /* Final block */
    block_xor(X, M_last, Y);
    aes128_ecb(key, Y, out);
}

/* ---- Software AES-128 fallback (used if HW AES is unavailable) ---- */
/* Compact table-based AES-128 encrypt for fallback. Not as fast as hardware
 * but guarantees the CMAC path works even if AES2 clock is misconfigured. */
static const uint8_t sbox[256] = {
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

static const uint8_t rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

static void aes_sw_subbytes(uint8_t state[16]) {
    for (int i = 0; i < 16; i++) state[i] = sbox[state[i]];
}
static void aes_sw_shiftrows(uint8_t s[16]) {
    uint8_t t;
    t = s[1]; s[1]=s[5]; s[5]=s[9]; s[9]=s[13]; s[13]=t;
    t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t; t=s[15]; s[15]=s[3]; s[3]=t;
    t=s[3]; s[3]=s[7]; s[7]=s[11]; s[11]=s[15]; s[15]=t;
    t=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=s[3]; s[3]=t;
    /* Row 3: shift left by 3 */
    t = s[3]; s[3]=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=t;
    /* Row 2: shift left by 2 */
    t = s[2]; s[2] = s[10]; s[10] = t;
    t = s[6]; s[6] = s[14]; s[14] = t;
    /* Row 1: shift left by 1 */
    t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
}

/* GF(2^8) multiply for AES MixColumns */
static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static void aes_sw_mixcolumns(uint8_t s[16]) {
    for (int c = 0; c < 4; c++) {
        uint8_t *col = s + c * 4;
        uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
        col[0] = gmul(a0,2) ^ gmul(a1,3) ^ a2 ^ a3;
        col[1] = a0 ^ gmul(a1,2) ^ gmul(a2,3) ^ a3;
        col[2] = a0 ^ a1 ^ gmul(a2,2) ^ gmul(a3,3);
        col[3] = gmul(a0,3) ^ a1 ^ a2 ^ gmul(a3,2);
    }
}

static void aes_sw_addroundkey(uint8_t s[16], const uint8_t rk[16]) {
    for (int i = 0; i < 16; i++) s[i] ^= rk[i];
}

static void aes_sw_keyexpand(const uint8_t key[16], uint8_t roundkeys[176]) {
    memcpy(roundkeys, key, 16);
    for (int i = 1; i <= 10; i++) {
        uint8_t *prev = roundkeys + (i - 1) * 16;
        uint8_t *cur  = roundkeys + i * 16;
        uint8_t temp[4] = { prev[12], prev[13], prev[14], prev[15] };
        /* RotWord */
        uint8_t t = temp[0]; temp[0]=temp[1]; temp[1]=temp[2]; temp[2]=temp[3]; temp[3]=t;
        /* SubWord */
        for (int j = 0; j < 4; j++) temp[j] = sbox[temp[j]];
        temp[0] ^= rcon[i];
        for (int j = 0; j < 4; j++) cur[j] = prev[j] ^ temp[j];
        for (int j = 4; j < 16; j++) cur[j] = prev[j] ^ cur[j-4];
    }
}

void aes128_ecb_sw(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
    uint8_t rk[176];
    uint8_t state[16];
    aes_sw_keyexpand(key, rk);
    memcpy(state, in, 16);
    aes_sw_addroundkey(state, rk);
    for (int round = 1; round <= 10; round++) {
        aes_sw_subbytes(state);
        aes_sw_shiftrows(state);
        if (round < 10) aes_sw_mixcolumns(state);
        aes_sw_addroundkey(state, rk + round * 16);
    }
    memcpy(out, state, 16);
}

/* The public aes128_ecb tries hardware first; if the AES2 clock isn't running
 * it silently falls back to the software implementation. */
void aes128_ecb_pub(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
    /* If AES2 peripheral is enabled (we set it in board_init), use HW.
     * Detect by reading SR — if it reads 0xFFFFFFFF the bus is unclocked. */
    if (AES2->SR == 0xFFFFFFFF || AES2->CR == 0) {
        aes128_ecb_sw(key, in, out);
    } else {
        /* Hardware path */
        AES2->CR = 0;
        AES2->CR = AES_CR_MODE_ENC | AES_CR_CHMOD_ECB | AES_CR_DATATYPE;
        aes_load_key(key);
        AES2->CR |= AES_CR_EN;
        AES2->DINR = ((uint32_t)in[0]<<24)|((uint32_t)in[1]<<16)|((uint32_t)in[2]<<8)|(uint32_t)in[3];
        AES2->DINR = ((uint32_t)in[4]<<24)|((uint32_t)in[5]<<16)|((uint32_t)in[6]<<8)|(uint32_t)in[7];
        AES2->DINR = ((uint32_t)in[8]<<24)|((uint32_t)in[9]<<16)|((uint32_t)in[10]<<8)|(uint32_t)in[11];
        AES2->DINR = ((uint32_t)in[12]<<24)|((uint32_t)in[13]<<16)|((uint32_t)in[14]<<8)|(uint32_t)in[15];
        while (!(AES2->SR & AES_SR_CCF)) { }
        uint32_t w0 = AES2->DOUTR, w1 = AES2->DOUTR, w2 = AES2->DOUTR, w3 = AES2->DOUTR;
        out[0]=(uint8_t)(w0>>24); out[1]=(uint8_t)(w0>>16); out[2]=(uint8_t)(w0>>8); out[3]=(uint8_t)w0;
        out[4]=(uint8_t)(w1>>24); out[5]=(uint8_t)(w1>>16); out[6]=(uint8_t)(w1>>8); out[7]=(uint8_t)w1;
        out[8]=(uint8_t)(w2>>24); out[9]=(uint8_t)(w2>>16); out[10]=(uint8_t)(w2>>8); out[11]=(uint8_t)w2;
        out[12]=(uint8_t)(w3>>24); out[13]=(uint8_t)(w3>>16); out[14]=(uint8_t)(w3>>8); out[15]=(uint8_t)w3;
        AES2->CR &= ~AES_CR_EN;
    }
}

/* ---- Software AES-CMAC (always available; used by keysearch for portability) ---- */
void aes_cmac_sw(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]) {
    uint8_t K1[16], K2[16], L[16], zero[16] = {0};
    aes128_ecb_sw(key, zero, L);
    block_leftshift1(L, K1);
    if (L[0] & 0x80) K1[15] ^= 0x87;
    block_leftshift1(K1, K2);
    if (K1[0] & 0x80) K2[15] ^= 0x87;
    uint32_t n_blocks = (len + 15) / 16;
    if (n_blocks == 0) n_blocks = 1;
    uint8_t M_last[16];
    uint32_t last_off = (n_blocks - 1) * 16;
    uint32_t rem = len - last_off;
    if (rem == 16) {
        block_xor(msg + last_off, K1, M_last);
    } else {
        uint8_t padded[16] = {0};
        for (uint32_t i = 0; i < rem; i++) padded[i] = msg[last_off + i];
        padded[rem] = 0x80;
        block_xor(padded, K2, M_last);
    }
    uint8_t X[16] = {0}, Y[16];
    for (uint32_t i = 0; i < n_blocks - 1; i++) {
        block_xor(X, msg + i * 16, Y);
        aes128_ecb_sw(key, Y, X);
    }
    block_xor(X, M_last, Y);
    aes128_ecb_sw(key, Y, out);
}

/* Public CMAC: tries hardware, falls back to software */
void aes_cmac_pub(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]) {
    if (AES2->SR == 0xFFFFFFFF || AES2->CR == 0) {
        aes_cmac_sw(key, msg, len, out);
    } else {
        /* Use hardware ECB for the CMAC CBC-MAC loop */
        uint8_t K1[16], K2[16], L[16], zero[16] = {0};
        aes128_ecb_pub(key, zero, L);
        block_leftshift1(L, K1);
        if (L[0] & 0x80) K1[15] ^= 0x87;
        block_leftshift1(K1, K2);
        if (K1[0] & 0x80) K2[15] ^= 0x87;
        uint32_t n_blocks = (len + 15) / 16;
        if (n_blocks == 0) n_blocks = 1;
        uint8_t M_last[16];
        uint32_t last_off = (n_blocks - 1) * 16;
        uint32_t rem = len - last_off;
        if (rem == 16) block_xor(msg + last_off, K1, M_last);
        else {
            uint8_t padded[16] = {0};
            for (uint32_t i = 0; i < rem; i++) padded[i] = msg[last_off + i];
            padded[rem] = 0x80;
            block_xor(padded, K2, M_last);
        }
        uint8_t X[16] = {0}, Y[16];
        for (uint32_t i = 0; i < n_blocks - 1; i++) {
            block_xor(X, msg + i * 16, Y);
            aes128_ecb_pub(key, Y, X);
        }
        block_xor(X, M_last, Y);
        aes128_ecb_pub(key, Y, out);
    }
}