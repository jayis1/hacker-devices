/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Crypto Engine — AES-256-GCM transport & ECDH P-256 via STM32H5 SAES/PKA
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "crypto_driver.h"
#include <string.h>

/* Device long-term ECDH private key (in real deployment, stored in
 * TrustZone-protected OTP. For this reference firmware, a static key.)
 * P-256 private key (32 bytes). Author: jayis1 */
static const uint8_t device_ecdh_privkey[32] = {
    0x4a, 0x61, 0x79, 0x69, 0x73, 0x31, 0x47, 0x48,
    0x4f, 0x53, 0x54, 0x42, 0x55, 0x53, 0x2d, 0x43,
    0x6f, 0x76, 0x65, 0x72, 0x74, 0x44, 0x65, 0x62,
    0x75, 0x67, 0x49, 0x6d, 0x70, 0x6c, 0x61, 0x6e
};

/* Device public key (uncompressed P-256: X || Y, 64 bytes).
 * In production, computed at provisioning time. Author: jayis1 */
static const uint8_t device_ecdh_pubkey[64] = {
    0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81,
    0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09,
    0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
    0x98, 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f,
    0x10, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01
};

/* =========================================================================
 * CRC-32 (IEEE 802.3 polynomial) — used for non-cryptographic integrity
 * ========================================================================= */

static uint32_t crc32_table[256];
static uint8_t  crc32_table_init = 0;

static void crc32_init_table(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
        }
        crc32_table[i] = c;
    }
    crc32_table_init = 1;
}

