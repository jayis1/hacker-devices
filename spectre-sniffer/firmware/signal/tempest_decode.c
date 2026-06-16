//==============================================================================
// tempest_decode.c — Spectre-Sniffer Tempest Decoder Engine
// Author: jayis1
// Description: Reconstructs video display content from unintentional
//              electromagnetic emanations (van Eck phreaking). Supports
//              VGA, DVI, HDMI, and LVDS leakage reconstruction.
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

static const char *TAG = "TEMPEST";

//==============================================================================
// Tempest Decoder State
//==============================================================================
typedef struct {
    bool                active;
    tempest_config_t    config;
    uint16_t           *line_buffer;       // Current line being reconstructed
    uint16_t           *frame_buffer;      // Full reconstructed frame
    uint32_t            line_count;
    uint32_t            frame_count;
    float               pixel_clock_estimate;
    float               sync_freq_estimate;
    uint32_t            samples_per_pixel;
    int32_t             sync_threshold;
    bool                sync_detected;
    uint32_t            sync_pulse_count;
    uint32_t            h_sync_samples;
    uint32_t            v_sync_samples;
    float               signal_quality;     // 0.0 - 1.0
} tempest_state_t;

static tempest_state_t s_tempest = {0};

//==============================================================================
// Standard Video Mode Database
//==============================================================================
typedef struct {
    const char *name;
    uint32_t    pixel_clock_hz;
    uint16_t    h_active;
    uint16_t    h_total;
    uint16_t    h_front;
    uint16_t    h_sync;
    uint16_t    h_back;
    uint16_t    v_active;
    uint16_t    v_total;
    uint16_t    v_front;
    uint16_t    v_sync;
    uint16_t    v_back;
    bool        interlaced;
} video_mode_t;

static const video_mode_t KNOWN_MODES[] = {
    {"640x480@60",   25175000,  640, 800,  16,  96,  48,  480, 525,  10, 2,  33, false},
    {"800x600@60",   40000000,  800, 1056, 40,  128, 88,  600, 628,  1,  4,  23, false},
    {"1024x768@60",  65000000,  1024,1344, 24,  136, 160, 768, 806,  3,  6,  29, false},
    {"1280x720@60",  74250000,  1280,1650, 110, 40,  220, 720, 750,  5,  5,  20, false},
    {"1280x1024@60", 108000000, 1280,1688, 48,  112, 248, 1024,1066, 1,  3,  38, false},
    {"1920x1080@60", 148500000, 1920,2200, 88,  44,  148, 1080,1125, 4,  5,  36, false},
    {"1366x768@60",  85500000,  1366,1792, 70,  143, 213, 768, 798,  3,  3,  24, false},
    {"1440x900@60",  106500000, 1440,1904, 80,  152, 232, 900, 934,  1,  3,  30, false},
    {"1600x900@60",  108000000, 1600,1800, 24,  80,  96,  900, 1000, 1,  3,  96, false},
    {"1680x1050@60", 146250000, 1680,2240, 104, 176, 280, 1050,1089, 1,  3,  35, false},
};

#define NUM_KNOWN_MODES (sizeof(KNOWN_MODES) / sizeof(KNOWN_MODES[0]))

