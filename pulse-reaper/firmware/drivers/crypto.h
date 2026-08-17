/*
 * crypto.h — ECDH P-256 + AES-256-CTR (session key for BLE C2)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_CRYPTO_H
#define PULSEREAPER_CRYPTO_H

#include <stdint.h>

void crypto_init(void);

/* Derive a session key via ECDH P-256 (placeholder handshake). */
void crypto_derive_session_key(void);

/* AES-256-CTR encrypt/decrypt (same function for both). */
void crypto_aes_ctr_encrypt(const uint8_t *in, uint8_t *out, uint16_t len,
                            const uint8_t *nonce12);
void crypto_aes_ctr_decrypt(const uint8_t *in, uint8_t *out, uint16_t len,
                            const uint8_t *nonce12);

/* Increment a byte counter (big-endian) by 1. */
void crypto_ctr_increment(uint8_t *counter, uint16_t len);

#endif