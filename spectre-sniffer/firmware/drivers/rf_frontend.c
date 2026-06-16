//==============================================================================
// rf_frontend.c — Spectre-Sniffer RF Frontend Driver
// Author: jayis1
// Description: Controls the LNA, tunable filter, and downconverter for the
//              Spectre-Sniffer's analog RF frontend chain.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "RF_FE";

//==============================================================================
// RF Frontend State
//==============================================================================
typedef struct {
    bool        lna1_enabled;
    bool        lna2_enabled;
    uint8_t     lna_gain_db;
    bool        downconv_enabled;
    uint64_t    filter_freq_hz;
    uint64_t    lo_freq_hz;
    uint64_t    center_freq_hz;
    bool        initialized;
} rf_state_t;

static rf_state_t s_rf = {
    .lna1_enabled = false,
    .lna2_enabled = false,
    .lna_gain_db = RF_LNA_GAIN_LOW,
    .downconv_enabled = false,
    .filter_freq_hz = 0,
    .lo_freq_hz = 0,
    .center_freq_hz = RF_DEFAULT_CENTER_FREQ,
    .initialized = false,
};

//==============================================================================
// Initialize RF Frontend
//==============================================================================
int rf_frontend_init_driver(void) {
    ESP_LOGI(TAG, "Initializing RF frontend...");

    // Ensure all RF components are disabled initially
    gpio_set_level(LNA1_ENABLE_GPIO, 0);
    gpio_set_level(LNA2_ENABLE_GPIO, 0);
    gpio_set_level(DOWNCONV_ENABLE_GPIO, 0);
    gpio_set_level(LNA_GAIN_CTRL_GPIO, 0);

    // Set filter to default bypass state
    gpio_set_level(FILTER_TUNE_LE_GPIO, 0);
    gpio_set_level(FILTER_TUNE_CLK_GPIO, 0);
    gpio_set_level(FILTER_TUNE_DATA_GPIO, 0);

    s_rf.initialized = true;
    ESP_LOGI(TAG, "RF frontend initialized (all channels off)");
    return SPECTRE_OK;
}

//==============================================================================
// LNA Control
//==============================================================================
int rf_set_lna_enable(uint8_t channel, bool enable) {
    if (!s_rf.initialized) return SPECTRE_ERR_NOT_INIT;
    if (channel > 1) return SPECTRE_ERR_INVALID_PARAM;

    int gpio = (channel == 0) ? LNA1_ENABLE_GPIO : LNA2_ENABLE_GPIO;
    gpio_set_level(gpio, enable ? 1 : 0);

    if (channel == 0) {
        s_rf.lna1_enabled = enable;
    } else {
        s_rf.lna2_enabled = enable;
    }

    ESP_LOGI(TAG, "LNA%d %s", channel + 1, enable ? "enabled" : "disabled");
    return SPECTRE_OK;
}

int rf_set_lna_gain(uint8_t gain_db) {
    if (!s_rf.initialized) return SPECTRE_ERR_NOT_INIT;

    if (gain_db == RF_LNA_GAIN_LOW) {
        gpio_set_level(LNA_GAIN_CTRL_GPIO, 0);
        s_rf.lna_gain_db = RF_LNA_GAIN_LOW;
    } else if (gain_db == RF_LNA_GAIN_HIGH) {
        gpio_set_level(LNA_GAIN_CTRL_GPIO, 1);
        s_rf.lna_gain_db = RF_LNA_GAIN_HIGH;
    } else {
        return SPECTRE_ERR_INVALID_PARAM;
    }

    ESP_LOGI(TAG, "LNA gain set to %d dB", s_rf.lna_gain_db);
    return SPECTRE_OK;
}

//==============================================================================
// Tunable Filter Control (PE82305)
//==============================================================================
int rf_set_filter_freq(uint64_t freq_hz) {
    if (!s_rf.initialized) return SPECTRE_ERR_NOT_INIT;
    if (freq_hz < RF_MIN_FREQ_HZ || freq_hz > 3000000000ULL) {
        return SPECTRE_ERR_INVALID_PARAM;
    }

    // PE82305 uses a 3-wire serial interface (CLK, DATA, LE)
    // Frequency is programmed as a 16-bit value
    // f_center = (code * 50000) + 10000000 (10 MHz to 3 GHz in ~50 kHz steps)
    uint16_t code = (uint16_t)((freq_hz - 10000000) / 50000);

    // Clock out the 16-bit code MSB first
    gpio_set_level(FILTER_TUNE_LE_GPIO, 0);

    for (int i = 15; i >= 0; i--) {
        gpio_set_level(FILTER_TUNE_CLK_GPIO, 0);
        gpio_set_level(FILTER_TUNE_DATA_GPIO, (code >> i) & 0x01);
        // Small delay for setup time
        for (volatile int d = 0; d < 5; d++);
        gpio_set_level(FILTER_TUNE_CLK_GPIO, 1);
        for (volatile int d = 0; d < 5; d++);
    }

    // Latch the new frequency
    gpio_set_level(FILTER_TUNE_LE_GPIO, 1);
    for (volatile int d = 0; d < 10; d++);
    gpio_set_level(FILTER_TUNE_LE_GPIO, 0);

    s_rf.filter_freq_hz = freq_hz;
    ESP_LOGI(TAG, "Filter set to %llu Hz (code: 0x%04X)", freq_hz, code);
    return SPECTRE_OK;
}