//==============================================================================
// Initialize Tempest Decoder
//==============================================================================
int tempest_init(void) {
    ESP_LOGI(TAG, "Initializing Tempest decoder...");

    // Allocate line buffer
    s_tempest.line_buffer = (uint16_t *)heap_caps_malloc(
        TEMPEST_LINE_WIDTH * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!s_tempest.line_buffer) {
        ESP_LOGE(TAG, "Failed to allocate line buffer");
        return SPECTRE_ERR_NOMEM;
    }

    // Allocate frame buffer
    s_tempest.frame_buffer = (uint16_t *)heap_caps_malloc(
        TEMPEST_LINE_WIDTH * TEMPEST_MAX_LINES * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM);
    if (!s_tempest.frame_buffer) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer");
        heap_caps_free(s_tempest.line_buffer);
        return SPECTRE_ERR_NOMEM;
    }

    // Set default config (auto-detect mode)
    s_tempest.config.pixel_clock_hz = 0;
    s_tempest.config.h_resolution = 0;
    s_tempest.config.v_resolution = 0;
    s_tempest.config.interlaced = false;
    s_tempest.config.bit_depth = 8;

    s_tempest.active = false;
    s_tempest.line_count = 0;
    s_tempest.frame_count = 0;
    s_tempest.signal_quality = 0.0f;

    ESP_LOGI(TAG, "Tempest decoder initialized (%d lines x %d pixels buffer)",
             TEMPEST_MAX_LINES, TEMPEST_LINE_WIDTH);
    return SPECTRE_OK;
}

//==============================================================================
// Auto-Detect Video Mode
//==============================================================================
int tempest_auto_detect_mode(const int16_t *samples, uint32_t num_samples,
                              uint32_t sample_rate_hz) {
    if (!samples || num_samples < 10000) return SPECTRE_ERR_INVALID_PARAM;

    ESP_LOGI(TAG, "Auto-detecting video mode from %u samples @ %u Hz...",
             num_samples, sample_rate_hz);

    // Step 1: Detect horizontal sync frequency
    // Look for periodic high-amplitude pulses (sync pulses)
    int32_t threshold = 0;
    int64_t sum = 0;
    int64_t sum_sq = 0;

    for (uint32_t i = 0; i < num_samples; i++) {
        int32_t val = abs(samples[i]);
        sum += val;
        sum_sq += (int64_t)val * val;
    }

    int32_t mean = (int32_t)(sum / num_samples);
    int32_t stddev = (int32_t)sqrt((double)(sum_sq / num_samples - (int64_t)mean * mean));
    threshold = mean + 3 * stddev;

    ESP_LOGI(TAG, "Signal: mean=%d, stddev=%d, threshold=%d", mean, stddev, threshold);

    // Find sync pulse intervals
    uint32_t last_sync = 0;
    uint32_t sync_intervals[256];
    uint32_t num_intervals = 0;
    bool in_sync = false;

    for (uint32_t i = 1000; i < num_samples; i++) {
        if (abs(samples[i]) > threshold && !in_sync) {
            in_sync = true;
            if (last_sync > 0 && num_intervals < 256) {
                sync_intervals[num_intervals++] = i - last_sync;
            }
            last_sync = i;
        }
        if (abs(samples[i]) < threshold / 2) {
            in_sync = false;
        }
    }

    if (num_intervals < 10) {
        ESP_LOGW(TAG, "Could not detect sync pulses (found %d)", num_intervals);
        return SPECTRE_ERR_NOT_FOUND;
    }

    // Average sync interval
    uint64_t total_interval = 0;
    for (uint32_t i = 0; i < num_intervals; i++) {
        total_interval += sync_intervals[i];
    }
    uint32_t avg_interval = (uint32_t)(total_interval / num_intervals);
    float h_sync_freq = (float)sample_rate_hz / avg_interval;

    ESP_LOGI(TAG, "Detected H-sync: %u samples interval = %.2f kHz",
             avg_interval, h_sync_freq / 1000.0f);

    // Step 2: Match against known modes
    int best_match = -1;
    float best_error = 1.0f;

    for (int i = 0; i < NUM_KNOWN_MODES; i++) {
        float expected_h_sync = (float)KNOWN_MODES[i].pixel_clock_hz /
                                KNOWN_MODES[i].h_total;
        float error = fabsf(h_sync_freq - expected_h_sync) / expected_h_sync;

        if (error < best_error) {
            best_error = error;
            best_match = i;
        }
    }

    if (best_match >= 0 && best_error < 0.1f) {
        s_tempest.config.pixel_clock_hz = KNOWN_MODES[best_match].pixel_clock_hz;
        s_tempest.config.h_resolution = KNOWN_MODES[best_match].h_active;
        s_tempest.config.v_resolution = KNOWN_MODES[best_match].v_active;
        s_tempest.config.h_front_porch = KNOWN_MODES[best_match].h_front;
        s_tempest.config.h_sync_pulse = KNOWN_MODES[best_match].h_sync;
        s_tempest.config.h_back_porch = KNOWN_MODES[best_match].h_back;
        s_tempest.config.v_front_porch = KNOWN_MODES[best_match].v_front;
        s_tempest.config.v_sync_pulse = KNOWN_MODES[best_match].v_sync;
        s_tempest.config.v_back_porch = KNOWN_MODES[best_match].v_back;
        s_tempest.config.interlaced = KNOWN_MODES[best_match].interlaced;

        s_tempest.samples_per_pixel = sample_rate_hz / s_tempest.config.pixel_clock_hz;
        s_tempest.sync_threshold = threshold;

        ESP_LOGI(TAG, "Detected mode: %s (error=%.2f%%)",
                 KNOWN_MODES[best_match].name, best_error * 100.0f);
        ESP_LOGI(TAG, "  Pixel clock: %u Hz, %dx%d, %d bpp",
                 s_tempest.config.pixel_clock_hz,
                 s_tempest.config.h_resolution,
                 s_tempest.config.v_resolution,
                 s_tempest.config.bit_depth);
        ESP_LOGI(TAG, "  Samples per pixel: %u", s_tempest.samples_per_pixel);

        s_tempest.signal_quality = 1.0f - best_error;
        return SPECTRE_OK;
    }

    ESP_LOGW(TAG, "Could not match video mode (best error=%.2f%%)", best_error * 100.0f);
    return SPECTRE_ERR_NOT_FOUND;
}

