//==============================================================================
// dpa_engine.c — Spectre-Sniffer Differential Power Analysis Engine
// Author: jayis1
// Description: Implements Differential Power Analysis (DPA) for recovering
//              cryptographic keys from EM side-channel traces. Supports
//              AES-128, AES-256, DES, and 3DES algorithms.
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

static const char *TAG = "DPA";

//==============================================================================
// DPA Engine State
//==============================================================================
typedef struct {
    bool                initialized;
    bool                analysis_running;
    uint32_t            num_traces;
    uint32_t            trace_length;
    uint32_t            num_key_bytes;
    uint8_t             target_algorithm;
    uint8_t             leakage_model;
    int16_t            *traces;            // All captured traces [num_traces][trace_length]
    uint8_t            *plaintexts;         // Known plaintexts [num_traces][16]
    uint8_t             key_candidates[16]; // Recovered key bytes
    float               key_confidence[16]; // Confidence score per key byte
    float              *differential_trace; // Differential trace for current hypothesis
    uint32_t            total_hypotheses;
    uint32_t            hypotheses_tested;
    volatile float      progress;           // 0.0 - 1.0
} dpa_state_t;

static dpa_state_t s_dpa = {0};

//==============================================================================
// AES S-Box (for DPA intermediate value computation)
//==============================================================================
static const uint8_t AES_SBOX[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
};

//==============================================================================
// DPA Initialization
//==============================================================================
int dpa_init(uint32_t max_traces, uint32_t trace_len) {
    ESP_LOGI(TAG, "Initializing DPA engine (max %u traces, %u samples each)...",
             max_traces, trace_len);

    if (max_traces > MAX_TRACES) max_traces = MAX_TRACES;
    if (trace_len > TRACE_LENGTH) trace_len = TRACE_LENGTH;

    // Allocate trace storage in PSRAM
    size_t trace_size = max_traces * trace_len * sizeof(int16_t);
    s_dpa.traces = (int16_t *)heap_caps_malloc(trace_size, MALLOC_CAP_SPIRAM);
    if (!s_dpa.traces) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes for traces", trace_size);
        return SPECTRE_ERR_NOMEM;
    }

    // Allocate plaintext storage
    s_dpa.plaintexts = (uint8_t *)heap_caps_malloc(
        max_traces * 16 * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    if (!s_dpa.plaintexts) {
        ESP_LOGE(TAG, "Failed to allocate plaintext storage");
        heap_caps_free(s_dpa.traces);
        return SPECTRE_ERR_NOMEM;
    }

    // Allocate differential trace buffer
    s_dpa.differential_trace = (float *)heap_caps_malloc(
        trace_len * sizeof(float), MALLOC_CAP_SPIRAM);
    if (!s_dpa.differential_trace) {
        ESP_LOGE(TAG, "Failed to allocate differential trace buffer");
        heap_caps_free(s_dpa.traces);
        heap_caps_free(s_dpa.plaintexts);
        return SPECTRE_ERR_NOMEM;
    }

    s_dpa.num_traces = 0;
    s_dpa.trace_length = trace_len;
    s_dpa.num_key_bytes = 16;  // AES-128 default
    s_dpa.target_algorithm = 0;  // AES-128
    s_dpa.leakage_model = 0;  // Hamming weight
    s_dpa.analysis_running = false;
    s_dpa.initialized = true;

    memset(s_dpa.key_candidates, 0, sizeof(s_dpa.key_candidates));
    memset(s_dpa.key_confidence, 0, sizeof(s_dpa.key_confidence));

    ESP_LOGI(TAG, "DPA engine initialized (%zu MB trace buffer)",
             trace_size / (1024 * 1024));
    return SPECTRE_OK;
}

//==============================================================================
// Add a Trace to the Dataset
//==============================================================================
int dpa_add_trace(const int16_t *trace, const uint8_t *plaintext) {
    if (!s_dpa.initialized) return SPECTRE_ERR_NOT_INIT;
    if (!trace || !plaintext) return SPECTRE_ERR_INVALID_PARAM;
    if (s_dpa.num_traces >= MAX_TRACES) return SPECTRE_ERR_NOMEM;

    uint32_t offset = s_dpa.num_traces * s_dpa.trace_length;
    memcpy(&s_dpa.traces[offset], trace, s_dpa.trace_length * sizeof(int16_t));
    memcpy(&s_dpa.plaintexts[s_dpa.num_traces * 16], plaintext, 16);

    s_dpa.num_traces++;
    return SPECTRE_OK;
}