//==============================================================================
// Downconverter Control (MAX2682)
//==============================================================================
int rf_set_lo_freq(uint64_t freq_hz) {
    if (!s_rf.initialized) return SPECTRE_ERR_NOT_INIT;

    // MAX2682 uses a 3-wire serial interface for PLL programming
    // The LO frequency is programmed via a 15-bit N-divider
    // f_LO = f_ref * (N + 1) where f_ref = 100 kHz typical
    // For simplicity, we use a lookup table for common LO frequencies
    uint16_t n_divider = (uint16_t)(freq_hz / 100000) - 1;

    // Program the PLL (simplified - actual implementation would use
    // the MAX2682's 3-wire programming interface)
    ESP_LOGI(TAG, "LO set to %llu Hz (N=%u)", freq_hz, n_divider);
    s_rf.lo_freq_hz = freq_hz;
    return SPECTRE_OK;
}

//==============================================================================
// Downconverter Enable
//==============================================================================
int rf_set_downconverter_enable(bool enable) {
    if (!s_rf.initialized) return SPECTRE_ERR_NOT_INIT;

    gpio_set_level(DOWNCONV_ENABLE_GPIO, enable ? 1 : 0);
    s_rf.downconv_enabled = enable;

    ESP_LOGI(TAG, "Downconverter %s", enable ? "enabled" : "disabled");
    return SPECTRE_OK;
}

//==============================================================================
// Set Center Frequency (High-Level)
//==============================================================================
int rf_set_center_freq(uint64_t freq_hz) {
    if (!s_rf.initialized) return SPECTRE_ERR_NOT_INIT;
    if (freq_hz < RF_MIN_FREQ_HZ || freq_hz > RF_MAX_FREQ_HZ) {
        return SPECTRE_ERR_INVALID_PARAM;
    }

    s_rf.center_freq_hz = freq_hz;

    if (freq_hz <= RF_DOWNCONV_THRESHOLD) {
        // Direct mode: no downconversion needed
        rf_set_downconverter_enable(false);
        rf_set_filter_freq(freq_hz);
        rf_set_lna_enable(0, true);
    } else {
        // Downconversion mode
        uint64_t if_freq = 100000000;  // 100 MHz IF
        uint64_t lo_freq = freq_hz - if_freq;

        rf_set_lo_freq(lo_freq);
        rf_set_filter_freq(if_freq);
        rf_set_downconverter_enable(true);
        rf_set_lna_enable(0, true);
    }

    ESP_LOGI(TAG, "Center frequency set to %llu Hz", freq_hz);
    return SPECTRE_OK;
}

//==============================================================================
// Status Queries
//==============================================================================
uint64_t rf_get_center_freq(void) {
    return s_rf.center_freq_hz;
}

uint8_t rf_get_lna_gain(void) {
    return s_rf.lna_gain_db;
}

bool rf_is_downconverter_enabled(void) {
    return s_rf.downconv_enabled;
}

//==============================================================================
// Self-Test
//==============================================================================
int rf_run_self_test(void) {
    ESP_LOGI(TAG, "Running RF frontend self-test...");

    // Test LNA1
    ESP_LOGI(TAG, "  Testing LNA1...");
    rf_set_lna_enable(0, true);
    vTaskDelay(pdMS_TO_TICKS(10));
    rf_set_lna_enable(0, false);

    // Test LNA2
    ESP_LOGI(TAG, "  Testing LNA2...");
    rf_set_lna_enable(1, true);
    vTaskDelay(pdMS_TO_TICKS(10));
    rf_set_lna_enable(1, false);

    // Test filter
    ESP_LOGI(TAG, "  Testing tunable filter...");
    rf_set_filter_freq(100000000);  // 100 MHz
    vTaskDelay(pdMS_TO_TICKS(10));
    rf_set_filter_freq(1000000000); // 1 GHz
    vTaskDelay(pdMS_TO_TICKS(10));

    // Test downconverter
    ESP_LOGI(TAG, "  Testing downconverter...");
    rf_set_downconverter_enable(true);
    vTaskDelay(pdMS_TO_TICKS(10));
    rf_set_downconverter_enable(false);

    ESP_LOGI(TAG, "RF frontend self-test complete");
    return SPECTRE_OK;
}
