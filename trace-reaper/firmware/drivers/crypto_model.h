/*
 * TRACE-REAPER — crypto_model driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef CRYPTO_MODEL_H
#define CRYPTO_MODEL_H

#include <stdint.h>
#include "../board.h"

uint8_t hw8(uint8_t x);
uint8_t crypto_model_eval(leak_model_t m, uint8_t i, uint8_t k, uint8_t t);
uint8_t crypto_model_des(uint8_t sbox_idx, uint8_t k6);
void crypto_model_hyp_vector(leak_model_t m, uint8_t i, uint8_t t,
                             uint8_t out[256]);
float corr_accum_rho(const corr_accum_t *a);
float crypto_model_best(const corr_byte_t *b, uint8_t *out_hyp);
void crypto_model_fold(corr_byte_t *table, uint8_t nbytes,
                       const uint8_t hyp[KEY_BYTES_AES256][256],
                       const int16_t *trace, uint16_t nsamp,
                       const uint16_t poi[KEY_BYTES_AES256]);
void crypto_model_reset(corr_byte_t *table, uint8_t nbytes);

#endif /* CRYPTO_MODEL_H */