/**
 * @file    edid_manager.c
 * @brief   EDID management — read, inject, spoof, and restore
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Manages the EDID (Extended Display Identification Data) path through
 * the HDMI-Siphon. Supports transparent passthrough, local injection,
 * HDCP disable, and arbitrary EDID override. Uses the TCA9548 I²C mux
 * to switch between the HDMI IN DDC bus, HDMI OUT DDC bus, and the
 * local 24LC02 EDID EEPROM.
 */

#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"

#include "board.h"
#include "registers.h"
#include "edid_manager.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "EDID";

/** I²C master configuration */
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

/** Default 1080p60 EDID (first block only, 128 bytes) */
static const uint8_t s_edid_1080p60[EDID_BLOCK_SIZE] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, /* Header */
    0x1E, 0x6D, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, /* Manufacturer + product */
    0x01, 0x18,                                      /* Serial */
    0x01, 0x03,                                      /* Week, year */
    0x80, 0x30, 0x1B, 0x78,                          /* EDID version 1.3 */
    0xEA, 0x2E, 0xA5, 0xA5, 0x55, 0x4D, 0x9D, 0x25, /* Basic display params */
    0x12, 0x50, 0x54, 0xBF,                          /* Chromaticity */
    0xEF, 0x00, 0x81, 0x80,                          /* Timing I */
    0x81, 0x40, 0x81, 0x00, 0x81, 0xC0, 0x01, 0x01, /* Established timings */
    0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, /* Standard timings */
    0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, /* Detailed timing 1 */
    0xC4, 0x8E, 0x21, 0x00, 0x00, 0x1E,              /* 1920x1080@60Hz */
    0x00, 0x00, 0x00, 0xFD, 0x00, 0x38, 0x4B, 0x1E, /* Monitor range limits */
    0x50, 0x11, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x48, /* Monitor name */
    0x44, 0x4D, 0x49, 0x2D, 0x53, 0x69, 0x70, 0x68,
    0x6F, 0x6E, 0x0A, 0x20,                          /* "HDMI-Siphon" */
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, /* Padding + checksum */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD8  /* Checksum (calculated) */
};

/* =========================================================================
 * Local State
 * ========================================================================= */

/** Current EDID data */
static uint8_t s_current_edid[EDID_MAX_BLOCKS][EDID_BLOCK_SIZE];
static uint8_t s_edid_block_count = 1;
static bool s_edid_valid = false;
static edid_source_t s_edid_source = EDID_SOURCE_PASSTHROUGH;

/* =========================================================================
 * I²C Helper Functions
 * ========================================================================= */

