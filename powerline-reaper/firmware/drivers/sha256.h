/*
 * sha256.h — SHA-256 interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[8];
    uint64_t len;
    uint8_t  buf[64];
    uint32_t buf_len;
} sha256_context;

void sha256_init(sha256_context *ctx);
void sha256_update(sha256_context *ctx, const uint8_t *data, uint32_t len);
void sha256_final(sha256_context *ctx, uint8_t out[32]);

#ifdef __cplusplus
}
#endif

#endif /* SHA256_H */