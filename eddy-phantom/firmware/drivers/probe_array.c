/*
 * eddy-phantom — probe_array.c
 * 4-element near-field H-field probe array management.
 * Controls VGA gain (AD8331) via MCP4922 DACs and
 * anti-alias filter cutoff (LTC1564-7) via MCP41010 digital pot.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── VGA gain ranges ──────────────────────────────────────────── */
#define VGA_MIN_GAIN_DB    0       /* 0 dB (minimum) */
#define VGA_MAX_GAIN_DB    48      /* 48 dB (maximum) */
#define VGA_GAIN_STEPS     256     /* MCP4922 has 12-bit resolution, 4096 steps */
#define VGA_DEFAULT_GAIN   21      /* 21 dB default mid-range */

#define FILTER_MIN_HZ      10000   /* 10 kHz minimum cutoff */
#define FILTER_MAX_HZ      500000  /* 500 kHz maximum cutoff */

/* Current gain settings for each channel (in dB × 10) */
static uint16_t g_current_gain[ADC_CHANNELS] = {
    VGA_DEFAULT_GAIN * 10,
    VGA_DEFAULT_GAIN * 10,
    VGA_DEFAULT_GAIN * 10,
    VGA_DEFAULT_GAIN * 10
};

/* ── SPI2 transfer (16-bit) for VGA DAC ───────────────────────── */
static void spi2_xfer16(uint16_t tx, uint8_t cs_port, uint8_t cs_pin)
{
    spi_regs_t *spi = SPI(2);

    /* CS low */
    gpio_clr((volatile gpio_regs_t *)(cs_port), cs_pin);

    /* Wait for TXE */
    volatile uint32_t timeout = 0xFFFF;
    while (!(spi->SR & SPI_SR_TXE) && --timeout);

    spi->DR = tx;

    /* Wait for RXNE (transfer complete) */
    timeout = 0xFFFF;
    while (!(spi->SR & SPI_SR_RXNE) && --timeout);
    (void)spi->DR;  /* read to clear RXNE */

    /* CS high */
    gpio_set((volatile gpio_regs_t *)(cs_port), cs_pin);
}

/* ── Bit-banged SPI for MCP41010 digital pot ──────────────────── */
static void pot_write(uint8_t cmd, uint8_t value)
{
    gpio_clr(GPIO(FILTER_SPI_CS_PORT), FILTER_SPI_CS_PIN);

    /* Send command byte */
    for (int i = 7; i >= 0; i--) {
        gpio_clr(GPIO(FILTER_SPI_SCK_PORT), FILTER_SPI_SCK_PIN);
        if (cmd & (1U << i))
            gpio_set(GPIO(FILTER_SPI_MOSI_PORT), FILTER_SPI_MOSI_PIN);
        else
            gpio_clr(GPIO(FILTER_SPI_MOSI_PORT), FILTER_SPI_MOSI_PIN);
        for (volatile int d = 0; d < 2; d++);
        gpio_set(GPIO(FILTER_SPI_SCK_PORT), FILTER_SPI_SCK_PIN);
        for (volatile int d = 0; d < 2; d++);
    }

    /* Send value byte */
    for (int i = 7; i >= 0; i--) {
        gpio_clr(GPIO(FILTER_SPI_SCK_PORT), FILTER_SPI_SCK_PIN);
        if (value & (1U << i))
            gpio_set(GPIO(FILTER_SPI_MOSI_PORT), FILTER_SPI_MOSI_PIN);
        else
            gpio_clr(GPIO(FILTER_SPI_MOSI_PORT), FILTER_SPI_MOSI_PIN);
        for (volatile int d = 0; d < 2; d++);
        gpio_set(GPIO(FILTER_SPI_SCK_PORT), FILTER_SPI_SCK_PIN);
        for (volatile int d = 0; d < 2; d++);
    }

    gpio_set(GPIO(FILTER_SPI_CS_PORT), FILTER_SPI_CS_PIN);
}

/* ── Set VGA gain for a single channel ──────────────────────────
 * gain_db_tenths: gain in dB × 10 (e.g., 210 = 21.0 dB)
 *
 * MCP4922 DAC output controls AD8606 gain set pin.
 * AD8331 gain formula: Gain(dB) = 5.4 - 0.5 * Vgain (V)
 * Vgain range: 0.1V to 1.2V → gain from 4.9 dB to 49.4 dB
 * DAC: 12-bit, Vref=3.3V → Vout = (DAC_code/4095) * 3.3
 */
