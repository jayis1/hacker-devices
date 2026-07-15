/*
 * tesla-phantom — drivers/hv_pulse.c
 * High-voltage charge pump and Marx bank control.
 * Uses DAC1 channel 1 to set charge voltage, TIM6 for charge pump clock,
 * and ADC channel to monitor actual voltage.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* The charge pump circuit is driven by a PWM signal (TIM6 → DAC trigger)
 * which generates a square wave that is stepped up through a diode-capacitor
 * ladder. The output voltage is proportional to the DAC setpoint which
 * controls the charge pump drive level via a variable regulator.
 *
 * A voltage divider (10:1) feeds back to an internal ADC channel for
 * monitoring the actual Marx bank voltage.
 */

#define HV_ADC_CHANNEL    5    /* ADC1 channel 5 (PA0) */
#define HV_DIVIDER_RATIO  10   /* 10:1 voltage divider */
#define HV_MAX_DAC        4095 /* 12-bit DAC */
#define HV_MAX_VOLTAGE    400  /* volts */
#define HV_CHARGE_TIMEOUT 500  /* ms */

static uint8_t hv_charged_flag = 0;
static uint16_t hv_target_voltage = 0;

/* ── Internal ADC for HV monitoring (simplified) ──────────────────── */
static uint16_t read_hv_adc(void) {
    /* The HV feedback voltage is on an internal ADC channel.
     * We use a simple polling read of ADC1.
     * In a real implementation, this would use the ADC with proper
     * channel sequencing and calibration. */
    volatile uint32_t *adc1_cr   = (volatile uint32_t *)(ADC1_BASE + 0x08);
    volatile uint32_t *adc1_pc   = (volatile uint32_t *)(ADC1_BASE + 0x0C);
    volatile uint32_t *adc1_sqr1 = (volatile uint32_t *)(ADC1_BASE + 0x30);
    volatile uint32_t *adc1_sqr3 = (volatile uint32_t *)(ADC1_BASE + 0x34);
    volatile uint32_t *adc1_dr   = (volatile uint32_t *)(ADC1_BASE + 0x40);
    volatile uint32_t *adc1_isr  = (volatile uint32_t *)(ADC1_BASE + 0x00);

    /* Enable ADC */
    *adc1_cr |= (1U << 0);  /* ADEN */
    delay_us(1);

    /* Configure channel 5, single conversion */
    *adc1_pc = (1U << HV_ADC_CHANNEL);  /* enable channel 5 precharge */
    *adc1_sqr1 = 0;  /* 1 conversion */
    *adc1_sqr3 = HV_ADC_CHANNEL;  /* first conversion = ch5 */

    /* Start conversion */
    *adc1_cr |= (1U << 2);  /* ADSTART */
    while (!(*adc1_isr & (1U << 2))) { }  /* wait for ADRDY */
    while (!(*adc1_isr & (1U << 1))) { }  /* wait for EOC */

    return (uint16_t)(*adc1_dr & 0xFFFF);
}

static uint16_t adc_to_voltage(uint16_t adc_val) {
    /* ADC: 0–3.3V → 0–4095 (12-bit)
     * Voltage divider: 10:1 → actual HV = ADC * 3.3 / 4095 * 10
     * = ADC * 33 / 4095 ≈ ADC * 0.00806
     * Return in volts */
    return (uint16_t)((uint32_t)adc_val * 33U / 4095U);
}

static void set_dac_voltage(uint16_t millivolts) {
    /* DAC output: 0–3.3V → 0–4095
     * millivolts: 0–3300 → DAC value = mV * 4095 / 3300 */
    if (millivolts > 3300) millivolts = 3300;
    uint16_t dac_val = (uint16_t)((uint32_t)millivolts * 4095U / 3300U);
    DAC1->DHR12R1 = dac_val;
    /* Trigger DAC update (software trigger) */
    DAC1->SWTRIGR |= (1U << 0);
}

static void charge_pump_enable(uint8_t enable) {
    /* The charge pump is enabled via TIM6 generating the DAC-trigger
     * which in turn drives the pump oscillator.
     * Enable = TIM6 running, Disable = TIM6 stopped. */
    if (enable) {
        TIM6->CR1 |= TIM_CR1_CEN;
    } else {
        TIM6->CR1 &= ~TIM_CR1_CEN;
    }
}

/* ── Public API ───────────────────────────────────────────────────── */

int hv_init(void) {
    /* Enable DAC channel 1 */
    DAC1->CR = 0;  /* disable before config */
    DAC1->CR = DAC_CR_EN1 | DAC_CR_BOFF1 | DAC_CR_TSEL1_SW;
    delay_us(10);

    /* Set initial voltage to 0 */
    set_dac_voltage(0);
    hv_charged_flag = 0;
    hv_target_voltage = 0;

    /* Disable charge pump */
    charge_pump_enable(0);

    return 0;
}

int hv_set_voltage(uint16_t voltage) {
    /* Clamp to safe range */
    if (voltage < 50) voltage = 50;
    if (voltage > HV_MAX_VOLTAGE) voltage = HV_MAX_VOLTAGE;

    hv_target_voltage = voltage;

    /* Map voltage (50–400V) to DAC control voltage.
     * The charge pump output is roughly proportional to the drive level.
     * Drive level 0–3.3V maps to 0–400V output.
     * So DAC mV = voltage * 3300 / 400 = voltage * 8.25 */
    uint16_t dac_mv = (uint16_t)((uint32_t)voltage * 3300U / 400U);
    set_dac_voltage(dac_mv);

    return 0;
}

int hv_charge(void) {
    if (hv_target_voltage == 0) {
        return -1;  /* voltage not set */
    }

    hv_charged_flag = 0;

    /* Enable charge pump */
    charge_pump_enable(1);

    /* Wait for voltage to reach target (with tolerance) */
    uint32_t start = millis();
    uint16_t target = hv_target_voltage;
    uint16_t tolerance = target / 20;  /* 5% tolerance */

    while ((millis() - start) < HV_CHARGE_TIMEOUT) {
        uint16_t actual = adc_to_voltage(read_hv_adc());
        if (actual >= (target - tolerance) && actual <= (target + tolerance)) {
            hv_charged_flag = 1;
            return 0;
        }
        delay_ms(5);
    }

    /* Timeout — did not reach target */
    charge_pump_enable(0);
    return -1;
}

int hv_discharge(void) {
    /* Disable charge pump */
    charge_pump_enable(0);

    /* Set DAC to 0 to stop charging */
    set_dac_voltage(0);

    /* Active discharge: enable bleeder resistor via GPIO
     * (would be a MOSFET gate connected to a GPIO pin)
     * For safety, we wait until voltage drops below safe level. */
    uint32_t start = millis();
    while ((millis() - start) < 2000) {
        uint16_t actual = adc_to_voltage(read_hv_adc());
        if (actual < 5) break;  /* below 5V is safe */
        delay_ms(5);
    }

    hv_charged_flag = 0;
    hv_target_voltage = 0;
    return 0;
}

int hv_is_charged(void) {
    return hv_charged_flag;
}

uint16_t hv_read_actual(void) {
    return adc_to_voltage(read_hv_adc());
}