//==============================================================================
// Decode One Line of Video
//==============================================================================
int tempest_decode_line(const int16_t *samples, uint32_t num_samples,
                        uint32_t sample_rate_hz) {
    if (!samples || !s_tempest.active) return SPECTRE_ERR_NOT_INIT;
    if (s_tempest.config.pixel_clock_hz == 0) return SPECTRE_ERR_NOT_INIT;

    uint32_t samples_per_pixel = s_tempest.samples_per_pixel;
    if (samples_per_pixel == 0) samples_per_pixel = 1;

    // Decode pixels from the EM signal
    // Each pixel's brightness is proportional to the EM amplitude at that position
    uint32_t pixels_decoded = 0;
    uint32_t max_pixels = (s_tempest.config.h_resolution < TEMPEST_LINE_WIDTH) ?
                           s_tempest.config.h_resolution : TEMPEST_LINE_WIDTH;

    for (uint32_t i = 0; i < num_samples && pixels_decoded < max_pixels;
         i += samples_per_pixel) {
        // Average samples for this pixel
        int32_t pixel_sum = 0;
        uint32_t count = 0;

        for (uint32_t j = 0; j < samples_per_pixel && (i + j) < num_samples; j++) {
            pixel_sum += abs(samples[i + j]);
            count++;
        }

        if (count > 0) {
            int32_t pixel_val = pixel_sum / count;
            // Normalize to 16-bit range
            if (pixel_val > 32767) pixel_val = 32767;
            s_tempest.line_buffer[pixels_decoded++] = (uint16_t)pixel_val;
        }
    }

    // Copy to frame buffer
    if (s_tempest.line_count < TEMPEST_MAX_LINES) {
        uint32_t offset = s_tempest.line_count * TEMPEST_LINE_WIDTH;
        memcpy(&s_tempest.frame_buffer[offset], s_tempest.line_buffer,
               pixels_decoded * sizeof(uint16_t));
        s_tempest.line_count++;
    }

    return (int)pixels_decoded;
}