uint32_t crypto_crc32(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    if (!crc32_table_init) crc32_init_table();
    crc = ~crc;
    for (uint32_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

/* =========================================================================
 * Hardware RNG via STM32H5 true random number generator
 * ========================================================================= */

void crypto_init(void)
{
    /* Enable RNG clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    (void)RCC->AHB2ENR;
    /* Enable SAES and PKA clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_SAESEN | RCC_AHB2ENR_PKAEN;
    (void)RCC->AHB2ENR;
    /* Enable RNG */
    RNG->CR |= RNG_CR_EN;
    while (!(RNG->SR & RNG_SR_DRDY)) { /* wait */ }
    crc32_init_table();
}

gb_status_t crypto_random_bytes(uint8_t *out, uint32_t len)
{
    if (!out || len == 0) return GB_ERR_PARAM;
    uint32_t i = 0;
    while (i < len) {
        while (!(RNG->SR & RNG_SR_DRDY)) { /* poll */ }
        uint32_t rnd = RNG->DR;
        for (int b = 0; b < 4 && i < len; b++) {
            out[i++] = (uint8_t)((rnd >> (b * 8)) & 0xFF);
        }
    }
    return GB_OK;
}

/* =========================================================================
 * SHA-256 (software implementation — avoids SAES-only limitation)
 * A minimal SHA-256 for integrity verification of extracted firmware.
 * ========================================================================= */

typedef struct {
    uint32_t h[8];
    uint64_t total_len;
    uint8_t  buf[64];
    uint32_t buf_len;
} sha256_ctx_t;

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,
    0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,
    0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,
    0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,
    0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,
    0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
    0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x,y,z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define EP1(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define S0(x)  (ROTR(x,7)  ^ ROTR(x,18) ^ ((x) >> 3))
#define S1(x)  (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

static void sha256_compress(sha256_ctx_t *ctx, const uint8_t *block)
{
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i*4]   << 24) |
               ((uint32_t)block[i*4+1] << 16) |
               ((uint32_t)block[i*4+2] << 8)  |
               ((uint32_t)block[i*4+3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = S1(w[i-2]) + w[i-7] + S0(w[i-15]) + w[i-16];
    }
    uint32_t a = ctx->h[0], b = ctx->h[1], c = ctx->h[2], d = ctx->h[3];
    uint32_t e = ctx->h[4], f = ctx->h[5], g = ctx->h[6], h = ctx->h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + EP1(e) + CH(e,f,g) + sha256_k[i] + w[i];
        uint32_t t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->h[0] += a; ctx->h[1] += b; ctx->h[2] += c; ctx->h[3] += d;
    ctx->h[4] += e; ctx->h[5] += f; ctx->h[6] += g; ctx->h[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx)
{
    ctx->h[0] = 0x6a09e667; ctx->h[1] = 0xbb67ae85;
    ctx->h[2] = 0x3c6ef372; ctx->h[3] = 0xa54ff53a;
    ctx->h[4] = 0x510e527f; ctx->h[5] = 0x9b05688c;
    ctx->h[6] = 0x1f83d9ab; ctx->h[7] = 0x5be0cd19;
    ctx->total_len = 0;
    ctx->buf_len = 0;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, uint32_t len)
{
    ctx->total_len += len;
    while (len > 0) {
        uint32_t copy = MIN(64 - ctx->buf_len, len);
        memcpy(ctx->buf + ctx->buf_len, data, copy);
        ctx->buf_len += copy;
        data += copy;
        len -= copy;
        if (ctx->buf_len == 64) {
            sha256_compress(ctx, ctx->buf);
            ctx->buf_len = 0;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t *out)
{
    /* Append 0x80, pad to 56, then 8-byte length */
    ctx->buf[ctx->buf_len++] = 0x80;
    if (ctx->buf_len > 56) {
        while (ctx->buf_len < 64) ctx->buf[ctx->buf_len++] = 0;
        sha256_compress(ctx, ctx->buf);
        ctx->buf_len = 0;
    }
    while (ctx->buf_len < 56) ctx->buf[ctx->buf_len++] = 0;
    uint64_t bits = ctx->total_len * 8;
    for (int i = 0; i < 8; i++) {
        ctx->buf[ctx->buf_len++] = (uint8_t)(bits >> (56 - i*8));
    }
    sha256_compress(ctx, ctx->buf);
    for (int i = 0; i < 8; i++) {
        out[i*4]   = (uint8_t)(ctx->h[i] >> 24);
        out[i*4+1] = (uint8_t)(ctx->h[i] >> 16);
        out[i*4+2] = (uint8_t)(ctx->h[i] >> 8);
        out[i*4+3] = (uint8_t)(ctx->h[i]);
    }
}

gb_status_t crypto_sha256(const uint8_t *data, uint32_t len, uint8_t *out_hash)
{
    if (!data || !out_hash) return GB_ERR_PARAM;
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out_hash);
    return GB_OK;
}

/* =========================================================================
 * AES-256-GCM via STM32H5 SAES accelerator
 *
 * GCM mode in SAES: set mode to GCM, load key + IV, process AAD then
 * plaintext/ciphertext, read tag. This is a simplified driver that
 * demonstrates the register-level interface; production code handles
 * multi-block AAD/plaintext continuation.
 * ========================================================================= */

static void saes_wait_ccf(void)
{
    while (!(SAES->SR & SAES_SR_CCF)) { }
    SAES->SR = SAES_SR_CCF; /* clear flag by writing (ICR not available) */
}

static void saes_load_key_256(const uint8_t *key)
{
    SAES->KEYR0 = ((uint32_t)key[3]  << 24) | ((uint32_t)key[2]  << 16) |
                  ((uint32_t)key[1]  << 8)  | key[0];
    SAES->KEYR1 = ((uint32_t)key[7]  << 24) | ((uint32_t)key[6]  << 16) |
                  ((uint32_t)key[5]  << 8)  | key[4];
    SAES->KEYR2 = ((uint32_t)key[11] << 24) | ((uint32_t)key[10] << 16) |
                  ((uint32_t)key[9]  << 8)  | key[8];
    SAES->KEYR3 = ((uint32_t)key[15] << 24) | ((uint32_t)key[14] << 16) |
                  ((uint32_t)key[13] << 8) | key[12];
    SAES->KEYR4 = ((uint32_t)key[19] << 24) | ((uint32_t)key[18] << 16) |
                  ((uint32_t)key[17] << 8) | key[16];
    SAES->KEYR5 = ((uint32_t)key[23] << 24) | ((uint32_t)key[22] << 16) |
                  ((uint32_t)key[21] << 8) | key[20];
    SAES->KEYR6 = ((uint32_t)key[27] << 24) | ((uint32_t)key[26] << 16) |
                  ((uint32_t)key[25] << 8) | key[24];
    SAES->KEYR7 = ((uint32_t)key[31] << 24) | ((uint32_t)key[30] << 16) |
                  ((uint32_t)key[29] << 8) | key[28];
}

static void saes_load_iv(const uint8_t *iv)
{
    SAES->IVR0 = ((uint32_t)iv[3]  << 24) | ((uint32_t)iv[2]  << 16) |
                 ((uint32_t)iv[1]  << 8)  | iv[0];
    SAES->IVR1 = ((uint32_t)iv[7]  << 24) | ((uint32_t)iv[6]  << 16) |
                 ((uint32_t)iv[5]  << 8)  | iv[4];
    SAES->IVR2 = ((uint32_t)iv[11] << 24) | ((uint32_t)iv[10] << 16) |
                 ((uint32_t)iv[9]  << 8)  | iv[8];
    SAES->IVR3 = 0x00000002; /* counter starts at 2 for GCM (J0 = IV||1) */
}

static void saes_write_block(const uint8_t *block)
{
    uint32_t w0 = ((uint32_t)block[3]  << 24) | ((uint32_t)block[2]  << 16) |
                  ((uint32_t)block[1]  << 8)  | block[0];
    uint32_t w1 = ((uint32_t)block[7]  << 24) | ((uint32_t)block[6]  << 16) |
                  ((uint32_t)block[5]  << 8)  | block[4];
    uint32_t w2 = ((uint32_t)block[11] << 24) | ((uint32_t)block[10] << 16) |
                  ((uint32_t)block[9]  << 8)  | block[8];
    uint32_t w3 = ((uint32_t)block[15] << 24) | ((uint32_t)block[14] << 16) |
                  ((uint32_t)block[13] << 8) | block[12];
    SAES->DINR = w0;
    saes_wait_ccf();
    SAES->DINR = w1;
    saes_wait_ccf();
    SAES->DINR = w2;
    saes_wait_ccf();
    SAES->DINR = w3;
    saes_wait_ccf();
}

static void saes_read_block(uint8_t *block)
{
    uint32_t w0 = SAES->DOUTR;
    uint32_t w1 = SAES->DOUTR;
    uint32_t w2 = SAES->DOUTR;
    uint32_t w3 = SAES->DOUTR;
    block[0]  = (uint8_t)(w0); block[1]  = (uint8_t)(w0 >> 8);
    block[2]  = (uint8_t)(w0 >> 16); block[3] = (uint8_t)(w0 >> 24);
    block[4]  = (uint8_t)(w1); block[5]  = (uint8_t)(w1 >> 8);
    block[6]  = (uint8_t)(w1 >> 16); block[7] = (uint8_t)(w1 >> 24);
    block[8]  = (uint8_t)(w2); block[9]  = (uint8_t)(w2 >> 8);
    block[10] = (uint8_t)(w2 >> 16); block[11] = (uint8_t)(w2 >> 24);
    block[12] = (uint8_t)(w3); block[13] = (uint8_t)(w3 >> 8);
    block[14] = (uint8_t)(w3 >> 16); block[15] = (uint8_t)(w3 >> 24);
}

gb_status_t crypto_aes_gcm(const uint8_t *key, uint32_t key_len,
                              const uint8_t *nonce, uint32_t nonce_len,
                              const uint8_t *aad, uint32_t aad_len,
                              const uint8_t *in, uint32_t in_len,
                              uint8_t *out, uint8_t *tag,
                              crypto_op_t op)
{
    if (!key || !nonce || !in || !out || !tag) return GB_ERR_PARAM;
    if (key_len != AES_256_KEY_SIZE || nonce_len != GCM_NONCE_SIZE) return GB_ERR_PARAM;

    /* Configure SAES for AES-256 GCM */
    SAES->CR = SAES_CR_KEYSIZE_256 | SAES_CR_MODE_GCM;
    saes_load_key_256(key);
    saes_load_iv(nonce);
    SAES->CR |= SAES_CR_EN;

    /* Process AAD (if any) as full blocks, pad last with zeros */
    if (aad && aad_len > 0) {
        uint32_t aad_blocks = (aad_len + 15) / 16;
        for (uint32_t i = 0; i < aad_blocks; i++) {
            uint8_t block[16] = {0};
            uint32_t copy = MIN(16, aad_len - i * 16);
            memcpy(block, aad + i * 16, copy);
            saes_write_block(block);
        }
    }

    /* Process plaintext/ciphertext in 16-byte blocks (pad with zeros) */
    uint32_t n_blocks = (in_len + 15) / 16;
    for (uint32_t i = 0; i < n_blocks; i++) {
        uint8_t block[16] = {0};
        uint32_t copy = MIN(16, in_len - i * 16);
        memcpy(block, in + i * 16, copy);
        saes_write_block(block);
        uint8_t out_block[16];
        saes_read_block(out_block);
        uint32_t out_copy = MIN(16, in_len - i * 16);
        memcpy(out + i * 16, out_block, out_copy);
    }

    /* Read GCM tag (16 bytes) from the accelerator */
    /* In real SAES, after writing the lengths register, the tag is
     * available in DOUTR (4 reads). We approximate by reading 4 words. */
    for (int i = 0; i < 4; i++) {
        uint32_t w = SAES->DOUTR;
        tag[i*4]   = (uint8_t)(w);
        tag[i*4+1] = (uint8_t)(w >> 8);
        tag[i*4+2] = (uint8_t)(w >> 16);
        tag[i*4+3] = (uint8_t)(w >> 24);
    }

    SAES->CR &= ~SAES_CR_EN;
    (void)op; /* GCM decrypt uses same key/IV; tag verification done by caller */
    return GB_OK;
}

gb_status_t crypto_ecdh_p256(const uint8_t *peer_pubkey, uint8_t *shared_secret)
{
    if (!peer_pubkey || !shared_secret) return GB_ERR_PARAM;
    /* Configure PKA for ECDH P-256 */
    PKA->CR = PKA_CR_EN | PKA_CR_MODE_ECDH;
    /* Load private key and peer public key into PKA RAM (indirect) */
    PKA->RAMADDR = 0x00;
    for (int i = 0; i < 32; i++) {
        PKA->RAMDATA = device_ecdh_privkey[i];
    }
    PKA->RAMADDR = 0x40;
    for (int i = 0; i < 64; i++) {
        PKA->RAMDATA = peer_pubkey[i];
    }
    /* Start computation */
    PKA->CR |= PKA_CR_START;
    /* Wait for completion */
    uint32_t timeout = 0;
    while ((PKA->SR & PKA_SR_BUSY) && timeout < 1000000) timeout++;
    if (PKA->SR & PKA_SR_BUSY) return GB_ERR_TIMEOUT;
    /* Read shared secret (32 bytes) from PKA RAM */
    PKA->RAMADDR = 0x80;
    for (int i = 0; i < 32; i++) {
        shared_secret[i] = (uint8_t)(PKA->RAMDATA & 0xFF);
    }
    return GB_OK;
}

gb_status_t crypto_get_pubkey(uint8_t *pubkey)
{
    if (!pubkey) return GB_ERR_PARAM;
    memcpy(pubkey, device_ecdh_pubkey, 64);
    return GB_OK;
}