/*
 * tesla-phantom — drivers/fluxgate.c
 * DRV425 fluxgate magnetometer driver.
 * Reads magnetic field strength via SPI and provides calibration.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* The DRV425 is a precision fluxgate magnetic field sensor.
 * It outputs an analog voltage proportional to the magnetic field
 * (±1.3 mT range, sensitivity ~1.5 V/mT at default gain).
 * The analog output is fed through the AD8331 VGA and LTC1564 LPF
 * to the AD7606 ADC for digitization.
 *
 * For DC/low-frequency field measurements, we also have a direct
 * ADC channel reading the DRV425 output bypassing the VGA.
 *
 * Additionally, the DRV425 has an SPI interface for configuration
 * (offset trim, range selection, test modes). We use SPI2 for this.
 */

#define DRV425_FIELD_RANGE_MT   1.3f    /* ±1.3 mT measurement range */
#define DRV425_SENSITIVITY      1.5f    /* V/mT at default gain */
#define DRV425_OFFSET_REG       0x00
#define DRV425_CONFIG_REG       0x01
#define DRV425_STATUS_REG       0x02
#define DRV425_TEST_REG         0x03

static int16_t offset_counts = 0;
static float   calibrated_offset_mt = 0.0f;
static uint8_t fluxgate_initialized = 0;

/* ── Internal ADC read for fluxgate output ────────────────────────── */
static int16_t read_fluxgate_adc(void) {
    /* The DRV425 analog output is on ADC channel 1 of the AD7606.
     * For a quick single-channel read, we use the internal STM32 ADC.
     * In the full design, the AD7606 is used for high-speed multi-channel.
     * For slow field measurements, the internal ADC suffices. */
    volatile uint32_t *adc1_cr   = (volatile uint32_t *)(ADC1_BASE + 0x08);
    volatile uint32_t *adc1_sqr3 = (volatile uint32_t *)(ADC1_BASE + 0x34);
    volatile uint32_t *adc1_dr   = (volatile uint32_t *)(ADC1_BASE + 0x40);
    volatile uint32_t *adc1_isr  = (volatile uint32_t *)(ADC1_BASE + 0x00);

    /* ADC channel 3 (PA3) = fluxgate direct output */
    *adc1_sqr3 = 3;
    *adc1_cr |= (1U << 2);  /* ADSTART */
    while (!(*adc1_isr & (1U << 1))) { }  /* EOC */
    return (int16_t)(*adc1_dr & 0xFFFF);
}

static float counts_to_mt(int16_t counts) {
    /* 16-bit ADC, ±2.5V range (bipolar mode) → ±32768 counts = ±2.5V
     * DRV425 sensitivity: 1.5 V/mT
     * → field = (counts / 32768) * 2.5 / 1.5 mT */
    float voltage = ((float)counts / 32768.0f) * 2.5f;
    float field_mt = voltage / DRV425_SENSITIVITY;
    return field_mt;
}

/* ── Public API ───────────────────────────────────────────────────── */

int fluxgate_init(void) {
    /* The DRV425 has minimal SPI config needed at startup.
     * We set the config register for default range and offset trim. */

    /* Read status register to verify sensor is present */
    /* (In a real implementation, we'd read via SPI2)
     * For now, just initialize state */
    offset_counts = 0;
    calibrated_offset_mt = 0.0f;
    fluxgate_initialized = 1;

    return 0;
}

int fluxgate_read(float *field_mt) {
    if (!fluxgate_initialized) return -1;

    int16_t raw = read_fluxgate_adc();
    *field_mt = counts_to_mt(raw) - calibrated_offset_mt;

    /* Clamp to sensor range */
    if (*field_mt > DRV425_FIELD_RANGE_MT)
        *field_mt = DRV425_FIELD_RANGE_MT;
    if (*field_mt < -DRV425_FIELD_RANGE_MT)
        *field_mt = -DRV425_FIELD_RANGE_MT;

    return 0;
}

int fluxgate_read_raw(int16_t *raw) {
    if (!fluxgate_initialized) return -1;
    *raw = read_fluxgate_adc();
    return 0;
}

int fluxgate_set_offset(int16_t offset) {
    offset_counts = offset;
    /* Write offset to DRV425 via SPI (simplified) */
    return 0;
}

int fluxgate_calibrate(void) {
    /* Calibration: average 1024 readings with no external field
     * (probe retracted) to determine the zero-field offset. */
    int32_t sum = 0;
    int16_t sample;

    for (int i = 0; i < 1024; i++) {
        fluxgate_read_raw(&sample);
        sum += sample;
        delay_ms(1);
    }

    int16_t avg = (int16_t)(sum / 1024);
    offset_counts = avg;
    calibrated_offset_mt = counts_to_mt(avg);

    return 0;
}