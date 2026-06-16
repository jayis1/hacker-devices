//==============================================================================
// fpga.c — Spectre-Sniffer FPGA Interface Driver
// Author: jayis1
// Description: Low-level SPI register access, bitstream loading, and
//              FPGA configuration management for the Lattice iCE40UP5K.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "FPGA_DRV";

//==============================================================================
// SPI Device Handle
//==============================================================================
static spi_device_handle_t s_fpga_spi = NULL;

//==============================================================================
// FPGA Register Access Implementation
//==============================================================================

int fpga_reg_read(uint8_t reg, uint8_t *value) {
    if (!s_fpga_spi || !value) return SPECTRE_ERR_NOT_INIT;

    uint8_t tx_buf[2] = { reg | REG_FLAG_READ, 0x00 };
    uint8_t rx_buf[2] = { 0, 0 };

    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_transmit(s_fpga_spi, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI read failed (reg=0x%02X, err=%d)", reg, ret);
        return SPECTRE_ERR_IO;
    }

    *value = rx_buf[1];
    return SPECTRE_OK;
}

int fpga_reg_write(uint8_t reg, uint8_t value) {
    if (!s_fpga_spi) return SPECTRE_ERR_NOT_INIT;

    uint8_t tx_buf[2] = { reg | REG_FLAG_WRITE, value };

    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = tx_buf,
        .rx_buffer = NULL,
    };

    esp_err_t ret = spi_device_transmit(s_fpga_spi, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI write failed (reg=0x%02X, val=0x%02X, err=%d)",
                 reg, value, ret);
        return SPECTRE_ERR_IO;
    }

    return SPECTRE_OK;
}

int fpga_reg_read16(uint8_t reg, uint16_t *value) {
    if (!value) return SPECTRE_ERR_INVALID_PARAM;

    uint8_t low, high;
    int ret = fpga_reg_read(reg, &low);
    if (ret != SPECTRE_OK) return ret;
    ret = fpga_reg_read(reg + 1, &high);
    if (ret != SPECTRE_OK) return ret;

    *value = ((uint16_t)high << 8) | low;
    return SPECTRE_OK;
}

int fpga_reg_write16(uint8_t reg, uint16_t value) {
    int ret = fpga_reg_write(reg, (uint8_t)(value & 0xFF));
    if (ret != SPECTRE_OK) return ret;
    return fpga_reg_write(reg + 1, (uint8_t)((value >> 8) & 0xFF));
}

int fpga_reg_read32(uint8_t reg, uint32_t *value) {
    if (!value) return SPECTRE_ERR_INVALID_PARAM;

    uint8_t bytes[4];
    for (int i = 0; i < 4; i++) {
        int ret = fpga_reg_read(reg + i, &bytes[i]);
        if (ret != SPECTRE_OK) return ret;
    }

    *value = ((uint32_t)bytes[3] << 24) |
             ((uint32_t)bytes[2] << 16) |
             ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[0] << 0);
    return SPECTRE_OK;
}

int fpga_reg_write32(uint8_t reg, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        int ret = fpga_reg_write(reg + i, (uint8_t)((value >> (i * 8)) & 0xFF));
        if (ret != SPECTRE_OK) return ret;
    }
    return SPECTRE_OK;
}

//==============================================================================
// FFT Bin Read (Burst Mode)
//==============================================================================
int fft_read_bins(uint16_t bin_index, uint16_t *data, uint16_t num_bins) {
    if (!data || num_bins == 0 || num_bins > 64) {
        return SPECTRE_ERR_INVALID_PARAM;
    }

    // Set FFT bin start index
    int ret = fpga_reg_write16(FPGA_REG_FFT_CTRL, bin_index);
    if (ret != SPECTRE_OK) return ret;

    // Read bins in burst mode
    for (uint16_t i = 0; i < num_bins; i++) {
        uint8_t low, high;
        ret = fpga_reg_read(FPGA_REG_FFT_CTRL + 2 + (i * 2), &low);
        if (ret != SPECTRE_OK) return ret;
        ret = fpga_reg_read(FPGA_REG_FFT_CTRL + 2 + (i * 2) + 1, &high);
        if (ret != SPECTRE_OK) return ret;
        data[i] = ((uint16_t)high << 8) | low;
    }

    return SPECTRE_OK;
}

//==============================================================================
// Capture Sample Read
//==============================================================================
int capture_read_samples(int16_t *buffer, uint32_t num_samples) {
    if (!buffer || num_samples == 0) return SPECTRE_ERR_INVALID_PARAM;

    // Read samples from FPGA data FIFO
    for (uint32_t i = 0; i < num_samples; i++) {
        uint8_t low, high;
        int ret = fpga_reg_read(FPGA_REG_BUFFER_CTRL, &low);
        if (ret != SPECTRE_OK) return ret;
        ret = fpga_reg_read(FPGA_REG_BUFFER_CTRL + 1, &high);
        if (ret != SPECTRE_OK) return ret;
        buffer[i] = (int16_t)(((uint16_t)high << 8) | low);
    }

    return (int)num_samples;
}

//==============================================================================
// FPGA Built-In Self-Test
//==============================================================================
int fpga_run_bist(void) {
    ESP_LOGI(TAG, "Running FPGA BIST...");

    // Enable loopback mode
    int ret = fpga_set_loopback(true);
    if (ret != SPECTRE_OK) return ret;

    // Write test pattern
    uint8_t test_pattern[] = {
        0xAA, 0x55, 0x00, 0xFF, 0x12, 0x34, 0x56, 0x78,
        0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67
    };

    for (int i = 0; i < sizeof(test_pattern); i++) {
        ret = fpga_reg_write(FPGA_REG_DEBUG, test_pattern[i]);
        if (ret != SPECTRE_OK) {
            ESP_LOGE(TAG, "BIST write failed at index %d", i);
            fpga_set_loopback(false);
            return SPECTRE_ERR_FPGA;
        }

        uint8_t readback;
        ret = fpga_reg_read(FPGA_REG_DEBUG, &readback);
        if (ret != SPECTRE_OK) {
            ESP_LOGE(TAG, "BIST read failed at index %d", i);
            fpga_set_loopback(false);
            return SPECTRE_ERR_FPGA;
        }

        if (readback != test_pattern[i]) {
            ESP_LOGE(TAG, "BIST mismatch at index %d: wrote 0x%02X, read 0x%02X",
                     i, test_pattern[i], readback);
            fpga_set_loopback(false);
            return SPECTRE_ERR_FPGA;
        }
    }

    // Disable loopback
    fpga_set_loopback(false);

    ESP_LOGI(TAG, "FPGA BIST passed (%d bytes verified)", sizeof(test_pattern));
    return SPECTRE_OK;
}

//==============================================================================
// FPGA Initialization (SPI device setup)
//==============================================================================
int fpga_init_spi_device(void) {
    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 20 * 1000 * 1000,  // 20 MHz for normal operation
        .spics_io_num = FPGA_SPI_CS_GPIO,
        .queue_size = 4,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    esp_err_t ret = spi_bus_add_device(FPGA_SPI_HOST, &dev_cfg, &s_fpga_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add FPGA SPI device: %d", ret);
        return SPECTRE_ERR_IO;
    }

    ESP_LOGI(TAG, "FPGA SPI device initialized (20 MHz, mode 0)");
    return SPECTRE_OK;
}