void probe_set_gain(uint8_t channel, uint16_t gain_db_tenths)
{
    if (channel >= ADC_CHANNELS) return;
    if (gain_db_tenths < VGA_MIN_GAIN_DB * 10) gain_db_tenths = VGA_MIN_GAIN_DB * 10;
    if (gain_db_tenths > VGA_MAX_GAIN_DB * 10) gain_db_tenths = VGA_MAX_GAIN_DB * 10;

    g_current_gain[channel] = gain_db_tenths;

    /* Convert gain to Vgain voltage:
     * Gain(dB) = 5.4 - 0.5 * Vgain → Vgain = (5.4 - Gain/10) / 0.5
     * = (54 - gain_db_tenths) / 5.0
     */
    float gain_db = gain_db_tenths / 10.0f;
    float v_gain = (5.4f - gain_db) / 0.5f;
    if (v_gain < 0.1f) v_gain = 0.1f;
    if (v_gain > 1.2f) v_gain = 1.2f;

    /* Convert voltage to DAC code: code = (Vout / Vref) * 4095 */
    uint16_t dac_code = (uint16_t)((v_gain / 3.3f) * 4095.0f);
    if (dac_code > 4095) dac_code = 4095;

    /* MCP4922 command:
     *   bit 15: 0 = DAC A, 1 = DAC B
     *   bit 14: 1 = Vref buffered
     *   bit 13: 1 = gain = 1x
     *   bit 12: 1 = output enabled
     *   bits 11-0: data
     */
    uint16_t cmd;
    uint8_t cs_port, cs_pin;

    if (channel < 2) {
        /* Channels 0,1 → DAC1 (CS1) */
        cs_port = VGA_CS1_PORT;
        cs_pin = VGA_CS1_PIN;
        cmd = (channel == 0 ? 0xB000 : 0xF000) | dac_code;
    } else {
        /* Channels 2,3 → DAC2 (CS2) */
        cs_port = VGA_CS2_PORT;
        cs_pin = VGA_CS2_PIN;
        cmd = (channel == 2 ? 0xB000 : 0xF000) | dac_code;
    }

    spi2_xfer16(cmd, cs_port, cs_pin);
}

/* ── Set anti-alias filter cutoff ───────────────────────────────
 * MCP41010 digital pot controls the LTC1564-7 cutoff frequency.
 * The LTC1564-7 cutoff is set by an external resistor:
 *   fc = 128000 / R (where R is in ohms, fc in Hz)
 * The MCP41010 provides a programmable resistance (0-10 kΩ).
 * R_total = R_pot * (wiper_position / 255) + R_fixed
 */
void probe_set_filter_cutoff(uint32_t cutoff_hz)
{
    if (cutoff_hz < FILTER_MIN_HZ) cutoff_hz = FILTER_MIN_HZ;
    if (cutoff_hz > FILTER_MAX_HZ) cutoff_hz = FILTER_MAX_HZ;

    /* Calculate required resistance: R = 128000 / fc */
    float r_required = 128000.0f / cutoff_hz;

    /* Map to pot wiper position (0-255)
     * R_pot = r_required (simplified; in practice there's a fixed offset)
     * MCP41010: wiper position 0 = 0 Ω, 255 = 10000 Ω */
    uint8_t wiper = (uint8_t)((r_required / 10000.0f) * 255.0f);
    if (wiper > 255) wiper = 255;
    if (wiper < 1) wiper = 1;

    /* MCP41010 command: 0x13 = write data to potentiometer */
    pot_write(0x13, wiper);
}

/* ── Auto-range: adjust gain based on signal amplitude ──────────
 * Examines the first 64 samples of a burst and adjusts VGA gain
 * so that the signal amplitude is in the optimal range (10-80%
 * of full-scale). Returns the gain settings used.
 */
void probe_auto_range(int16_t *sample_buf, uint16_t *gain_out)
{
    /* Compute peak amplitude for each channel from the first 64 samples */
    int16_t peaks[ADC_CHANNELS] = {0, 0, 0, 0};

    for (int s = 0; s < 64 && s < BURST_SAMPLES; s++) {
        for (int ch = 0; ch < ADC_CHANNELS; ch++) {
            int16_t val = sample_buf[s * ADC_CHANNELS + ch];
            int16_t abs_val = (val < 0) ? -val : val;
            if (abs_val > peaks[ch])
                peaks[ch] = abs_val;
        }
    }

    /* Full-scale for ±5V range at 16-bit = 32767 */
    const int16_t fs = 32767;
    const int16_t target_low = fs * 10 / 100;   /* 10% of FS = 3276 */
    const int16_t target_high = fs * 80 / 100;  /* 80% of FS = 26213 */

    for (int ch = 0; ch < ADC_CHANNELS; ch++) {
        uint16_t new_gain = g_current_gain[ch];

        if (peaks[ch] < target_low) {
            /* Signal too small — increase gain */
            new_gain += 30;  /* +3 dB */
            if (new_gain > VGA_MAX_GAIN_DB * 10)
                new_gain = VGA_MAX_GAIN_DB * 10;
        } else if (peaks[ch] > target_high) {
            /* Signal too large — decrease gain */
            if (new_gain > 30)
                new_gain -= 30;  /* -3 dB */
            else
                new_gain = VGA_MIN_GAIN_DB * 10;
        }

        probe_set_gain(ch, new_gain);
        gain_out[ch] = new_gain;
    }
}

/* ── Initialize probe array ───────────────────────────────────── */
void probe_init(void)
{
    /* Initialize SPI2 for VGA DAC communication */
    spi_regs_t *spi = SPI(2);

    spi->CR1 = 0;
    spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV32;
    spi->CR2 = SPI_CR2_DS_16BIT | SPI_CR2_FRXTH;
    spi->CR1 |= SPI_CR1_SPE;

    /* Set default gain for all channels */
    for (int ch = 0; ch < ADC_CHANNELS; ch++) {
        probe_set_gain(ch, VGA_DEFAULT_GAIN * 10);
    }

    /* Set default filter cutoff to 350 kHz */
    probe_set_filter_cutoff(DSP_BANDPASS_HIGH_HZ);
}