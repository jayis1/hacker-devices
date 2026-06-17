/**
 * @file    battery.c
 * @brief   Battery voltage monitoring and fuel gauging
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Monitors the Li-Po battery voltage through the ESP32-S3 ADC (GPIO20).
 * Uses a voltage divider (100k/47k) to bring 4.2V max down to ~1.35V,
 * within the 0-2.5V ADC reference range. Provides read_mv(), percentage,
 * and low-battery callback.
 */

#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "board.h"
#include "battery.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "BATTERY";

/** ADC channel used for battery monitoring */
#define BAT_ADC_CHANNEL               ADC1_CHANNEL_4   /* GPIO20 */

/** ADC attenuation (0dB for 0-1.1V, 2.5dB for 0-1.5V, 6dB for 0-2.2V, 11dB for 0-3.3V) */
#define BAT_ADC_ATTEN                 ADC_ATTEN_DB_11

/** ADC width (12 bits = 0-4095) */
#define BAT_ADC_WIDTH                 ADC_WIDTH_BIT_12

/** Number of samples for averaging */
#define BAT_ADC_SAMPLES                     8U

/** Calibration result */
static esp_adc_cal_characteristics_t s_adc_chars;

/* =========================================================================
 * Initialization
 * ========================================================================= */

void battery_init(void)
{
    /* Configure ADC */
    adc1_config_width(BAT_ADC_WIDTH);
    adc1_config_channel_atten(BAT_ADC_CHANNEL, BAT_ADC_ATTEN);

    /* Attempt Vref calibration */
    esp_adc_cal_characterize(ADC_UNIT_1, BAT_ADC_ATTEN, BAT_ADC_WIDTH, 1100, &s_adc_chars);

    ESP_LOGI(TAG, "Battery monitor initialized");
}

/* =========================================================================
 * Reading
 * ========================================================================= */

uint32_t battery_read_mv(void)
{
    uint32_t adc_reading = 0;

    /* Take multiple samples and average */
    for (int i = 0; i < BAT_ADC_SAMPLES; i++) {
        adc_reading += adc1_get_raw(BAT_ADC_CHANNEL);
    }
    adc_reading /= BAT_ADC_SAMPLES;

    /* Convert to voltage with calibration */
    uint32_t voltage_mv;
    esp_adc_cal_get_voltage(ADC_UNIT_1, BAT_ADC_CHANNEL, &s_adc_chars, &voltage_mv);

    /* Apply voltage divider correction (100k/47k = 3.128 ratio) */
    voltage_mv = (uint32_t)((float)voltage_mv * BAT_DIVIDER_RATIO);

    return voltage_mv;
}

uint32_t battery_read_percent(void)
{
    uint32_t mv = battery_read_mv();
    return board_batt_percent(mv);
}

bool battery_is_low(void)
{
    return board_batt_low(battery_read_mv());
}

bool battery_is_critical(void)
{
    return board_batt_critical(battery_read_mv());
}

/* =========================================================================
 * FreeRTOS Task
 * ========================================================================= */

void battery_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Battery monitor task started");

    while (1) {
        uint32_t mv = battery_read_mv();
        uint32_t pct = board_batt_percent(mv);

        ESP_LOGD(TAG, "Battery: %" PRIu32 " mV (%" PRIu32 "%%)", mv, pct);

        /* Check for critical battery */
        if (board_batt_critical(mv)) {
            ESP_LOGE(TAG, "Battery CRITICAL — requesting shutdown");
            /* Send critical event to main task */
            event_t evt = {
                .type = EVENT_BATTERY_CRIT,
                .data = mv,
                .payload = NULL
            };
            /* Note: g_event_queue is extern in main.c */
            extern QueueHandle_t g_event_queue;
            if (g_event_queue) {
                xQueueSend(g_event_queue, &evt, 0);
            }
        }

        /* Sleep for 5 seconds between readings */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