/**
 * @brief Initialize I²C master for EDID bus access
 */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = CONFIG_I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) return ret;

    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void i2c_init(void)
{
    esp_err_t ret = i2c_master_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "I²C master initialized at %u Hz", CONFIG_I2C_FREQ_HZ);
    } else {
        ESP_LOGE(TAG, "I²C init failed: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief Select a TCA9548 channel
 * @param channel_bits Bitmask of channels to enable (e.g., 0x01 for channel 0)
 */
static esp_err_t tca9548_select(uint8_t channel_bits)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TCA9548_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, channel_bits, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Read a byte from I²C device
 */
static esp_err_t i2c_read_byte(uint8_t dev_addr, uint16_t mem_addr, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    /* Write memory address */
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, (uint8_t)(mem_addr & 0xFF), true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) return ret;

    /* Read data */
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief Write a byte to I²C device
 */
static esp_err_t i2c_write_byte(uint8_t dev_addr, uint16_t mem_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, (uint8_t)(mem_addr & 0xFF), true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* =========================================================================
 * EDID Validation
 * ========================================================================= */

/**
 * @brief Validate EDID header and checksum
 * @param edid EDID block data
 * @param block_size Block size in bytes (128 for standard)
 * @return true if valid
 */
static bool edid_validate(const uint8_t *edid, size_t block_size)
{
    /* Check header */
    if (edid[0] != 0x00 || edid[1] != 0xFF || edid[2] != 0xFF ||
        edid[3] != 0xFF || edid[4] != 0xFF || edid[5] != 0xFF ||
        edid[6] != 0xFF || edid[7] != 0x00) {
        ESP_LOGW(TAG, "Invalid EDID header");
        return false;
    }

    /* Validate checksum (sum of all bytes must be 0 mod 256) */
    uint8_t checksum = 0;
    for (size_t i = 0; i < block_size; i++) {
        checksum += edid[i];
    }
    if (checksum != 0) {
        ESP_LOGW(TAG, "Invalid EDID checksum: 0x%02X (expected 0x00)", checksum);
        return false;
    }

    return true;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t edid_manager_init(void)
{
    memset(s_current_edid, 0, sizeof(s_current_edid));
    s_edid_block_count = 1;
    s_edid_valid = false;
    s_edid_source = EDID_SOURCE_PASSTHROUGH;

    /* Try to read EDID from display */
    esp_err_t ret = edid_read_from_display();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Read EDID from connected display");
    } else {
        ESP_LOGW(TAG, "Could not read display EDID, using default 1080p60");
        memcpy(s_current_edid[0], s_edid_1080p60, EDID_BLOCK_SIZE);
        s_edid_valid = true;
    }

    ESP_LOGI(TAG, "EDID manager initialized");
    return ESP_OK;
}

esp_err_t edid_read_from_display(void)
{
    esp_err_t ret;

    /* Select TCA9548 channel for HDMI OUT DDC */
    ret = tca9548_select(TCA9548_CH_HDMI_OUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to select HDMI OUT mux channel");
        return ret;
    }

    /* Read 128 bytes from display EDID EEPROM (0xA0 = EDID address) */
    for (int i = 0; i < EDID_BLOCK_SIZE; i++) {
        ret = i2c_read_byte(EDID_EEPROM_ADDR, i, &s_current_edid[0][i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read EDID byte %d", i);
            return ret;
        }
    }

    /* Store in FPGA EDID register */
    reg_write(REG_EDID_SOURCE_ADDR, EDID_EEPROM_ADDR);

    /* Validate */
    if (edid_validate(s_current_edid[0], EDID_BLOCK_SIZE)) {
        s_edid_valid = true;
        s_edid_source = EDID_SOURCE_PASSTHROUGH;
        reg_set_bits(REG_EDID_STATUS, EDID_STATUS_VALID | EDID_STATUS_FROM_DISPLAY);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t edid_read_from_local(void)
{
    esp_err_t ret;

    /* Select TCA9548 channel for local EDID EEPROM */
    ret = tca9548_select(TCA9548_CH_EDID_LOCAL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to select local EEPROM mux channel");
        return ret;
    }

    /* Read 128 bytes from local 24LC02 */
    for (int i = 0; i < EDID_BLOCK_SIZE; i++) {
        ret = i2c_read_byte(EDID_EEPROM_ADDR, i, &s_current_edid[0][i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read local EDID byte %d", i);
            return ret;
        }
    }

    if (edid_validate(s_current_edid[0], EDID_BLOCK_SIZE)) {
        s_edid_valid = true;
        s_edid_source = EDID_SOURCE_LOCAL;
        reg_set_bits(REG_EDID_STATUS, EDID_STATUS_VALID | EDID_STATUS_FROM_LOCAL);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t edid_inject(const uint8_t *edid_data, size_t len)
{
    if (edid_data == NULL || len < EDID_BLOCK_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Validate before injecting */
    if (!edid_validate(edid_data, EDID_BLOCK_SIZE)) {
        ESP_LOGE(TAG, "Cannot inject — invalid EDID data");
        return ESP_ERR_INVALID_ARG;
    }

    /* Write to local EEPROM */
    esp_err_t ret = tca9548_select(TCA9548_CH_EDID_LOCAL);
    if (ret != ESP_OK) return ret;

    for (int i = 0; i < EDID_BLOCK_SIZE; i++) {
        ret = i2c_write_byte(EDID_EEPROM_ADDR, i, edid_data[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write EDID byte %d to EEPROM", i);
            return ret;
        }
    }

    /* Update local cache */
    memcpy(s_current_edid[0], edid_data, EDID_BLOCK_SIZE);
    s_edid_valid = true;
    s_edid_source = EDID_SOURCE_INJECTED;

    /* Tell FPGA to use injected EDID */
    reg_set_bits(REG_EDID_CTRL, EDID_CTRL_FORCE_INJECT);
    reg_clear_bits(REG_EDID_CTRL, EDID_CTRL_USE_PASSTHROUGH | EDID_CTRL_USE_LOCAL);

    ESP_LOGI(TAG, "EDID injected: %zu bytes", len);

    /* Dump to SD card */
    sd_card_write_edid(edid_data, len);

    return ESP_OK;
}

esp_err_t edid_disable_hdcp(void)
{
    /* Modify current EDID: clear HDCP support bit (byte 0x14, bit 7) */
    if (!s_edid_valid) {
        return ESP_ERR_INVALID_STATE;
    }

    s_current_edid[0][0x14] &= ~0x80;  /* Clear HDCP flag */

    /* Recalculate checksum */
    uint8_t sum = 0;
    for (int i = 0; i < EDID_BLOCK_SIZE - 1; i++) {
        sum += s_current_edid[i];
    }
    s_current_edid[0][EDID_BLOCK_SIZE - 1] = (uint8_t)(256 - sum);

    /* Write modified EDID to local EEPROM */
    esp_err_t ret = edid_inject(s_current_edid[0], EDID_BLOCK_SIZE);
    if (ret == ESP_OK) {
        reg_set_bits(REG_EDID_CTRL, EDID_CTRL_DISABLE_HDCP);
        ESP_LOGI(TAG, "HDCP disabled in EDID");
    }

    return ret;
}

esp_err_t edid_restore_passthrough(void)
{
    s_edid_source = EDID_SOURCE_PASSTHROUGH;
    reg_set_bits(REG_EDID_CTRL, EDID_CTRL_USE_PASSTHROUGH);
    reg_clear_bits(REG_EDID_CTRL, EDID_CTRL_USE_LOCAL | EDID_CTRL_FORCE_INJECT);
    ESP_LOGI(TAG, "EDID restored to passthrough");
    return ESP_OK;
}

const uint8_t *edid_get_current(size_t *len)
{
    *len = EDID_BLOCK_SIZE * s_edid_block_count;
    return (const uint8_t *)s_current_edid;
}

edid_source_t edid_get_source(void)
{
    return s_edid_source;
}

bool edid_is_valid(void)
{
    return s_edid_valid;
}
