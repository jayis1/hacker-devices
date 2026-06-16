/**
 * @file crypto_engine.c
 * @author jayis1
 * @brief Cryptographic Engine for SATA Phantom
 *
 * Wraps mbedTLS for AES-256-GCM encryption/decryption of exfiltrated data,
 * key derivation, and secure channel establishment.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/sha256.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "board.h"

static const char *TAG = "crypto";

/* Static context for encryption operations */
static mbedtls_gcm_context gcm_ctx;
static mbedtls_ctr_drbg_context drbg_ctx;
static mbedtls_entropy_context entropy_ctx;
static bool crypto_initialized = false;
static uint8_t aes_key[32];  /* AES-256 key */
static uint64_t nonce_counter = 0;

/* ===================================================================
 * Initialization
 * =================================================================== */

esp_err_t crypto_init(const uint8_t *key, size_t key_len)
{
    if (!key || key_len < 32) {
        ESP_LOGE(TAG, "Invalid key (need 32 bytes for AES-256)");
        return ESP_FAIL;
    }

    mbedtls_gcm_init(&gcm_ctx);
    mbedtls_entropy_init(&entropy_ctx);
    mbedtls_ctr_drbg_init(&drbg_ctx);

    /* Seed the DRBG */
    int ret = mbedtls_ctr_drbg_seed(&drbg_ctx, mbedtls_entropy_func,
                                     &entropy_ctx, NULL, 0);
    if (ret != 0) {
        ESP_LOGE(TAG, "DRBG seed failed: %d", ret);
        return ESP_FAIL;
    }

    /* Set AES-256 key */
    memcpy(aes_key, key, 32);
    ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, aes_key, 256);
    if (ret != 0) {
        ESP_LOGE(TAG, "GCM setkey failed: %d", ret);
        return ESP_FAIL;
    }

    /* Generate initial nonce */
    mbedtls_ctr_drbg_random(&drbg_ctx, (uint8_t *)&nonce_counter, sizeof(nonce_counter));

    crypto_initialized = true;
    ESP_LOGI(TAG, "Crypto engine initialized");
    return ESP_OK;
}

void crypto_deinit(void)
{
    if (crypto_initialized) {
        mbedtls_gcm_free(&gcm_ctx);
        mbedtls_ctr_drbg_free(&drbg_ctx);
        mbedtls_entropy_free(&entropy_ctx);
        crypto_initialized = false;
    }
}

/* ===================================================================
 * Encryption / Decryption
 * =================================================================== */

/**
 * @brief Encrypt plaintext with AES-256-GCM.
 * Output format: [12-byte nonce][ciphertext][16-byte GCM tag]
 */
esp_err_t crypto_encrypt(const uint8_t *plaintext, size_t pt_len,
                          uint8_t *ciphertext, size_t *ct_len)
{
    if (!crypto_initialized) {
        ESP_LOGE(TAG, "Crypto not initialized");
        return ESP_FAIL;
    }

    /* Build nonce: 8 bytes counter + 4 bytes random */
    uint8_t nonce[12];
    memcpy(nonce, &nonce_counter, 8);
    mbedtls_ctr_drbg_random(&drbg_ctx, nonce + 8, 4);
    nonce_counter++;

    size_t tag_offset = pt_len;
    size_t total_len = 12 + pt_len + 16;

    if (total_len > *ct_len) {
        ESP_LOGE(TAG, "Output buffer too small: need %zu, have %zu", total_len, *ct_len);
        return ESP_FAIL;
    }

    /* Nonce header */
    memcpy(ciphertext, nonce, 12);

    /* Encrypt */
    int ret = mbedtls_gcm_crypt_and_tag(&gcm_ctx, MBEDTLS_GCM_ENCRYPT,
                                         pt_len, nonce, 12,
                                         NULL, 0,
                                         plaintext,
                                         ciphertext + 12,
                                         16,
                                         ciphertext + 12 + tag_offset);
    if (ret != 0) {
        ESP_LOGE(TAG, "Encryption failed: %d", ret);
        return ESP_FAIL;
    }

    *ct_len = total_len;
    return ESP_OK;
}

/**
 * @brief Decrypt AES-256-GCM ciphertext.
 */
esp_err_t crypto_decrypt(const uint8_t *ciphertext, size_t ct_len,
                          uint8_t *plaintext, size_t *pt_len)
{
    if (!crypto_initialized || ct_len < 28) { /* 12 nonce + 16 tag minimum */
        return ESP_FAIL;
    }

    const uint8_t *nonce = ciphertext;
    size_t tag_offset = ct_len - 16;
    size_t enc_len = tag_offset - 12;

    if (enc_len > *pt_len) {
        ESP_LOGE(TAG, "Output buffer too small");
        return ESP_FAIL;
    }

    int ret = mbedtls_gcm_auth_decrypt(&gcm_ctx, enc_len,
                                        nonce, 12, NULL, 0,
                                        ciphertext + 12 + enc_len, 16,
                                        ciphertext + 12,
                                        plaintext);
    if (ret != 0) {
        ESP_LOGE(TAG, "Decryption FAILED (auth tag mismatch): %d", ret);
        return ESP_FAIL;
    }

    *pt_len = enc_len;
    return ESP_OK;
}

/* ===================================================================
 * Key Derivation & Hashing
 * =================================================================== */

/**
 * @derive a key from a shared secret and salt using SHA-256.
 */
esp_err_t crypto_derive_key(const uint8_t *secret, size_t secret_len,
                             const uint8_t *salt, size_t salt_len,
                             uint8_t *key_out, size_t key_len)
{
    if (key_len > 32) key_len = 32;

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);

    mbedtls_sha256_update(&sha, salt, salt_len);
    mbedtls_sha256_update(&sha, secret, secret_len);

    uint8_t hash[32];
    mbedtls_sha256_finish(&sha, hash);

    memcpy(key_out, hash, key_len);
    mbedtls_sha256_free(&sha);

    return ESP_OK;
}

/**
 * @brief Generate a random key using the hardware RNG + DRBG.
 */
esp_err_t crypto_generate_key(uint8_t *key, size_t key_len)
{
    if (!crypto_initialized) return ESP_FAIL;

    int ret = mbedtls_ctr_drbg_random(&drbg_ctx, key, key_len);
    return (ret == 0) ? ESP_OK : ESP_FAIL;
}
