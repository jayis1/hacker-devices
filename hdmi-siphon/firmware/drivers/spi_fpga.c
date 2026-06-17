/**
 * @file    spi_fpga.c
 * @brief   SPI master driver for FPGA (iCE40UP5K) register access
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Implements the 4-wire SPI master interface to the FPGA. All FPGA
 * registers are accessed via 32-bit transactions over SPI at 20 MHz.
 * Provides register read/write, bit manipulation, burst transfers,
 * and polling with timeout.
 */

#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "board.h"
#include "registers.h"
#include "spi_fpga.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "SPI_FPGA";

/** SPI device handle */
static spi_device_handle_t s_spi_handle = NULL;

/** SPI transaction descriptor (reused for all transactions) */
static spi_transaction_t s_trans;

/** Lock for SPI bus access */
static SemaphoreHandle_t s_spi_lock = NULL;

/* =========================================================================
 * Initialization
 * ========================================================================= */

esp_err_t spi_fpga_init(void)
{
    ESP_LOGI(TAG, "Initializing SPI-FPGA master interface");

    /* Configure SPI bus */
    spi_bus_config_t buscfg = {
        .mosi_io_num     = PIN_SPI_FPGA_MOSI,
        .miso_io_num     = PIN_SPI_FPGA_MISO,
        .sclk_io_num     = PIN_SPI_FPGA_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = SPI_BURST_MAX * 4,
        .flags           = SPICOMMON_BUSFLAG_MASTER
    };

    /* Configure SPI device (FPGA as slave) */
    spi_device_interface_config_t devcfg = {
        .mode           = 0,               /* CPOL=0, CPHA=0 */
        .clock_speed_hz = CONFIG_SPI_FPGA_FREQ_HZ,
        .spics_io_num   = PIN_SPI_FPGA_CS,
        .queue_size     = 8,
        .flags          = SPI_DEVICE_HALFDUPLEX,
        .pre_cb         = NULL,
        .post_cb        = NULL,
    };

    /* Initialize the SPI bus (shared with SD card if needed) */
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Attach FPGA device to bus */
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Create SPI lock */
    s_spi_lock = xSemaphoreCreateMutex();
    if (s_spi_lock == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI mutex");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "SPI-FPGA initialized at %u Hz", CONFIG_SPI_FPGA_FREQ_HZ);
    return ESP_OK;
}

/* =========================================================================
 * Single Register Access
 * ========================================================================= */

void reg_write(uint16_t reg_addr, uint32_t value)
{
    uint8_t tx_buf[4];

    /* Build write transaction: [CMD] [addr_low] [data_hi] [data_lo] */
    tx_buf[0] = SPI_CMD_WRITE | ((reg_addr >> 8) & 0x0F);
    tx_buf[1] = reg_addr & 0xFF;
    tx_buf[2] = (value >> 24) & 0xFF;
    tx_buf[3] = (value)       & 0xFF;

    if (xSemaphoreTake(s_spi_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "reg_write(0x%04X) — SPI lock timeout", reg_addr);
        return;
    }

    memset(&s_trans, 0, sizeof(s_trans));
    s_trans.length    = 8 * 4;  /* 4 bytes, 8 bits each */
    s_trans.tx_buffer = tx_buf;
    s_trans.rx_buffer = NULL;

    esp_err_t ret = spi_device_transmit(s_spi_handle, &s_trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "reg_write(0x%04X) failed: %s", reg_addr, esp_err_to_name(ret));
    }

    xSemaphoreGive(s_spi_lock);
}

uint32_t reg_read(uint16_t reg_addr)
{
    uint8_t tx_buf[4];
    uint8_t rx_buf[4] = {0};
    uint32_t value = 0;

    /* Build read transaction: [CMD] [addr_low] [dummy] [dummy] */
    tx_buf[0] = SPI_CMD_READ | ((reg_addr >> 8) & 0x0F);
    tx_buf[1] = reg_addr & 0xFF;
    tx_buf[2] = 0x00;  /* dummy byte */
    tx_buf[3] = 0x00;  /* dummy byte */

    if (xSemaphoreTake(s_spi_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "reg_read(0x%04X) — SPI lock timeout", reg_addr);
        return 0xFFFFFFFF;
    }

    memset(&s_trans, 0, sizeof(s_trans));
    s_trans.length    = 8 * 4;
    s_trans.tx_buffer = tx_buf;
    s_trans.rx_buffer = rx_buf;

    esp_err_t ret = spi_device_transmit(s_spi_handle, &s_trans);
    if (ret == ESP_OK) {
        /* FPGA returns data on last 2 bytes of MISO during transaction */
        value = ((uint32_t)rx_buf[2] << 8) | (uint32_t)rx_buf[3];
    } else {
        ESP_LOGE(TAG, "reg_read(0x%04X) failed: %s", reg_addr, esp_err_to_name(ret));
        value = 0xFFFFFFFF;
    }

    xSemaphoreGive(s_spi_lock);
    return value;
}

