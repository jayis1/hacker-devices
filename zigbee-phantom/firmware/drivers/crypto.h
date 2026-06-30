/*
 * drivers/crypto.h — AES-MMO install-code derivation + AES-CCM* for ZIGBEE-PHANTOM
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef ZIGBEE_PHANTOM_CRYPTO_H
#define ZIGBEE_PHANTOM_CRYPTO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Zigbee well-known keys (used as dictionary entries in key recovery) */
#define ZB_WELL_KNOWN_TCLK_LEGACY_LEN  16
extern const uint8_t ZB_TCLK_ZIGBEE_ALLIANCE_09[16];  /* 5A 69 67 42 65 65 ... */

/* Derive Trust Center Link Key from install code via AES-MMO.
 * install_code: 6/8/12/16 bytes (Zigbee 3.0 permits these lengths)
 * tclk_out:     16-byte derived key */
int  zb_crypto_derive_tclk(const uint8_t *install_code, uint8_t ic_len,
                           uint8_t tclk_out[16]);

/* AES-CCM* decrypt of NWK / APS secured frame.
 * key:       16-byte AES key
 * nonce:     13-byte nonce (constructed per Zigbee NWK/APS spec)
 * aad:       additional authenticated data (header)
 * aad_len:   length of aad
 * ciphertext: pointer to ciphertext+MIC buffer
 * ct_len:    ciphertext length (excluding MIC)
 * mic_len:   0/4/8/16
 * plaintext_out: output buffer (>= ct_len)
 * Returns 0 if MIC verifies and decryption succeeds, -1 on auth failure. */
int  zb_crypto_ccm_decrypt(const uint8_t key[16],
                           const uint8_t nonce[13],
                           const uint8_t *aad, uint8_t aad_len,
                           const uint8_t *ciphertext, uint8_t ct_len,
                           uint8_t mic_len,
                           uint8_t *plaintext_out);

/* AES-CCM* encrypt (for forging secured frames). */
int  zb_crypto_ccm_encrypt(const uint8_t key[16],
                           const uint8_t nonce[13],
                           const uint8_t *aad, uint8_t aad_len,
                           const uint8_t *plaintext, uint8_t pt_len,
                           uint8_t mic_len,
                           uint8_t *ciphertext_out,
                           uint8_t *mic_out);

/* Brute-force install codes of a given length against a captured
 * Transport-Key blob. Returns the install code on success (length ic_len),
 * or NULL if not found within max_iterations.
 *   enc_key:    16-byte encrypted network key from Transport-Key frame
 *   tclk_out:   recovered TCLK written here on success
 *   nwk_key_out: recovered network key (decrypt of enc_key) written here
 *   ic_len:     install-code length to brute (4, 8, 12, or 16)
 *   max_iter:   iteration cap
 *   progress:   optional callback every 4096 iters for UI feedback
 */
typedef void (*zb_brute_progress_cb)(uint32_t iter, uint32_t total);
int zb_crypto_brute_install_code(const uint8_t enc_key[16],
                                 uint8_t ic_len,
                                 uint32_t max_iter,
                                 uint8_t install_code_out[16],
                                 uint8_t tclk_out[16],
                                 uint8_t nwk_key_out[16],
                                 zb_brute_progress_cb progress);

/* Construct a Zigbee NWK nonce (13 bytes) from frame ctr, src addr, sec level. */
void zb_crypto_nwk_nonce(uint32_t frame_ctr, const uint8_t src_eui[8],
                         uint8_t sec_level, uint8_t nonce_out[13]);

/* Construct a Zigbee APS nonce (13 bytes) from frame ctr, src_eui, sec level. */
void zb_crypto_aps_nonce(uint32_t frame_ctr, const uint8_t src_eui[8],
                         uint8_t sec_level, uint8_t nonce_out[13]);

/* CRC-16/CCITT for Zigbee install-code integrity check. */
uint16_t zb_crc16_ccitt(const uint8_t *data, uint8_t len);

#endif /* ZIGBEE_PHANTOM_CRYPTO_H */