//==============================================================================
// Detect Frame Boundaries
//==============================================================================
bool tempest_detect_frame_boundary(const int16_t *samples, uint32_t num_samples) {
    // Look for vertical sync interval (longer than H-sync)
    // V-sync is typically multiple consecutive H-sync pulses
    if (!samples || num_samples < 1000) return false;

    uint32_t consecutive_syncs = 0;
    uint32_t required_syncs = 3;  // V-sync is at least 3 H-sync periods

    for (uint32_t i = 0; i < num_samples; i++) {
        if (abs(samples[i]) > s_tempest.sync_threshold) {
            consecutive_syncs++;
            if (consecutive_syncs > s_tempest.samples_per_pixel * 10) {
                // Long sync = V-sync
                if (s_tempest.line_count > s_tempest.config.v_resolution / 2) {
                    // We have enough lines for a frame
                    s_tempest.frame_count++;
                    s_tempest.line_count = 0;
                    return true;
                }
            }
        } else {
            consecutive_syncs = 0;
        }
    }

    return false;
}

//==============================================================================
// Get Reconstructed Frame
//==============================================================================
const uint16_t *tempest_get_frame(uint32_t *width, uint32_t *height) {
    if (width) *width = s_tempest.config.h_resolution;
    if (height) *height = s_tempest.line_count;
    return s_tempest.frame_buffer;
}

//==============================================================================
// Start/Stop Tempest Decoding
//==============================================================================
int tempest_start(const tempest_config_t *config) {
    if (!config) return SPECTRE_ERR_INVALID_PARAM;

    memcpy(&s_tempest.config, config, sizeof(tempest_config_t));
    s_tempest.samples_per_pixel = ADC_SAMPLE_RATE_HZ / config->pixel_clock_hz;
    s_tempest.line_count = 0;
    s_tempest.frame_count = 0;
    s_tempest.active = true;

    ESP_LOGI(TAG, "Tempest decoding started: %dx%d @ %u Hz pixel clock",
             config->h_resolution, config->v_resolution, config->pixel_clock_hz);
    return SPECTRE_OK;
}

int tempest_stop(void) {
    s_tempest.active = false;
    ESP_LOGI(TAG, "Tempest decoding stopped (%d frames captured)", s_tempest.frame_count);
    return SPECTRE_OK;
}

bool tempest_is_active(void) {
    return s_tempest.active;
}

float tempest_get_signal_quality(void) {
    return s_tempest.signal_quality;
}

uint32_t tempest_get_frame_count(void) {
    return s_tempest.frame_count;
}

//==============================================================================
// Save Frame to SD Card
//==============================================================================
int tempest_save_frame(const char *filename) {
    if (!filename) return SPECTRE_ERR_INVALID_PARAM;

    // PPM format (simple, uncompressed)
    FILE *f = fopen(filename, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", filename);
        return SPECTRE_ERR_FILE;
    }

    // Write PPM header
    fprintf(f, "P6\n%u %u\n255\n",
            s_tempest.config.h_resolution, s_tempest.line_count);

    // Write pixel data (convert 16-bit EM amplitude to 8-bit grayscale)
    for (uint32_t y = 0; y < s_tempest.line_count; y++) {
        for (uint32_t x = 0; x < s_tempest.config.h_resolution; x++) {
            uint16_t val = s_tempest.frame_buffer[y * TEMPEST_LINE_WIDTH + x];
            uint8_t pixel = (uint8_t)(val >> 8);  // Scale to 8-bit
            fwrite(&pixel, 1, 1, f);  // R
            fwrite(&pixel, 1, 1, f);  // G
            fwrite(&pixel, 1, 1, f);  // B
        }
    }

    fclose(f);
    ESP_LOGI(TAG, "Frame saved to %s (%ux%u)", filename,
             s_tempest.config.h_resolution, s_tempest.line_count);
    return SPECTRE_OK;
}
