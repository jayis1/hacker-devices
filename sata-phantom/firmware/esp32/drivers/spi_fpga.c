/**
 * @file spi_fpga.c
 * @author jayis1
 * @brief SPI Driver for FPGA Communication
 *
 * Low-level SPI master driver for communicating with the
 * Gowin GW1N-LV1 FPGA over the ESP32-S3 SPI bus.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "spi_fpga";
static spi_device_handle_t fpga_spi_handle;
static bool spi_initialized = false;

/**
 * @brief Initialize the SPI bus and add FPGA device.
 */
esp_err_t spi_fpga_init(void)
{
    if (spi_initialized) return ESP_OK;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_FPGA_SPI_MOSI,
        .miso_io_num     = PIN_FPGA_SPI_MISO,
        .sclk_io_num     = PIN_FPGA_SPI_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %d", ret);
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .mode           = 0,
        .clock_speed_hz = FPGA_SPI_CLOCK_HZ,
        .spics_io_num   = PIN_FPGA_SPI_CS,
        .queue_size     = 8,
        .flags          = SPI_DEVICE_HALFDUPLEX,
        .pre_cb         = NULL,
        .post_cb        = NULL,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &fpga_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %d", ret);
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    spi_initialized = true;
    ESP_LOGI(TAG, "FPGA SPI driver initialized (%d MHz)", FPGA_SPI_CLOCK_HZ / 1000000);
    return ESP_OK;
}

/**
 * @brief Deinitialize the SPI driver.
 */
void spi_fpga_deinit(void)
{
    if (!spi_initialized) return;
    spi_bus_remove_device(fpga_spi_handle);
    spi_bus_free(SPI2_HOST);
    spi_initialized = false;
}

/**
 * @brief Write a 32-bit register to the FPGA.
 */
int spi_fpga_write_reg(uint16_t addr, uint32_t data)
{
    if (!spi_initialized) return SPI_ERR_BUSY;

    uint8_t tx_buf[6];
    uint16_t encoded = (addr & REG_ADDR_MASK);
    tx_buf[0] = (encoded >> 8) & 0xFF;
    tx_buf[1] = encoded & 0xFF;
    tx_buf[2] = (data >> 24) & 0xFF;
    tx_buf[3] = (data >> 16) & 0xFF;
    tx_buf[4] = (data >> 8) & 0xFF;
    tx_buf[5] = data & 0xFF;

    spi_transaction_t trans = {
        .length    = 48,
        .tx_buffer = tx_buf,
        .rx_buffer = NULL,
    };

    esp_err_t ret = spi_device_transmit(fpga_spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI write failed (addr=0x%04X, data=0x%08X): %d", addr, data, ret);
        return SPI_ERR_TIMEOUT;
    }

    return SPI_OK;
}

/**
 * @brief Read a 32-bit register from the FPGA.
 */
int spi_fpga_read_reg(uint16_t addr, uint32_t *data)
{
    if (!spi_initialized) return SPI_ERR_BUSY;
    if (!data) return SPI_ERR_INVALID_ADDR;

    uint8_t tx_buf[6];
    uint8_t rx_buf[6] = {0};
    uint16_t encoded = (addr & REG_ADDR_MASK) | REG_READ_FLAG;
    tx_buf[0] = (encoded >> 8) & 0xFF;
    tx_buf[1] = encoded & 0xFF;
    tx_buf[2] = 0; tx_buf[3] = 0; tx_buf[4] = 0; tx_buf[5] = 0;

    spi_transaction_t trans = {
        .length    = 48,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_transmit(fpga_spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI read failed (addr=0x%04X): %d", addr, ret);
        return SPI_ERR_TIMEOUT;
    }

    *data = ((uint32_t)rx_buf[2] << 24) |
            ((uint32_t)rx_buf[3] << 16) |
            ((uint32_t)rx_buf[4] << 8)  |
            (uint32_t)rx_buf[5];

    return SPI_OK;
}

/**
 * @brief Burst read multiple registers (useful for scratch buffer reads).
 */
int spi_fpga_read_burst(uint16_t start_addr, uint32_t *data, int count)
{
    if (!spi_initialized || !data || count <= 0) {
        return SPI_ERR_INVALID_ADDR;
    }

    for (int i = 0; i < count; i++) {
        int ret = spi_fpga_read_reg(start_addr + i, &data[i]);
        if (ret != SPI_OK) return ret;
    }
    return SPI_OK;
}

/**
 * @brief Burst write to auto-incrementing registers (scratch buffer).
 */
int spi_fpga_write_burst(uint16_t start_addr, const uint32_t *data, int count)
{
    if (!spi_initialized || !data || count <= 0) {
        return SPI_ERR_INVALID_ADDR;
    }

    for (int i = 0; i < count; i++) {
        int ret = spi_fpga_write_reg(start_addr + i, data[i]);
        if (ret != SPI_OK) return ret;
    }
    return SPI_OK;
}

/**
 * @brief Check if the FPGA is ready and responsive.
 * @return true if FPGA responds with a valid version.
 */
bool spi_fpga_is_ready(void)
{
    uint32_t version = 0;
    if (spi_fpga_read_reg(REG_SYS_VERSION, &version) == SPI_OK) {
        return (version != 0 && version != 0xFFFFFFFF);
    }
    return false;
}

/**
 * @brief Reset the FPGA via register write.
 */
int spi_fpga_soft_reset(void)
{
    ESP_LOGI(TAG, "FPGA soft reset");
    return spi_fpga_write_reg(REG_SYS_RESET, 0xDEADBEEF);
}

/**
 * @brief Get FPGA firmware version as a string.
 */
void spi_fpga_get_version(char *buf, size_t buf_len)
{
    uint32_t version = 0;
    if (spi_fpga_read_reg(REG_SYS_VERSION, &version) == SPI_OK) {
        uint8_t major = (version >> 16) & 0xFF;
        uint8_t minor = (version >> 8) & 0xFF;
        uint8_t patch = version & 0xFF;
        snprintf(buf, buf_len, "v%d.%d.%d", major, minor, patch);
    } else {
        snprintf(buf, buf_len, "UNKNOWN");
    }
}
