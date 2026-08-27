/*
 * signature.c - Current signature analysis for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "signature.h"

void pw_signature_reset(pw_signature_result_t *result)
{
    memset(result, 0, sizeof(*result));
    snprintf(result->inferred_state, sizeof(result->inferred_state), "unknown");
}

void pw_signature_feed(pw_signature_result_t *result, float sample_ma)
{
    if (result->count < PW_MAX_CURRENT_SAMPLES) {
        result->samples_ma[result->count++] = sample_ma;
    }
}

void pw_signature_finalize(pw_signature_result_t *result)
{
    float sum = 0.0f;
    result->peak_ma = 0.0f;
    for (size_t i = 0; i < result->count; ++i) {
        sum += result->samples_ma[i];
        if (result->samples_ma[i] > result->peak_ma) {
            result->peak_ma = result->samples_ma[i];
        }
    }
    result->mean_ma = result->count ? sum / (float)result->count : 0.0f;

    float variance_sum = 0.0f;
    for (size_t i = 0; i < result->count; ++i) {
        float delta = result->samples_ma[i] - result->mean_ma;
        variance_sum += delta * delta;
    }
    result->variance = result->count ? variance_sum / (float)result->count : 0.0f;

    if (result->peak_ma > 900.0f && result->variance > 20000.0f) {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "motor-or-ir-burst");
    } else if (result->mean_ma > 520.0f && result->variance > 6000.0f) {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "radio-boot");
    } else if (result->mean_ma > 250.0f) {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "steady-online");
    } else {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "idle-or-low-power");
    }
}

float pw_signature_anomaly_score(const pw_signature_result_t *result, float expected_mean_ma)
{
    float mean_delta = fabsf(result->mean_ma - expected_mean_ma);
    float variance_term = sqrtf(result->variance) * 0.05f;
    float peak_term = result->peak_ma > expected_mean_ma ? (result->peak_ma - expected_mean_ma) * 0.02f : 0.0f;
    return mean_delta * 0.08f + variance_term + peak_term;
}
