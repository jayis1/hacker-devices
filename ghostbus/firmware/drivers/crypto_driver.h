/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Crypto Engine — AES-256-GCM transport & ECDH P-256 session key
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GHOSTBUS_CRYPTO_DRIVER_H
#define GHOSTBUS_CRYPTO_DRIVER_H

#include "registers.h"
#include "board.h"

#define AES_BLOCK_SIZE     16
#define AES_256_KEY_SIZE   32
#define GCM_TAG_SIZE       16
#define GCM_NONCE_SIZE     12

typedef enum {
    CRYPTO_ENCRYPT = 0,
    CRYPTO_DECRYPT = 1
} crypto_op_t;

/* Public API */
void          crypto_init(void);
gb_status_t   crypto_aes_gcm(const uint8_t *key, uint32_t key_len,
                              const uint8_t *nonce, uint32_t nonce_len,
                              const uint8_t *aad, uint32_t aad_len,
                              const uint8_t *in, uint32_t in_len,
                              uint8_t *out, uint8_t *tag,
                              crypto_op_t op);
gb_status_t   crypto_ecdh_p256(const uint8_t *peer_pubkey,
                                uint8_t *shared_secret);
gb_status_t   crypto_get_pubkey(uint8_t *pubkey);
gb_status_t   crypto_sha256(const uint8_t *data, uint32_t len,
                             uint8_t *out_hash);
gb_status_t   crypto_random_bytes(uint8_t *out, uint32_t len);
uint32_t      crypto_crc32(uint32_t crc, const uint8_t *buf, uint32_t len);

#endif /* GHOSTBUS_CRYPTO_DRIVER_H */