/**
 * @file battery_mon.c
 * @author jayis1
 * @brief MAX17048 Battery Monitor Driver
 *
 * Driver for the MAX17048 LiPo battery fuel gauge IC.
 * Provides voltage, state of charge, and alert functionality
 * over I2C. Used by the power management task for battery-aware
 * operation of the SATA Phantom.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "board.h"

static const char *TAG = "batmon";

/* MAX17048 I2C registers */
#define MAX17048_VCELL         0x02
#define MAX17048_SOC           0x04
#define MAX17048_MODE          0x06
#define MAX17048_VERSION       0x08
#define MAX17048_HIBRT         0x0A
#define MAX17048_CONFIG        0x0C
#define MAX17048_VALRT         0x14
#define MAX17048_CRATE         0x16
#define MAX17048_VRESET_ID     0x18
#define MAX17048_STATUS        0x1A
#define MAX17048_TABLE         0x40
#define MAX17048_CMD           0xFE

/* I2C timeout */
#define I2C_TIMEOUT_MS         100

/**
 * @brief Read a 16-bit register from the MAX17048.
 */
static esp_err_t max17048_read(uint8_t reg, uint16_t *value)
{
    if (!value) return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX17048_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX17048_ADDR << 1) | I2C_MASTER_READ, true);

    uint8_t data[2];
    i2c_master_read_byte(cmd, &data[0], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data[1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(BAT_I2C_PORT, cmd,
                                          pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        *value = ((uint16_t)data[0] << 8) | data[1];
    }
    return ret;
}

/**
 * @brief Write a 16-bit value to a MAX17048 register.
 */
static esp_err_t max17048_write(uint8_t reg, uint16_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX17048_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, (value >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, value & 0xFF, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(BAT_I2C_PORT, cmd,
                                          pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* ===================================================================
 * Public API
 * =================================================================== */

/**
 * @brief Initialize the MAX17048 battery monitor.
 * Performs a quick-start if the IC is in sleep mode.
 */
esp_err_t batmon_init(void)
{
    /* Check IC is alive by reading version */
    uint16_t version = 0;
    esp_err_t ret = max17048_read(MAX17048_VERSION, &version);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAX17048 not responding (I2C error: %d)", ret);
        return ret;
    }

    ESP_LOGI(TAG, "MAX17048 detected (version=0x%04X)", version);

    /* Perform quick-start to re-initialize fuel gauge calculations */
    ret = max17048_write(MAX17048_MODE, 0x4000);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(10));  /* Wait for quick-start to complete */
    }

    /* Configure alerts: voltage threshold */
    /* Set VALRT: max voltage alert = 4.35V, min voltage alert = 3.2V */
    ret = max17048_write(MAX17048_VALRT, (0x022C << 8) | 0x01A0);
    /* 0x022C = 4.35V (4340/10*2^4 = 0x022C), 0x01A0 = 3.2V (3200/10*2^4) */

    /* Enable voltage alerts */
    uint16_t config = 0;
    max17048_read(MAX17048_CONFIG, &config);
    config &= ~0x001F;  /* Clear ALSC and alert thresholds */
    config |= 0x0004;   /* Enable voltage alerts */
    max17048_write(MAX17048_CONFIG, config);

    ESP_LOGI(TAG, "Battery monitor initialized");
    return ESP_OK;
}

/**
 * @brief Read the battery voltage in millivolts.
 */
esp_err_t batmon_read_voltage(uint32_t *voltage_mv)
{
    if (!voltage_mv) return ESP_ERR_INVALID_ARG;

    uint16_t vcell = 0;
    esp_err_t ret = max17048_read(MAX17048_VCELL, &vcell);
    if (ret != ESP_OK) {
        return ret;
    }

    /* VCELL is a 12-bit value where each LSB = 1.25 mV */
    /* Register format: [15:4] = 12-bit MSB-aligned ADC value */
    uint32_t mv = ((uint32_t)(vcell >> 4) * 125) / 100;
    *voltage_mv = mv;

    return ESP_OK;
}

/**
 * @brief Read the state of charge as a percentage (0.0–100.0).
 */
esp_err_t batmon_read_soc(float *soc_percent)
{
    if (!soc_percent) return ESP_ERR_INVALID_ARG;

    uint16_t soc_raw = 0;
    esp_err_t ret = max17048_read(MAX17048_SOC, &soc_raw);
    if (ret != ESP_OK) {
        return ret;
    }

    /* SOC register: value/256 = percentage */
    *soc_percent = (float)soc_raw / 256.0f;
    if (*soc_percent > 100.0f) *soc_percent = 100.0f;

    return ESP_OK;
}

/**
 * @brief Read the charge/discharge rate in %/hour.
 */
esp_err_t batmon_read_crate(float *crate)
{
    if (!crate) return ESP_ERR_INVALID_ARG;

    uint16_t crate_raw = 0;
    esp_err_t ret = max17048_read(MAX17048_CRATE, &crate_raw);
    if (ret != ESP_OK) {
        return ret;
    }

    /* CRATE: signed 16-bit, 5.625%/hr per LSB, in 2's complement */
    int16_t signed_raw = (int16_t)crate_raw;
    *crate = (float)signed_raw * 5.625f / 100.0f;
    return ESP_OK;
}

/**
 * @brief Put the MAX17048 into hibernate mode for low power.
 */
esp_err_t batmon_hibernate(void)
{
    /* Set hibernate threshold registers */
    esp_err_t ret = max17048_write(MAX17048_HIBRT, 0x8060);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Battery monitor entering hibernate");
    }
    return ret;
}

/**
 * @brief Wake the MAX17048 from hibernate.
 */
esp_err_t batmon_wake(void)
{
    return max17048_write(MAX17048_MODE, 0x4000);  /* Quick-start wakes it */
}
