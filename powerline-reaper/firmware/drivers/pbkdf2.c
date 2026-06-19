/*
 * pbkdf2.c — PBKDF2-HMAC-SHA256 (constant-time-ish, used for HomePlug AV NMK)
 *
 * HomePlug AV derives the Network Membership Key via:
 *   NMK = PBKDF2-HMAC-SHA256(passphrase, salt, 1000, 16)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "pbkdf2.h"
#include "sha256.h"

static void hmac_sha256(const uint8_t *key, uint32_t key_len,
                         const uint8_t *msg, uint32_t msg_len,
                         uint8_t out[32]) {
    uint8_t k[64];
    if (key_len > 64) {
        sha256_context h;
        sha256_init(&h);
        sha256_update(&h, key, key_len);
        sha256_final(&h, k);
        memset(&k[32], 0, 32);
    } else {
        memcpy(k, key, key_len);
        memset(&k[key_len], 0, 64 - key_len);
    }
    uint8_t ipad[64], opad[64];
    for (int i = 0; i < 64; i++) {
        ipad[i] = k[i] ^ 0x36;
        opad[i] = k[i] ^ 0x5C;
    }
    sha256_context h1, h2;
    sha256_init(&h1);
    sha256_update(&h1, ipad, 64);
    sha256_update(&h1, msg, msg_len);
    uint8_t inner[32];
    sha256_final(&h1, inner);
    sha256_init(&h2);
    sha256_update(&h2, opad, 64);
    sha256_update(&h2, inner, 32);
    sha256_final(&h2, out);
}

void pbkdf2_hmac_sha256(const uint8_t *pass, uint32_t pass_len,
                          const uint8_t *salt, uint32_t salt_len,
                          uint32_t iters, uint8_t *out, uint32_t out_len) {
    uint32_t blocks = (out_len + 31) / 32;
    for (uint32_t i = 1; i <= blocks; i++) {
        uint8_t U[32];
        uint8_t T[32];
        /* First iteration: salt || INT(i) */
        uint8_t s[128];
        uint32_t sl = salt_len;
        memcpy(s, salt, sl);
        s[sl++] = (i >> 24) & 0xFF;
        s[sl++] = (i >> 16) & 0xFF;
        s[sl++] = (i >> 8) & 0xFF;
        s[sl++] = i & 0xFF;
        hmac_sha256(pass, pass_len, s, sl, U);
        memcpy(T, U, 32);
        for (uint32_t j = 1; j < iters; j++) {
            hmac_sha256(pass, pass_len, U, 32, U);
            for (int k = 0; k < 32; k++) T[k] ^= U[k];
        }
        uint32_t off = (i - 1) * 32;
        uint32_t cpy = (off + 32 <= out_len) ? 32 : (out_len - off);
        memcpy(&out[off], T, cpy);
    }
}