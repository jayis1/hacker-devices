//==============================================================================
// cpa_engine.c — Spectre-Sniffer Correlation Power Analysis Engine
// Author: jayis1
// Description: Implements Correlation Power Analysis (CPA) using Pearson
//              correlation coefficient between hypothetical power models
//              and actual EM traces for key recovery.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "board.h"

static const char *TAG = "CPA";

//==============================================================================
// CPA State
//==============================================================================
typedef struct {
    bool        initialized;
    bool        running;
    uint32_t    num_traces;
    uint32_t    trace_length;
    int16_t    *traces;
    uint8_t    *plaintexts;
    uint8_t     key_candidates[16];
    float       key_confidence[16];
    float       progress;
} cpa_state_t;

static cpa_state_t s_cpa = {0};

//==============================================================================
// AES S-Box (shared with DPA engine)
//==============================================================================
extern const uint8_t AES_SBOX[256];

//==============================================================================
// Initialize CPA Engine
//==============================================================================
int cpa_init(uint32_t max_traces, uint32_t trace_len) {
    ESP_LOGI(TAG, "Initializing CPA engine...");

    if (max_traces > MAX_TRACES) max_traces = MAX_TRACES;
    if (trace_len > TRACE_LENGTH) trace_len = TRACE_LENGTH;

    size_t trace_size = max_traces * trace_len * sizeof(int16_t);
    s_cpa.traces = (int16_t *)heap_caps_malloc(trace_size, MALLOC_CAP_SPIRAM);
    if (!s_cpa.traces) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes for traces", trace_size);
        return SPECTRE_ERR_NOMEM;
    }

    s_cpa.plaintexts = (uint8_t *)heap_caps_malloc(
        max_traces * 16 * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    if (!s_cpa.plaintexts) {
        heap_caps_free(s_cpa.traces);
        return SPECTRE_ERR_NOMEM;
    }

    s_cpa.num_traces = 0;
    s_cpa.trace_length = trace_len;
    s_cpa.initialized = true;
    s_cpa.running = false;

    memset(s_cpa.key_candidates, 0, sizeof(s_cpa.key_candidates));
    memset(s_cpa.key_confidence, 0, sizeof(s_cpa.key_confidence));

    ESP_LOGI(TAG, "CPA engine initialized");
    return SPECTRE_OK;
}

//==============================================================================
// Add Trace
//==============================================================================
int cpa_add_trace(const int16_t *trace, const uint8_t *plaintext) {
    if (!s_cpa.initialized) return SPECTRE_ERR_NOT_INIT;
    if (!trace || !plaintext) return SPECTRE_ERR_INVALID_PARAM;
    if (s_cpa.num_traces >= MAX_TRACES) return SPECTRE_ERR_NOMEM;

    uint32_t offset = s_cpa.num_traces * s_cpa.trace_length;
    memcpy(&s_cpa.traces[offset], trace, s_cpa.trace_length * sizeof(int16_t));
    memcpy(&s_cpa.plaintexts[s_cpa.num_traces * 16], plaintext, 16);
    s_cpa.num_traces++;

    return SPECTRE_OK;
}

//==============================================================================
// Hamming Weight
//==============================================================================
static inline uint8_t hw(uint8_t x) {
    x = (x & 0x55) + ((x >> 1) & 0x55);
    x = (x & 0x33) + ((x >> 2) & 0x33);
    x = (x & 0x0F) + ((x >> 4) & 0x0F);
    return x;
}

//==============================================================================
// Pearson Correlation Coefficient
//==============================================================================
static float pearson_correlation(const int16_t *traces, const float *hypothesis,
                                  uint32_t num_traces, uint32_t sample_point) {
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0, sum_y2 = 0;

    for (uint32_t t = 0; t < num_traces; t++) {
        float x = (float)traces[t * s_cpa.trace_length + sample_point];
        float y = hypothesis[t];

        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
        sum_y2 += y * y;
    }

    float n = (float)num_traces;
    float numerator = n * sum_xy - sum_x * sum_y;
    float denom_x = n * sum_x2 - sum_x * sum_x;
    float denom_y = n * sum_y2 - sum_y * sum_y;
    float denominator = sqrtf(denom_x * denom_y);

    if (denominator == 0.0f) return 0.0f;
    return numerator / denominator;
}

