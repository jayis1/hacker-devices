/*
 * chacha20poly1305.h — ChaCha20-Poly1305 AEAD interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHACHA20POLY1305_H
#define CHACHA20POLY1305_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void chacha20_xor(const uint8_t key[32], uint32_t counter,
                   const uint8_t nonce[12], uint8_t *buf, uint32_t len);
void poly1305(const uint8_t key[32], const uint8_t *msg, uint32_t len, uint8_t out[16]);

int  chacha20poly1305_encrypt(const uint8_t key[32], uint32_t counter,
                                const uint8_t nonce[12],
                                const uint8_t *aad, uint32_t aad_len,
                                uint8_t *buf, uint32_t len,
                                uint8_t tag[16]);
int  chacha20poly1305_decrypt(const uint8_t key[32], uint32_t counter,
                                const uint8_t nonce[12],
                                const uint8_t *aad, uint32_t aad_len,
                                uint8_t *buf, uint32_t len,
                                const uint8_t tag[16]);

#ifdef __cplusplus
}
#endif

#endif /* CHACHA20POLY1305_H */