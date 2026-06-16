/**
 * @file power_mgmt.c
 * @author jayis1
 * @brief Power Management and Battery Monitoring
 *
 * Manages power rail sequencing, battery monitoring via MAX17048,
 * charge state detection, and deep sleep entry/exit based on
 * SATA bus activity. Implements power-aware throttling for
 * stealth exfiltration.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "board.h"

static const char *TAG = "power";

/* Battery thresholds (for 100 mAh LiPo at 3.7V nominal) */
#define BAT_CRITICAL_MV        3200   /* Shut down WiFi/BT */
#define BAT_LOW_MV             3500   /* Throttle exfiltration */
#define BAT_NORMAL_MV          3700   /* Nominal */
#define BAT_FULL_MV            4200   /* Fully charged */

/* Deep sleep timeout (seconds to sleep before wake-check) */
#define SLEEP_INTERVAL_S       10

/* ===================================================================
 * MAX17048 Battery Gauge Driver
 * =================================================================== */

/* MAX17048 register addresses */
#define MAX17048_REG_VCELL      0x02   /* Battery voltage (12-bit, unit = 1.25 mV) */
#define MAX17048_REG_SOC        0x04   /* State of charge (% * 256) */
#define MAX17048_REG_MODE       0x06   /* Mode register */
#define MAX17048_REG_VERSION    0x08   /* IC version */
#define MAX17048_REG_CONFIG     0x0C   /* Configuration */
#define MAX17048_REG_CMD        0xFE   /* Command register */

/**
 * @brief Read a 16-bit register from the MAX17048.
 */
static esp_err_t max17048_read_reg(uint8_t reg, uint16_t *value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX17048_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX17048_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, (uint8_t *)value, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, ((uint8_t *)value) + 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(BAT_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        *value = __bswap16(*value);  /* MAX17048 uses big-endian */
    }
    return ret;
}

/**
 * @brief Read battery voltage in millivolts.
 * @param mv  Output: voltage in mV
 * @return ESP_OK on success.
 */
static esp_err_t max17048_read_voltage(uint32_t *mv)
{
    uint16_t vcell = 0;
    esp_err_t ret = max17048_read_reg(MAX17048_REG_VCELL, &vcell);
    if (ret == ESP_OK) {
        /* VCELL is 12-bit, 1.25 mV per LSB */
        *mv = ((uint32_t)(vcell >> 4)) * 125 / 100;
    }
    return ret;
}

/**
 * @brief Read battery state of charge in percent.
 * @param percent  Output: 0.0–100.0
 * @return ESP_OK on success.
 */
static esp_err_t max17048_read_soc(float *percent)
{
    uint16_t soc_raw = 0;
    esp_err_t ret = max17048_read_reg(MAX17048_REG_SOC, &soc_raw);
    if (ret == ESP_OK) {
        *percent = (float)soc_raw / 256.0f;
    }
    return ret;
}

/* ===================================================================
 * Power Rail Management
 * =================================================================== */

/**
 * @brief Detect the current power source.
 * @return Power source enum value.
 */
static power_source_t detect_power_source(void)
{
    if (gpio_get_level(PIN_PWR_SATA_DETECT)) {
        /* Could distinguish 5V vs 12V via ADC */
        return PWR_SOURCE_SATA_5V;
    }
    if (gpio_get_level(PIN_PWR_USB_DETECT)) {
        return PWR_SOURCE_USB_C;
    }
    /* Check if battery is above critical threshold */
    uint32_t mv = 0;
    if (max17048_read_voltage(&mv) == ESP_OK && mv > BAT_CRITICAL_MV) {
        return PWR_SOURCE_BATTERY;
    }
    return PWR_SOURCE_NONE;
}

/**
 * @brief Sequence power rails on.
 */