//==============================================================================
// Run CPA Attack on One Key Byte
//==============================================================================
static int cpa_attack_key_byte(uint8_t key_byte_idx, uint8_t *best_key,
                                float *best_correlation) {
    float max_corr = 0.0f;
    uint8_t best_hypothesis = 0;

    // Allocate hypothesis array
    float *hypothesis = (float *)heap_caps_malloc(
        s_cpa.num_traces * sizeof(float), MALLOC_CAP_SPIRAM);
    if (!hypothesis) return SPECTRE_ERR_NOMEM;

    // Test all 256 key hypotheses
    for (uint16_t hyp = 0; hyp < 256; hyp++) {
        // Build hypothesis array: Hamming weight of S-box(plaintext XOR key)
        for (uint32_t t = 0; t < s_cpa.num_traces; t++) {
            uint8_t pt = s_cpa.plaintexts[t * 16 + key_byte_idx];
            uint8_t intermediate = AES_SBOX[pt ^ (uint8_t)hyp];
            hypothesis[t] = (float)hw(intermediate);
        }

        // Find maximum correlation across all sample points
        float max_corr_for_hyp = 0.0f;
        for (uint32_t s = 0; s < s_cpa.trace_length; s++) {
            float corr = pearson_correlation(s_cpa.traces, hypothesis,
                                              s_cpa.num_traces, s);
            if (fabsf(corr) > max_corr_for_hyp) {
                max_corr_for_hyp = fabsf(corr);
            }
        }

        if (max_corr_for_hyp > max_corr) {
            max_corr = max_corr_for_hyp;
            best_hypothesis = (uint8_t)hyp;
        }
    }

    heap_caps_free(hypothesis);

    *best_key = best_hypothesis;
    *best_correlation = max_corr;
    return SPECTRE_OK;
}

//==============================================================================
// Run Full CPA Attack
//==============================================================================
int cpa_run_attack(crypto_analysis_config_t *config) {
    if (!s_cpa.initialized) return SPECTRE_ERR_NOT_INIT;
    if (!config) return SPECTRE_ERR_INVALID_PARAM;
    if (s_cpa.num_traces < 10) {
        ESP_LOGE(TAG, "Not enough traces (%u)", s_cpa.num_traces);
        return SPECTRE_ERR_INVALID_PARAM;
    }

    s_cpa.running = true;
    s_cpa.progress = 0.0f;

    ESP_LOGI(TAG, "Starting CPA attack: %u traces, %u key bytes",
             s_cpa.num_traces, 16);

    for (uint8_t byte_idx = 0; byte_idx < 16; byte_idx++) {
        uint8_t best_key = 0;
        float correlation = 0.0f;

        int ret = cpa_attack_key_byte(byte_idx, &best_key, &correlation);
        if (ret != SPECTRE_OK) {
            s_cpa.running = false;
            return ret;
        }

        s_cpa.key_candidates[byte_idx] = best_key;
        s_cpa.key_confidence[byte_idx] = correlation;
        s_cpa.progress = (float)(byte_idx + 1) / 16.0f;

        ESP_LOGI(TAG, "Key byte %2d: 0x%02X (corr: %.4f)",
                 byte_idx, best_key, correlation);
    }

    s_cpa.running = false;
    s_cpa.progress = 1.0f;

    ESP_LOGI(TAG, "CPA attack complete!");
    return SPECTRE_OK;
}

//==============================================================================
// Get Results
//==============================================================================
int cpa_get_key(uint8_t *key, uint16_t key_len) {
    if (!s_cpa.initialized) return SPECTRE_ERR_NOT_INIT;
    if (!key || key_len < 16) return SPECTRE_ERR_INVALID_PARAM;
    memcpy(key, s_cpa.key_candidates, 16);
    return SPECTRE_OK;
}

float cpa_get_confidence(uint8_t key_byte_idx) {
    if (key_byte_idx >= 16) return 0.0f;
    return s_cpa.key_confidence[key_byte_idx];
}

float cpa_get_progress(void) {
    return s_cpa.progress;
}

bool cpa_is_running(void) {
    return s_cpa.running;
}

uint32_t cpa_get_num_traces(void) {
    return s_cpa.num_traces;
}

//==============================================================================
// Cleanup
//==============================================================================
void cpa_deinit(void) {
    if (s_cpa.traces) {
        heap_caps_free(s_cpa.traces);
        s_cpa.traces = NULL;
    }
    if (s_cpa.plaintexts) {
        heap_caps_free(s_cpa.plaintexts);
        s_cpa.plaintexts = NULL;
    }
    s_cpa.initialized = false;
    ESP_LOGI(TAG, "CPA engine deinitialized");
}
