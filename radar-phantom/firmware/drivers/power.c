/*
 * drivers/power.c — battery monitoring, rail sequencing, low-battery cutoff
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#include "../board.h"
#include "../registers.h"

static uint32_t last_sample_ms;
static uint16_t bat_mv_smoothed;
static uint8_t  low_battery_flag;
static uint8_t  rf_disabled_by_low_bat;

static uint16_t adc_read_battery(void)
{
    /* Single software-trigger conversion on ADC3 channel 7 (PF9) */
    ADC_CR(ADC3_BASE) |= ADC_CR_ADEN;
    while (!(ADC_ISR(ADC3_BASE) & ADC_ISR_ADRDY)) { }
    ADC_SQR1(ADC3_BASE) = BAT_ADC_CHANNEL;
    ADC_CR(ADC3_BASE) |= ADC_CR_ADSTART;
    while (!(ADC_ISR(ADC3_BASE) & ADC_ISR_EOC)) { }
    uint16_t raw = (uint16_t)(ADC_DR1(ADC3_BASE) & 0xFFF);
    /* Convert to mV: Vref 3.3 V, 12-bit, divider /3 */
    uint32_t mv = (uint32_t)raw * 3300 * BAT_DIV / 4096;
    return (uint16_t)mv;
}

void power_init(void)
{
    last_sample_ms = board_millis();
    bat_mv_smoothed = adc_read_battery();
    low_battery_flag = 0;
    rf_disabled_by_low_bat = 0;
}

void power_task(void)
{
    uint32_t now = board_millis();
    if (now - last_sample_ms < 1000) return;
    last_sample_ms = now;
    uint16_t mv = adc_read_battery();
    /* exponential smoothing */
    bat_mv_smoothed = (uint16_t)((bat_mv_smoothed * 7 + mv) / 8);
    if (bat_mv_smoothed < BAT_EMPTY_MV) {
        low_battery_flag = 1;
        if (!rf_disabled_by_low_bat) {
            board_power_disable_rf();
            rf_disabled_by_low_bat = 1;
        }
    }
    if (low_battery_flag && bat_mv_smoothed > BAT_WARN_MV + 200) {
        /* hysteresis: clear once we recover well above cutoff (e.g. after charging) */
        low_battery_flag = 0;
        rf_disabled_by_low_bat = 0;
    }
}

uint16_t power_battery_mv(void) { return bat_mv_smoothed; }

uint8_t power_battery_percent(void)
{
    if (bat_mv_smoothed >= BAT_FULL_MV) return 100;
    if (bat_mv_smoothed <= BAT_EMPTY_MV) return 0;
    uint32_t span = BAT_FULL_MV - BAT_EMPTY_MV;
    uint32_t v = bat_mv_smoothed - BAT_EMPTY_MV;
    return (uint8_t)((v * 100) / span);
}

uint8_t power_low_battery(void) { return low_battery_flag; }

uint8_t power_rf_ok(void) { return !rf_disabled_by_low_bat; }