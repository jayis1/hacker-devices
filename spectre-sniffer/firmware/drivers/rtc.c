//==============================================================================
// rtc.c — Spectre-Sniffer DS3231 RTC Driver
// Author: jayis1
// Description: Precision real-time clock driver for timestamping captured
//              EM data and maintaining system time across reboots.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "board.h"

static const char *TAG = "RTC";

//==============================================================================
// DS3231 Register Map
//==============================================================================
#define DS3231_REG_SECONDS      0x00
#define DS3231_REG_MINUTES      0x01
#define DS3231_REG_HOURS        0x02
#define DS3231_REG_DAY          0x03
#define DS3231_REG_DATE         0x04
#define DS3231_REG_MONTH        0x05
#define DS3231_REG_YEAR         0x06
#define DS3231_REG_ALARM1_SEC   0x07
#define DS3231_REG_ALARM1_MIN   0x08
#define DS3231_REG_ALARM1_HOUR  0x09
#define DS3231_REG_ALARM1_DAY   0x0A
#define DS3231_REG_ALARM2_MIN   0x0B
#define DS3231_REG_ALARM2_HOUR  0x0C
#define DS3231_REG_ALARM2_DAY   0x0D
#define DS3231_REG_CONTROL      0x0E
#define DS3231_REG_STATUS       0x0F
#define DS3231_REG_AGING_OFFSET 0x10
#define DS3231_REG_TEMP_MSB     0x11
#define DS3231_REG_TEMP_LSB     0x12

// Control register bits
#define DS3231_CTRL_EOSC        (1 << 7)    // Enable oscillator (0=enable)
#define DS3231_CTRL_BBSQW       (1 << 6)    // Battery-backed square wave
#define DS3231_CTRL_CONV        (1 << 5)    // Temperature conversion
#define DS3231_CTRL_RS2         (1 << 4)    // Rate select 2
#define DS3231_CTRL_RS1         (1 << 3)    // Rate select 1
#define DS3231_CTRL_INTCN       (1 << 2)    // Interrupt control
#define DS3231_CTRL_A2IE        (1 << 1)    // Alarm 2 interrupt enable
#define DS3231_CTRL_A1IE        (1 << 0)    // Alarm 1 interrupt enable

// Status register bits
#define DS3231_STAT_OSF         (1 << 7)    // Oscillator stop flag
#define DS3231_STAT_EN32kHz     (1 << 3)    // Enable 32kHz output
#define DS3231_STAT_BSY         (1 << 2)    // Busy (temperature conversion)
#define DS3231_STAT_A2F         (1 << 1)    // Alarm 2 flag
#define DS3231_STAT_A1F         (1 << 0)    // Alarm 1 flag

//==============================================================================
// RTC State
//==============================================================================
typedef struct {
    bool    initialized;
    bool    present;
    int16_t temperature_celsius;  // Tenths of degrees
} rtc_state_t;

static rtc_state_t s_rtc = {0};

