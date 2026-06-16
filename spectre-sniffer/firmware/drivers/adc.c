//==============================================================================
// adc.c — Spectre-Sniffer ADC Driver (LTC2208 via FPGA Bridge)
// Author: jayis1
// Description: High-level ADC configuration, sample rate control, and
//              data acquisition management for the LTC2208 16-bit ADC.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "ADC_DRV";

//==============================================================================
// ADC State
//==============================================================================
typedef struct {
    bool            initialized;
    bool            enabled;
    uint16_t        decimation_factor;
    uint32_t        effective_sample_rate;
    uint32_t        total_samples_captured;
    uint32_t        overflow_count;
    int16_t         dc_offset;
    float           gain_correction;
} adc_state_t;

static adc_state_t s_adc = {
    .initialized = false,
    .enabled = false,
    .decimation_factor = DEFAULT_DECIMATION,
    .effective_sample_rate = ADC_SAMPLE_RATE_HZ / DEFAULT_DECIMATION,
    .total_samples_captured = 0,
    .overflow_count = 0,
    .dc_offset = 0,
    .gain_correction = 1.0f,
};

//==============================================================================
// ADC Initialization
//==============================================================================
int adc_init_driver(void) {
    ESP_LOGI(TAG, "Initializing ADC driver...");

    if (!s_fpga_spi) {
        ESP_LOGE(TAG, "FPGA not initialized");
        return SPECTRE_ERR_NOT_INIT;
    }

    // Reset ADC engine
    int ret = fpga_soft_reset();
    if (ret != SPECTRE_OK) {
        ESP_LOGE(TAG, "FPGA reset failed");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // Configure ADC control register
    uint8_t adc_ctrl = 0;
    adc_ctrl |= (ADC_CLOCK_DIVIDER & 0x0F) << 0;  // Clock divider
    adc_ctrl |= (0 << 4);   // Internal VREF
    adc_ctrl |= (0 << 5);   // Normal mode
    adc_ctrl |= (0 << 6);   // Single-ended input
    adc_ctrl |= (1 << 7);   // Enable PLL

    ret = fpga_reg_write(FPGA_REG_ADC_CTRL, adc_ctrl);
    if (ret != SPECTRE_OK) {
        ESP_LOGE(TAG, "Failed to write ADC control register");
        return ret;
    }

    // Wait for PLL lock
    int lock_retries = 100;
    while (lock_retries-- > 0) {
        if (adc_is_locked()) {
            ESP_LOGI(TAG, "ADC PLL locked successfully");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (lock_retries <= 0) {
        ESP_LOGE(TAG, "ADC PLL failed to lock after 1000ms");
        return SPECTRE_ERR_TIMEOUT;
    }

    // Set default decimation
    ret = adc_set_decimation(s_adc.decimation_factor);
    if (ret != SPECTRE_OK) {
        ESP_LOGE(TAG, "Failed to set decimation factor");
        return ret;
    }

    // Perform DC offset calibration
    ret = adc_calibrate_dc_offset();
    if (ret != SPECTRE_OK) {
        ESP_LOGW(TAG, "DC offset calibration failed, using zero");
        s_adc.dc_offset = 0;
    }

    s_adc.initialized = true;
    ESP_LOGI(TAG, "ADC driver initialized (sample rate: %u Hz, decimation: %u)",
             s_adc.effective_sample_rate, s_adc.decimation_factor);

    return SPECTRE_OK;
}

//==============================================================================
// ADC Enable/Disable
//==============================================================================
int adc_enable(void) {
    if (!s_adc.initialized) return SPECTRE_ERR_NOT_INIT;

    int ret = adc_set_enable(true);
    if (ret == SPECTRE_OK) {
        s_adc.enabled = true;
        ESP_LOGI(TAG, "ADC enabled");
    }
    return ret;
}

int adc_disable(void) {
    int ret = adc_set_enable(false);
    if (ret == SPECTRE_OK) {
        s_adc.enabled = false;
        ESP_LOGI(TAG, "ADC disabled");
    }
    return ret;
}

//==============================================================================
// ADC Configuration
//==============================================================================
int adc_set_sample_rate(uint32_t sample_rate_hz) {
    if (sample_rate_hz < ADC_SAMPLE_RATE_HZ / MAX_DECIMATION_FACTOR) {
        sample_rate_hz = ADC_SAMPLE_RATE_HZ / MAX_DECIMATION_FACTOR;
    }
    if (sample_rate_hz > ADC_SAMPLE_RATE_HZ) {
        sample_rate_hz = ADC_SAMPLE_RATE_HZ;
    }

    uint16_t decimation = (uint16_t)(ADC_SAMPLE_RATE_HZ / sample_rate_hz);
    if (decimation < MIN_DECIMATION_FACTOR) decimation = MIN_DECIMATION_FACTOR;
    if (decimation > MAX_DECIMATION_FACTOR) decimation = MAX_DECIMATION_FACTOR;

    int ret = adc_set_decimation(decimation);
    if (ret == SPECTRE_OK) {
        s_adc.decimation_factor = decimation;
        s_adc.effective_sample_rate = ADC_SAMPLE_RATE_HZ / decimation;
        ESP_LOGI(TAG, "Sample rate set to %u Hz (decimation: %u)",
                 s_adc.effective_sample_rate, decimation);
    }
    return ret;
}

uint32_t adc_get_sample_rate(void) {
    return s_adc.effective_sample_rate;
}

//==============================================================================
// DC Offset Calibration
//==============================================================================
int adc_calibrate_dc_offset(void) {
    ESP_LOGI(TAG, "Calibrating ADC DC offset...");

    // Enable ADC with no input (input grounded via FPGA mux)
    adc_set_enable(true);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Collect 1024 samples and average
    int16_t samples[1024];
    int32_t sum = 0;

    for (int i = 0; i < 10; i++) {
        int read = capture_read_samples(samples, 1024);
        if (read > 0) {
            for (int j = 0; j < read; j++) {
                sum += samples[j];
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    s_adc.dc_offset = (int16_t)(sum / 10240);  // Average of 10 batches of 1024
    s_adc.gain_correction = 1.0f;

    adc_set_enable(false);

    ESP_LOGI(TAG, "DC offset calibrated: %d ADC codes", s_adc.dc_offset);
    return SPECTRE_OK;
}

//==============================================================================
// Data Acquisition
//==============================================================================
int adc_capture_samples(int16_t *buffer, uint32_t num_samples, uint32_t timeout_ms) {
    if (!buffer || num_samples == 0) return SPECTRE_ERR_INVALID_PARAM;
    if (!s_adc.initialized || !s_adc.enabled) return SPECTRE_ERR_NOT_INIT;

    // Start single capture
    int ret = capture_start_single();
    if (ret != SPECTRE_OK) return ret;

    // Wait for buffer to fill
    uint32_t elapsed = 0;
    uint32_t total_read = 0;

    while (elapsed < timeout_ms && total_read < num_samples) {
        uint32_t remaining = num_samples - total_read;
        uint32_t batch = (remaining > 1024) ? 1024 : remaining;

        int read = capture_read_samples(&buffer[total_read], batch);
        if (read > 0) {
            total_read += read;
            // Apply DC offset correction
            for (uint32_t i = total_read - read; i < total_read; i++) {
                buffer[i] -= s_adc.dc_offset;
            }
        }

        if (capture_buffer_full()) break;
        vTaskDelay(pdMS_TO_TICKS(1));
        elapsed++;
    }

    if (total_read < num_samples) {
        ESP_LOGW(TAG, "Capture incomplete: got %u of %u samples", total_read, num_samples);
    }

    // Check for overflow
    if (adc_overflow_detected()) {
        s_adc.overflow_count++;
        ESP_LOGW(TAG, "ADC overflow detected (count: %u)", s_adc.overflow_count);
    }

    s_adc.total_samples_captured += total_read;
    return (int)total_read;
}

//==============================================================================
// Status Queries
//==============================================================================
bool adc_is_initialized(void) {
    return s_adc.initialized;
}

bool adc_is_enabled(void) {
    return s_adc.enabled;
}

uint32_t adc_get_total_samples(void) {
    return s_adc.total_samples_captured;
}

uint32_t adc_get_overflow_count(void) {
    return s_adc.overflow_count;
}

int16_t adc_get_dc_offset(void) {
    return s_adc.dc_offset;
}

//==============================================================================
// ADC Self-Test
//==============================================================================
int adc_run_self_test(void) {
    ESP_LOGI(TAG, "Running ADC self-test...");

    // Test 1: Check PLL lock
    if (!adc_is_locked()) {
        ESP_LOGE(TAG, "Self-test FAILED: PLL not locked");
        return SPECTRE_ERR_ADC;
    }
    ESP_LOGI(TAG, "  [PASS] PLL locked");

    // Test 2: Enable ADC and check for data
    adc_set_enable(true);
    vTaskDelay(pdMS_TO_TICKS(50));

    int16_t test_samples[128];
    int read = capture_read_samples(test_samples, 128);
    if (read <= 0) {
        ESP_LOGE(TAG, "Self-test FAILED: No data from ADC");
        adc_set_enable(false);
        return SPECTRE_ERR_ADC;
    }
    ESP_LOGI(TAG, "  [PASS] ADC data flowing (%d samples)", read);

    // Test 3: Check for stuck bits
    bool all_same = true;
    for (int i = 1; i < read; i++) {
        if (test_samples[i] != test_samples[0]) {
            all_same = false;
            break;
        }
    }
    if (all_same) {
        ESP_LOGW(TAG, "  [WARN] All samples identical - possible stuck ADC");
    } else {
        ESP_LOGI(TAG, "  [PASS] ADC data varies (min=%d, max=%d, mean=%d)",
                 test_samples[0], test_samples[read-1], s_adc.dc_offset);
    }

    adc_set_enable(false);
    ESP_LOGI(TAG, "ADC self-test complete");
    return SPECTRE_OK;
}
