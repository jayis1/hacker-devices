/*
 * signature.h - Current signature analysis for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_SIGNATURE_H
#define POE_WHISPER_SIGNATURE_H

#include "../board.h"

typedef struct {
    float samples_ma[PW_MAX_CURRENT_SAMPLES];
    size_t count;
    float mean_ma;
    float peak_ma;
    float variance;
    char inferred_state[40];
} pw_signature_result_t;

void pw_signature_reset(pw_signature_result_t *result);
void pw_signature_feed(pw_signature_result_t *result, float sample_ma);
void pw_signature_finalize(pw_signature_result_t *result);
float pw_signature_anomaly_score(const pw_signature_result_t *result, float expected_mean_ma);

#endif