//==============================================================================
// I2C Read/Write Helpers
//==============================================================================
static esp_err_t rtc_write_reg(uint8_t reg, uint8_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RTC_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t rtc_read_reg(uint8_t reg, uint8_t *value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RTC_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t rtc_read_burst(uint8_t reg, uint8_t *data, uint8_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    for (int i = 0; i < len - 1; i++) {
        i2c_master_read_byte(cmd, &data[i], I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &data[len - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RTC_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

//==============================================================================
// BCD Conversion Helpers
//==============================================================================
static inline uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

//==============================================================================
// Initialize RTC
//==============================================================================
int rtc_init_driver(void) {
    ESP_LOGI(TAG, "Initializing RTC (DS3231)...");

    // Verify RTC presence
    uint8_t test;
    esp_err_t ret = rtc_read_reg(DS3231_REG_STATUS, &test);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "RTC not detected on I2C bus (err=%d)", ret);
        s_rtc.present = false;
        return SPECTRE_ERR_IO;
    }

    s_rtc.present = true;

    // Check oscillator stop flag
    if (test & DS3231_STAT_OSF) {
        ESP_LOGW(TAG, "RTC oscillator has stopped - time may be invalid");
        // Clear the flag
        rtc_write_reg(DS3231_REG_STATUS, test & ~DS3231_STAT_OSF);
    }

    // Enable 32kHz output for potential use as FPGA reference
    rtc_write_reg(DS3231_REG_STATUS, test | DS3231_STAT_EN32kHz);

    // Read temperature
    rtc_read_temperature(&s_rtc.temperature_celsius);

    s_rtc.initialized = true;
    ESP_LOGI(TAG, "RTC initialized (temp: %d.%d°C)",
             s_rtc.temperature_celsius / 10,
             abs(s_rtc.temperature_celsius % 10));
    return SPECTRE_OK;
}

//==============================================================================
// Get Current Time
//==============================================================================
int rtc_get_time(struct tm *timeinfo) {
    if (!s_rtc.initialized || !s_rtc.present) return SPECTRE_ERR_NOT_INIT;
    if (!timeinfo) return SPECTRE_ERR_INVALID_PARAM;

    uint8_t data[7];
    esp_err_t ret = rtc_read_burst(DS3231_REG_SECONDS, data, 7);
    if (ret != ESP_OK) return SPECTRE_ERR_IO;

    timeinfo->tm_sec  = bcd_to_dec(data[0] & 0x7F);
    timeinfo->tm_min  = bcd_to_dec(data[1] & 0x7F);
    timeinfo->tm_hour = bcd_to_dec(data[2] & 0x3F);
    timeinfo->tm_wday = bcd_to_dec(data[3] & 0x07) - 1;  // DS3231: 1=Sun, tm: 0=Sun
    timeinfo->tm_mday = bcd_to_dec(data[4] & 0x3F);
    timeinfo->tm_mon  = bcd_to_dec(data[5] & 0x1F) - 1;  // DS3231: 1-12, tm: 0-11
    timeinfo->tm_year = bcd_to_dec(data[6]) + 100;        // DS3231: 0-99, tm: years since 1900

    return SPECTRE_OK;
}

//==============================================================================
// Set Current Time
//==============================================================================
int rtc_set_time(const struct tm *timeinfo) {
    if (!s_rtc.initialized || !s_rtc.present) return SPECTRE_ERR_NOT_INIT;
    if (!timeinfo) return SPECTRE_ERR_INVALID_PARAM;

    uint8_t data[7];
    data[0] = dec_to_bcd(timeinfo->tm_sec) & 0x7F;   // Seconds (bit 7 = CH)
    data[1] = dec_to_bcd(timeinfo->tm_min) & 0x7F;    // Minutes
    data[2] = dec_to_bcd(timeinfo->tm_hour) & 0x3F;   // Hours (24-hour mode)
    data[3] = dec_to_bcd(timeinfo->tm_wday + 1) & 0x07; // Day of week
    data[4] = dec_to_bcd(timeinfo->tm_mday) & 0x3F;   // Date
    data[5] = dec_to_bcd(timeinfo->tm_mon + 1) & 0x1F; // Month
    data[6] = dec_to_bcd(timeinfo->tm_year - 100);     // Year

    // Write all registers in burst
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, DS3231_REG_SECONDS, true);
    for (int i = 0; i < 7; i++) {
        i2c_master_write_byte(cmd, data[i], true);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RTC_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) return SPECTRE_ERR_IO;

    ESP_LOGI(TAG, "RTC time set to %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return SPECTRE_OK;
}

//==============================================================================
// Read Temperature
//==============================================================================
int rtc_read_temperature(int16_t *temp_tenths) {
    if (!s_rtc.present) return SPECTRE_ERR_NOT_INIT;
    if (!temp_tenths) return SPECTRE_ERR_INVALID_PARAM;

    uint8_t msb, lsb;
    esp_err_t ret = rtc_read_reg(DS3231_REG_TEMP_MSB, &msb);
    if (ret != ESP_OK) return SPECTRE_ERR_IO;
    ret = rtc_read_reg(DS3231_REG_TEMP_LSB, &lsb);
    if (ret != ESP_OK) return SPECTRE_ERR_IO;

    // Temperature: MSB = integer part, upper bits of LSB = fractional (0.25°C steps)
    *temp_tenths = (int16_t)(msb * 10) + ((lsb >> 6) * 25) / 10;
    s_rtc.temperature_celsius = *temp_tenths;

    return SPECTRE_OK;
}

//==============================================================================
// Status Queries
//==============================================================================
bool rtc_is_present(void) {
    return s_rtc.present;
}

bool rtc_is_initialized(void) {
    return s_rtc.initialized;
}

int16_t rtc_get_temperature(void) {
    return s_rtc.temperature_celsius;
}
