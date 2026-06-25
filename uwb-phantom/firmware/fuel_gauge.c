/*
 * fuel_gauge.c — MAX17048 single-cell LiPo fuel gauge driver.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The MAX17048 exposes a 16-bit SOC register (0x04) and a 16-bit VCELL
 * register (0x02).  SOC is in 1/256 % units; VCELL is in 78.125 µV
 * units.  We read both over the shared I²C bus and convert to friendly
 * units.
 */

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "board.h"
#include "fuel_gauge.h"

static const char *TAG = "fg";

static int reg_read16(uint8_t reg, uint16_t *out)
{
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (FG_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, reg, true);
    i2c_master_start(h);
    i2c_master_write_byte(h, (FG_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    uint8_t hi, lo;
    i2c_master_read_byte(h, &hi, I2C_MASTER_ACK);
    i2c_master_read_byte(h, &lo, I2C_MASTER_NACK);
    i2c_master_stop(h);
    esp_err_t e = i2c_master_cmd_begin(FG_I2C_HOST, h, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(h);
    if (e != ESP_OK) return -EIO;
    *out = ((uint16_t)hi << 8) | lo;
    return 0;
}

int fuel_gauge_init(void)
{
    uint16_t v = 0;
    int rc = reg_read16(0x08 /* VRESET */, &v);
    if (rc != 0) {
        ESP_LOGE(TAG, "MAX17048 not found on I2C");
        return -ENODEV;
    }
    ESP_LOGI(TAG, "MAX17048 fuel gauge OK (VRESET=0x%04x)", v);
    return 0;
}

int fuel_gauge_percent(int *out)
{
    uint16_t soc;
    int rc = reg_read16(0x04 /* SOC */, &soc);
    if (rc != 0) return rc;
    /* SOC register: 1/256 % per LSB. */
    *out = (int)((soc * 100 + 128) >> 8);
    if (*out > 100) *out = 100;
    if (*out < 0)   *out = 0;
    return 0;
}

int fuel_gauge_voltage_mv(int *out)
{
    uint16_t v;
    int rc = reg_read16(0x02 /* VCELL */, &v);
    if (rc != 0) return rc;
    /* VCELL: 78.125 µV per LSB -> mV = v * 5 / 64. */
    *out = (int)((uint32_t)v * 5 / 64);
    return 0;
}

int fuel_gauge_soc_pct(int *out)
{
    return fuel_gauge_percent(out);
}