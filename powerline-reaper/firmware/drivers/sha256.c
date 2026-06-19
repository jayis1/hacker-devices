/*
 * sha256.c — SHA-256 (used by PBKDF2 for NMK derivation)
 *
 * Reference FIPS 180-4 implementation. Constant-time w.r.t. secret data
 * that fits in a single block. The STM32H743 has a HASH peripheral that
 * could accelerate this; we keep the portable C path for clarity.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "sha256.h"

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x,y,z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x)&(y)) ^ ((x)&(z)) ^ ((y)&(z)))
#define S0(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S1(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define s0(x) (ROTR(x,7) ^ ROTR(x,18) ^ ((x) >> 3))
#define s1(x) (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

static void sha256_block(sha256_context *ctx, const uint8_t *block) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i*4] << 24) | ((uint32_t)block[i*4+1] << 16)
             | ((uint32_t)block[i*4+2] << 8) | ((uint32_t)block[i*4+3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = s1(w[i-2]) + w[i-7] + s0(w[i-15]) + w[i-16];
    }
    uint32_t a = ctx->state[0], b = ctx->state[1], c = ctx->state[2], d = ctx->state[3];
    uint32_t e = ctx->state[4], f = ctx->state[5], g = ctx->state[6], h = ctx->state[7];
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + S1(e) + CH(e,f,g) + K[i] + w[i];
        uint32_t t2 = S0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

void sha256_init(sha256_context *ctx) {
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
    ctx->len = 0;
    ctx->buf_len = 0;
}

void sha256_update(sha256_context *ctx, const uint8_t *data, uint32_t len) {
    ctx->len += len;
    while (len > 0) {
        uint32_t take = 64 - ctx->buf_len;
        if (take > len) take = len;
        memcpy(&ctx->buf[ctx->buf_len], data, take);
        ctx->buf_len += take;
        data += take;
        len  -= take;
        if (ctx->buf_len == 64) {
            sha256_block(ctx, ctx->buf);
            ctx->buf_len = 0;
        }
    }
}

void sha256_final(sha256_context *ctx, uint8_t out[32]) {
    uint64_t bitlen = (uint64_t)ctx->len * 8;
    ctx->buf[ctx->buf_len++] = 0x80;
    if (ctx->buf_len > 56) {
        while (ctx->buf_len < 64) ctx->buf[ctx->buf_len++] = 0;
        sha256_block(ctx, ctx->buf);
        ctx->buf_len = 0;
    }
    while (ctx->buf_len < 56) ctx->buf[ctx->buf_len++] = 0;
    for (int i = 7; i >= 0; i--) ctx->buf[ctx->buf_len++] = (bitlen >> (i*8)) & 0xFF;
    sha256_block(ctx, ctx->buf);
    for (int i = 0; i < 8; i++) {
        out[i*4]   = (ctx->state[i] >> 24) & 0xFF;
        out[i*4+1] = (ctx->state[i] >> 16) & 0xFF;
        out[i*4+2] = (ctx->state[i] >> 8) & 0xFF;
        out[i*4+3] = ctx->state[i] & 0xFF;
    }
}