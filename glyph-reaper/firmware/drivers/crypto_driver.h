/**
 * @file crypto_driver.h
 * @brief AES-256-GCM encryption driver for frame data exfiltration
 *
 * Uses the nRF52840 CryptoCell-310 hardware accelerator for
 * AES-256-GCM encryption of all exfiltrated frame data.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef CRYPTO_DRIVER_H
#define CRYPTO_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize crypto subsystem
 */
void crypto_driver_init(void);

/**
 * @brief Generate a random AES-256 key
 * @param key Output buffer (32 bytes)
 */
void crypto_generate_key(uint8_t key[32]);

/**
 * @brief Generate a random IV
 * @param iv Output buffer (12 bytes)
 */
void crypto_generate_iv(uint8_t iv[12]);

/**
 * @brief Derive a new key from an existing key (key rotation)
 * @param old_key Current key (32 bytes)
 * @param new_key Output new key (32 bytes)
 */
void crypto_derive_key(const uint8_t old_key[32], uint8_t new_key[32]);

/**
 * @brief Encrypt data with AES-256-GCM
 * @param plaintext Input data
 * @param plaintext_len Length of input
 * @param ciphertext Output buffer (must be plaintext_len + 28 for IV + tag)
 * @param key AES-256 key
 * @param iv Initialization vector (12 bytes)
 * @return Total ciphertext size (IV + encrypted data + GCM tag), 0 on error
 */
uint32_t crypto_encrypt(const uint8_t *plaintext, uint32_t plaintext_len,
                        uint8_t *ciphertext, const uint8_t key[32],
                        const uint8_t iv[12]);

/**
 * @brief Decrypt data with AES-256-GCM
 * @param ciphertext Input data (IV + encrypted + tag)
 * @param ciphertext_len Length of input
 * @param plaintext Output buffer
 * @param key AES-256 key
 * @return Length of decrypted data, 0 on error (auth failure)
 */
uint32_t crypto_decrypt(const uint8_t *ciphertext, uint32_t ciphertext_len,
                        uint8_t *plaintext, const uint8_t key[32]);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_DRIVER_H */