//==============================================================================
// Compute Hamming Weight (number of 1 bits)
//==============================================================================
static inline uint8_t hamming_weight(uint8_t x) {
    // Popcount using bit manipulation
    x = (x & 0x55) + ((x >> 1) & 0x55);
    x = (x & 0x33) + ((x >> 2) & 0x33);
    x = (x & 0x0F) + ((x >> 4) & 0x0F);
    return x;
}

//==============================================================================
// Compute Intermediate Value for DPA Selection Function
//==============================================================================
static uint8_t dpa_intermediate_value(uint8_t plaintext_byte, uint8_t key_hypothesis) {
    // AES SubBytes: S-box(plaintext XOR key)
    return AES_SBOX[plaintext_byte ^ key_hypothesis];
}

//==============================================================================
// Run DPA for a Single Key Byte
//==============================================================================
static int dpa_attack_key_byte(uint8_t key_byte_idx, uint8_t *best_key,
                                float *best_confidence) {
    if (key_byte_idx >= 16) return SPECTRE_ERR_INVALID_PARAM;

    float max_differential = 0.0f;
    uint8_t best_hypothesis = 0;

    // Test all 256 key hypotheses for this byte
    for (uint16_t hypothesis = 0; hypothesis < 256; hypothesis++) {
        // Clear differential trace
        memset(s_dpa.differential_trace, 0, s_dpa.trace_length * sizeof(float));

        // Separate traces into two sets based on selection function
        uint32_t set1_count = 0;
        uint32_t set0_count = 0;

        // Accumulate traces for each set
        for (uint32_t t = 0; t < s_dpa.num_traces; t++) {
            uint8_t pt = s_dpa.plaintexts[t * 16 + key_byte_idx];
            uint8_t intermediate = dpa_intermediate_value(pt, (uint8_t)hypothesis);
            uint8_t hw = hamming_weight(intermediate);

            // Selection function: bit 0 of Hamming weight
            // (can be changed to target specific bit positions)
            bool selection = (hw & 0x01) != 0;

            uint32_t trace_offset = t * s_dpa.trace_length;

            if (selection) {
                for (uint32_t s = 0; s < s_dpa.trace_length; s++) {
                    s_dpa.differential_trace[s] += (float)s_dpa.traces[trace_offset + s];
                }
                set1_count++;
            } else {
                for (uint32_t s = 0; s < s_dpa.trace_length; s++) {
                    s_dpa.differential_trace[s] -= (float)s_dpa.traces[trace_offset + s];
                }
                set0_count++;
            }
        }

        // Compute differential trace (average of set1 - average of set0)
        if (set1_count > 0 && set0_count > 0) {
            float max_abs = 0.0f;
            for (uint32_t s = 0; s < s_dpa.trace_length; s++) {
                s_dpa.differential_trace[s] /= (float)set1_count;
                float abs_val = fabsf(s_dpa.differential_trace[s]);
                if (abs_val > max_abs) {
                    max_abs = abs_val;
                }
            }

            // Track the best hypothesis (highest peak in differential trace)
            if (max_abs > max_differential) {
                max_differential = max_abs;
                best_hypothesis = (uint8_t)hypothesis;
            }
        }

        s_dpa.hypotheses_tested++;
    }

    *best_key = best_hypothesis;
    *best_confidence = max_differential;

    return SPECTRE_OK;
}

