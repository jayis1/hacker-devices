/*
 * safety.c - Safety monitoring: overtemp, battery, watchdog, e-stop
 *
 * Implements hardware-independent safety checks. The hardware overtemp
 * cutoff (analog comparator) operates independently of this code.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "safety.h"
#include "board.h"
#include "registers.h"
#include "tec_driver.h"
#include "laser_driver.h"
#include "temp_sensor.h"

static safety_status_t current_status = SAFETY_OK;
static uint32_t last_watchdog_feed = 0;
static uint32_t estop_debounce_ms = 0;
static bool estop_latched = false;

extern volatile uint32_t systick_ms;

/* ============================================================
 * GPIO configuration
 * ============================================================ */

static void safety_configure_gpio(void)
{
    uint32_t pc_base = GPIOC_BASE;
    uint32_t pa_base = GPIOA_BASE;
    uint32_t pb_base = GPIOB_BASE;
    
    /* E-Stop button (PC13) - input, pull-up (active low) */
    GPIO_MODER(pc_base) &= ~(3U << (ESTOP_PIN * 2));
    GPIO_PUPDR(pc_base) &= ~(3U << (ESTOP_PIN * 2));
    GPIO_PUPDR(pc_base) |= (GPIO_PULLUP << (ESTOP_PIN * 2));
    
    /* Battery ADC (PA5) - analog input */
    GPIO_MODER(pa_base) &= ~(3U << (5 * 2));
    GPIO_MODER(pa_base) |= (GPIO_MODE_ANALOG << (5 * 2));
    
    /* Status LEDs */
    GPIO_MODER(pb_base) &= ~(3U << (LED_STATUS_PIN * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_OUTPUT_PP << (LED_STATUS_PIN * 2));
    GPIO_MODER(pb_base) &= ~(3U << (LED_ARM_PIN * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_OUTPUT_PP << (LED_ARM_PIN * 2));
    GPIO_MODER(pb_base) &= ~(3U << (LED_BLE_PIN * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_OUTPUT_PP << (LED_BLE_PIN * 2));
}

/* ============================================================
 * ADC for battery monitoring
 * ============================================================ */

static uint16_t read_battery_adc(void)
{
    uint32_t adc_base = ADC1_BASE;
    /* Configure channel 5 (PA5) */
    REG32(adc_base + 0x08) = (5U << 0) | (1U << 2);  /* Channel 5, start */
    /* Wait for conversion */
    for (volatile int i = 0; i < 1000; i++) {
        if (REG32(adc_base + 0x08) & (1U << 2)) break;
    }
    return (uint16_t)(REG32(adc_base + 0x40) & 0xFFF);
}

/* ============================================================
 * Public API
 * ============================================================ */

void safety_init(void)
{
    safety_configure_gpio();
    current_status = SAFETY_OK;
    last_watchdog_feed = systick_ms;
    
    /* Turn on status LED */
    GPIO_SET(GPIOB_BASE, LED_STATUS_PIN);
    GPIO_CLR(GPIOB_BASE, LED_ARM_PIN);
    GPIO_CLR(GPIOB_BASE, LED_BLE_PIN);
}

void safety_tick(void)
{
    uint32_t now = systick_ms;
    bool estop_pressed = !GPIO_READ(GPIOC_BASE, ESTOP_PIN);
    
    /* --- E-Stop check with debounce --- */
    if (estop_pressed) {
        if (estop_debounce_ms == 0) {
            estop_debounce_ms = now;
        } else if (now - estop_debounce_ms > 50) {
            estop_latched = true;
            current_status = SAFETY_ESTOP_PRESSED;
            safety_emergency_stop();
            return;
        }
    } else {
        estop_debounce_ms = 0;
    }
    
    /* --- Overtemperature check --- */
    temp_readings_t temps;
    temp_read_all(&temps);
    
    if (temps.ir_temp > SAFETY_MAX_TEMP_C || temps.plate_temp > SAFETY_MAX_TEMP_C + 5.0f) {
        current_status = SAFETY_OVERTEMP;
        safety_emergency_stop();
        return;
    }
    
    /* --- Overcurrent check --- */
    if (tec_is_fault()) {
        current_status = SAFETY_OVERCURRENT;
        safety_emergency_stop();
        return;
    }
    
    /* --- Battery check --- */
    uint16_t bat_mv = safety_read_battery_mv();
    if (bat_mv < BAT_EMPTY_MV && bat_mv > 0) {
        current_status = SAFETY_BATTERY_CRITICAL;
        safety_emergency_stop();
        return;
    }
    
    uint16_t bat_pct = safety_get_battery_pct();
    if (bat_pct < BAT_WARN_PCT) {
        if (current_status == SAFETY_OK) {
            current_status = SAFETY_BATTERY_LOW;
        }
    }
    
    /* --- Watchdog check --- */
    if (now - last_watchdog_feed > SAFETY_WATCHDOG_TIMEOUT_MS) {
        current_status = SAFETY_WATCHDOG;
        safety_emergency_stop();
        return;
    }
    
    /* --- Laser interlock --- */
    if (!laser_is_interlock_ok() && laser_is_shutter_open()) {
        current_status = SAFETY_LASER_INTERLOCK;
        laser_disable();
        return;
    }
    
    /* If we get here and no active fault, clear status */
    if (current_status == SAFETY_BATTERY_LOW) {
        /* Keep low battery warning active */
    } else if (!estop_latched) {
        current_status = SAFETY_OK;
    }
    
    /* --- LED updates --- */
    if (current_status == SAFETY_OK) {
        /* Blink status LED at 1Hz */
        if ((now / 500) % 2 == 0) {
            GPIO_SET(GPIOB_BASE, LED_STATUS_PIN);
        } else {
            GPIO_CLR(GPIOB_BASE, LED_STATUS_PIN);
        }
    } else {
        /* Solid red on fault */
        GPIO_CLR(GPIOB_BASE, LED_STATUS_PIN);
        GPIO_SET(GPIOB_BASE, LED_ARM_PIN);
    }
}

safety_status_t safety_get_status(void)
{
    return current_status;
}

const char *safety_get_status_str(void)
{
    switch (current_status) {
    case SAFETY_OK:               return "OK";
    case SAFETY_OVERTEMP:         return "OVERTEMPERATURE";
    case SAFETY_OVERCURRENT:      return "OVERCURRENT";
    case SAFETY_BATTERY_LOW:      return "BATTERY LOW";
    case SAFETY_BATTERY_CRITICAL: return "BATTERY CRITICAL";
    case SAFETY_ESTOP_PRESSED:    return "EMERGENCY STOP";
    case SAFETY_LASER_INTERLOCK:  return "LASER INTERLOCK";
    case SAFETY_WATCHDOG:         return "WATCHDOG TIMEOUT";
    case SAFETY_FAULT:            return "FAULT";
    default:                      return "UNKNOWN";
    }
}

bool safety_is_ok(void)
{
    return current_status == SAFETY_OK || current_status == SAFETY_BATTERY_LOW;
}

void safety_emergency_stop(void)
{
    /* Immediately disable all outputs */
    tec_disable();
    laser_disable();
    
    /* Turn on fault LED */
    GPIO_SET(GPIOB_BASE, LED_ARM_PIN);
    GPIO_CLR(GPIOB_BASE, LED_STATUS_PIN);
    
    /* E-stop is latching - requires manual reset */
}

uint16_t safety_read_battery_mv(void)
{
    uint16_t adc = read_battery_adc();
    /* ADC: 0-4095 = 0-3300mV
     * Divider ratio: 4.0 (14.8V -> 3.7V)
     * voltage = (adc / 4095) * 3300 * 4.0 */
    uint32_t mv = ((uint32_t)adc * 3300 * BAT_DIVIDER_RATIO) / 4095;
    return (uint16_t)mv;
}

uint16_t safety_get_battery_pct(void)
{
    uint16_t mv = safety_read_battery_mv();
    if (mv == 0) return 0;
    if (mv >= BAT_FULL_MV) return 100;
    if (mv <= BAT_EMPTY_MV) return 0;
    uint32_t pct = ((uint32_t)(mv - BAT_EMPTY_MV) * 100) /
                   (BAT_FULL_MV - BAT_EMPTY_MV);
    return (uint16_t)pct;
}

float safety_get_mcu_temp(void)
{
    /* MCU internal temperature sensor - simplified */
    return 25.0f;
}

void safety_feed_watchdog(void)
{
    last_watchdog_feed = systick_ms;
}