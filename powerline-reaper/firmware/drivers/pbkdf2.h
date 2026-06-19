/*
 * pbkdf2.h — PBKDF2-HMAC-SHA256 interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PBKDF2_H
#define PBKDF2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pbkdf2_hmac_sha256(const uint8_t *pass, uint32_t pass_len,
                          const uint8_t *salt, uint32_t salt_len,
                          uint32_t iters, uint8_t *out, uint32_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* PBKDF2_H */