//==============================================================================
// Run Full DPA Attack
//==============================================================================
int dpa_run_attack(crypto_analysis_config_t *config) {
    if (!s_dpa.initialized) return SPECTRE_ERR_NOT_INIT;
    if (!config) return SPECTRE_ERR_INVALID_PARAM;
    if (s_dpa.num_traces < 10) {
        ESP_LOGE(TAG, "Not enough traces (%u, need at least 10)", s_dpa.num_traces);
        return SPECTRE_ERR_INVALID_PARAM;
    }

    s_dpa.analysis_running = true;
    s_dpa.total_hypotheses = 256 * 16;  // 256 hypotheses × 16 bytes
    s_dpa.hypotheses_tested = 0;
    s_dpa.progress = 0.0f;

    ESP_LOGI(TAG, "Starting DPA attack: %u traces, %u key bytes, %u total hypotheses",
             s_dpa.num_traces, 16, s_dpa.total_hypotheses);

    // Attack each key byte independently
    for (uint8_t byte_idx = 0; byte_idx < 16; byte_idx++) {
        uint8_t best_key = 0;
        float confidence = 0.0f;

        int ret = dpa_attack_key_byte(byte_idx, &best_key, &confidence);
        if (ret != SPECTRE_OK) {
            ESP_LOGE(TAG, "DPA failed on key byte %d", byte_idx);
            s_dpa.analysis_running = false;
            return ret;
        }

        s_dpa.key_candidates[byte_idx] = best_key;
        s_dpa.key_confidence[byte_idx] = confidence;

        s_dpa.progress = (float)(byte_idx + 1) / 16.0f;

        ESP_LOGI(TAG, "Key byte %2d: 0x%02X (confidence: %.2f)",
                 byte_idx, best_key, confidence);
    }

    s_dpa.analysis_running = false;
    s_dpa.progress = 1.0f;

    ESP_LOGI(TAG, "DPA attack complete!");
    ESP_LOGI(TAG, "Recovered key: %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X",
             s_dpa.key_candidates[0], s_dpa.key_candidates[1],
             s_dpa.key_candidates[2], s_dpa.key_candidates[3],
             s_dpa.key_candidates[4], s_dpa.key_candidates[5],
             s_dpa.key_candidates[6], s_dpa.key_candidates[7],
             s_dpa.key_candidates[8], s_dpa.key_candidates[9],
             s_dpa.key_candidates[10], s_dpa.key_candidates[11],
             s_dpa.key_candidates[12], s_dpa.key_candidates[13],
             s_dpa.key_candidates[14], s_dpa.key_candidates[15]);

    return SPECTRE_OK;
}

//==============================================================================
// Get Results
//==============================================================================
int dpa_get_key(uint8_t *key, uint16_t key_len) {
    if (!s_dpa.initialized) return SPECTRE_ERR_NOT_INIT;
    if (!key || key_len < 16) return SPECTRE_ERR_INVALID_PARAM;

    memcpy(key, s_dpa.key_candidates, 16);
    return SPECTRE_OK;
}

float dpa_get_confidence(uint8_t key_byte_idx) {
    if (key_byte_idx >= 16) return 0.0f;
    return s_dpa.key_confidence[key_byte_idx];
}

float dpa_get_progress(void) {
    return s_dpa.progress;
}

bool dpa_is_running(void) {
    return s_dpa.analysis_running;
}

uint32_t dpa_get_num_traces(void) {
    return s_dpa.num_traces;
}

//==============================================================================
// Save Traces to SD Card
//==============================================================================
int dpa_save_traces(const char *filename) {
    if (!s_dpa.initialized) return SPECTRE_ERR_NOT_INIT;
    if (!filename) return SPECTRE_ERR_INVALID_PARAM;

    FILE *f = fopen(filename, "wb");
    if (!f) return SPECTRE_ERR_FILE;

    // Write header
    uint32_t header[4] = {
        s_dpa.num_traces,
        s_dpa.trace_length,
        16,  // plaintext length
        0    // flags
    };
    fwrite(header, sizeof(header), 1, f);

    // Write traces
    fwrite(s_dpa.traces, sizeof(int16_t),
           s_dpa.num_traces * s_dpa.trace_length, f);

    // Write plaintexts
    fwrite(s_dpa.plaintexts, sizeof(uint8_t),
           s_dpa.num_traces * 16, f);

    fclose(f);
    ESP_LOGI(TAG, "Saved %u traces to %s", s_dpa.num_traces, filename);
    return SPECTRE_OK;
}

//==============================================================================
// Cleanup
//==============================================================================
void dpa_deinit(void) {
    if (s_dpa.traces) {
        heap_caps_free(s_dpa.traces);
        s_dpa.traces = NULL;
    }
    if (s_dpa.plaintexts) {
        heap_caps_free(s_dpa.plaintexts);
        s_dpa.plaintexts = NULL;
    }
    if (s_dpa.differential_trace) {
        heap_caps_free(s_dpa.differential_trace);
        s_dpa.differential_trace = NULL;
    }
    s_dpa.initialized = false;
    ESP_LOGI(TAG, "DPA engine deinitialized");
}