static void power_rails_on(void)
{
    ESP_LOGI(TAG, "Powering up rails...");
    gpio_set_level(PIN_PWR_5V_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(PIN_PWR_3V3_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(PIN_PWR_1V2_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "All power rails enabled");
}

/**
 * @brief Sequence power rails off.
 */
static void power_rails_off(void)
{
    ESP_LOGI(TAG, "Powering down rails...");
    gpio_set_level(PIN_PWR_1V2_EN, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(PIN_PWR_3V3_EN, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(PIN_PWR_5V_EN, 0);
}

/* ===================================================================
 * Deep Sleep Management
 * =================================================================== */

/**
 * @brief Enter deep sleep mode, waking on SATA OOB signal or timer.
 */
static void enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep mode");

    /* Notify other tasks */
    xEventGroupSetBits(system_events, EVENT_BIT_DEEP_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(100));  /* Allow tasks to clean up */

    /* Configure wake-up sources */
    /* Wake on GPIO (FPGA IRQ line — OOB signal detected) */
    esp_sleep_enable_ext0_wakeup(PIN_FPGA_IRQ, 0);  /* Wake on low */

    /* Timer wakeup as fallback */
    esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_S * 1000000ULL);

    /* Power down peripherals */
    power_rails_off();

    /* Enter deep sleep */
    ESP_LOGI(TAG, "Deep sleep — wake on OOB or %ds timer", SLEEP_INTERVAL_S);
    esp_deep_sleep_start();
}

/* ===================================================================
 * Power Management Task
 * =================================================================== */

void power_mgmt_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Power management task started");

    /* Initialize I2C for battery monitor */
    /* I2C already initialized in main.c */

    /* Enable power rails */
    power_rails_on();

    /* Wait for FPGA ready */
    xEventGroupWaitBits(system_events, EVENT_BIT_FPGA_READY,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    power_source_t last_source = PWR_SOURCE_NONE;
    uint32_t last_battery_mv = 0;
    uint32_t low_battery_count = 0;

    while (1) {
        /* Detect power source */
        power_source_t source = detect_power_source();

        /* Read battery voltage */
        uint32_t battery_mv = 0;
        max17048_read_voltage(&battery_mv);

        /* Log power state changes */
        if (source != last_source || abs((int)battery_mv - (int)last_battery_mv) > 50) {
            const char *src_str = "NONE";
            switch (source) {
                case PWR_SOURCE_SATA_5V:  src_str = "SATA 5V";  break;
                case PWR_SOURCE_SATA_12V: src_str = "SATA 12V"; break;
                case PWR_SOURCE_USB_C:    src_str = "USB-C";    break;
                case PWR_SOURCE_BATTERY:  src_str = "BATTERY";  break;
                default: break;
            }
            float soc = 0.0f;
            max17048_read_soc(&soc);
            ESP_LOGI(TAG, "Power: %s, %d mV (%.1f%%)", src_str, battery_mv, soc);
            last_source = source;
            last_battery_mv = battery_mv;
        }

        /* Battery management */
        if (source == PWR_SOURCE_BATTERY) {
            if (battery_mv < BAT_CRITICAL_MV) {
                low_battery_count++;
                if (low_battery_count >= 3) {
                    /* Critically low — signal all tasks to throttle */
                    ESP_LOGW(TAG, "BATTERY CRITICAL (%d mV) — throttling all activity", battery_mv);
                    xEventGroupSetBits(system_events, EVENT_BIT_BATTERY_LOW);

                    /* If battery is critically low and no SATA power, enter deep sleep */
                    if (battery_mv < 3000) {
                        ESP_LOGE(TAG, "Battery critically low — entering deep sleep");
                        enter_deep_sleep();
                    }
                }
            } else if (battery_mv > BAT_LOW_MV) {
                low_battery_count = 0;
                xEventGroupClearBits(system_events, EVENT_BIT_BATTERY_LOW);
            }
        } else {
            /* External power — clear battery low flag */
            xEventGroupClearBits(system_events, EVENT_BIT_BATTERY_LOW);
            low_battery_count = 0;

            /* Enable battery charging if USB-C connected */
            if (source == PWR_SOURCE_USB_C) {
                /* MAX17048 handles charging autonomously */
            }
        }

        /* Handle deep sleep request from mode change */
        if (xEventGroupGetBits(system_events) & EVENT_BIT_DEEP_SLEEP) {
            enter_deep_sleep();
        }

        vTaskDelay(pdMS_TO_TICKS(5000));  /* Check every 5 seconds */
    }
}