/* =========================================================================
 * Bit Manipulation
 * ========================================================================= */

void reg_set_bits(uint16_t reg_addr, uint32_t bitmask)
{
    uint32_t current = reg_read(reg_addr);
    if (current == 0xFFFFFFFF) {
        ESP_LOGW(TAG, "reg_set_bits(0x%04X) — read failed", reg_addr);
        return;
    }
    reg_write(reg_addr, current | bitmask);
}

void reg_clear_bits(uint16_t reg_addr, uint32_t bitmask)
{
    uint32_t current = reg_read(reg_addr);
    if (current == 0xFFFFFFFF) {
        ESP_LOGW(TAG, "reg_clear_bits(0x%04X) — read failed", reg_addr);
        return;
    }
    reg_write(reg_addr, current & ~bitmask);
}

bool reg_wait_for_bit(uint16_t reg_addr, uint32_t bitmask, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 1;  /* ms */

    while (elapsed < timeout_ms) {
        uint32_t val = reg_read(reg_addr);
        if (val != 0xFFFFFFFF && (val & bitmask) == bitmask) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(poll_interval));
        elapsed += poll_interval;
    }

    ESP_LOGW(TAG, "reg_wait_for_bit(0x%04X, 0x%08" PRIX32 ") timed out after %" PRIu32 " ms",
             reg_addr, bitmask, timeout_ms);
    return false;
}

/* =========================================================================
 * Burst Transfers
 * ========================================================================= */

uint32_t reg_burst_read(uint16_t start_addr, uint32_t *buffer, uint32_t count)
{
    if (buffer == NULL || count == 0 || count > SPI_BURST_MAX) {
        return 0;
    }

    if (xSemaphoreTake(s_spi_lock, pdMS_TO_TICKS(500)) != pdTRUE) {
        ESP_LOGW(TAG, "reg_burst_read — SPI lock timeout");
        return 0;
    }

    /* Burst read: send start address, then clock out N words */
    uint8_t cmd_buf[2];
    cmd_buf[0] = SPI_CMD_READ | ((start_addr >> 8) & 0x0F);
    cmd_buf[1] = start_addr & 0xFF;

    /* Combined transaction: command + data read */
    memset(&s_trans, 0, sizeof(s_trans));
    s_trans.length    = 8 * 2 + 8 * count * 4;  /* cmd (2B) + data (count * 4B) */
    s_trans.tx_buffer = cmd_buf;
    s_trans.rx_buffer = (uint8_t *)buffer;

    esp_err_t ret = spi_device_transmit(s_spi_handle, &s_trans);
    xSemaphoreGive(s_spi_lock);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "reg_burst_read failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return count;
}

uint32_t reg_burst_write(uint16_t start_addr, const uint32_t *buffer, uint32_t count)
{
    if (buffer == NULL || count == 0 || count > SPI_BURST_MAX) {
        return 0;
    }

    /* Prepare combined command + data buffer */
    uint8_t *tx_buf = malloc(2 + count * 4);
    if (tx_buf == NULL) {
        ESP_LOGE(TAG, "reg_burst_write — malloc failed for %u bytes", 2 + count * 4);
        return 0;
    }

    tx_buf[0] = SPI_CMD_WRITE | ((start_addr >> 8) & 0x0F);
    tx_buf[1] = start_addr & 0xFF;
    memcpy(tx_buf + 2, buffer, count * 4);

    if (xSemaphoreTake(s_spi_lock, pdMS_TO_TICKS(500)) != pdTRUE) {
        ESP_LOGW(TAG, "reg_burst_write — SPI lock timeout");
        free(tx_buf);
        return 0;
    }

    memset(&s_trans, 0, sizeof(s_trans));
    s_trans.length    = 8 * (2 + count * 4);
    s_trans.tx_buffer = tx_buf;
    s_trans.rx_buffer = NULL;

    esp_err_t ret = spi_device_transmit(s_spi_handle, &s_trans);
    xSemaphoreGive(s_spi_lock);

    free(tx_buf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "reg_burst_write failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return count;
}

/**
 * @brief Reset the SPI-FPGA interface (reinitialize device)
 */
esp_err_t spi_fpga_reset(void)
{
    ESP_LOGW(TAG, "Resetting SPI-FPGA interface");

    /* Remove and re-add device */
    esp_err_t ret = spi_bus_remove_device(s_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t devcfg = {
        .mode           = 0,
        .clock_speed_hz = CONFIG_SPI_FPGA_FREQ_HZ,
        .spics_io_num   = PIN_SPI_FPGA_CS,
        .queue_size     = 8,
        .flags          = SPI_DEVICE_HALFDUPLEX,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to re-add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI-FPGA interface reset complete");
    return ESP_OK;
}
