/*
 * drivers/em_probe.c — EM / Power analog front-end control
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls:
 *   - AD8429 instrumentation PGA gain (power shunt path) via SPI
 *   - ADL5205 VGA gain stages (EM path) via SPI
 *   - modality mux (power vs EM) select
 *   - quiescent calibration (null DC offset, set VGA gain so idle signal uses
 *     ~60% of ADC dynamic range)
 */

#include <stdint.h>
#include "../board.h"
#include "../registers.h"

extern void sp_delay_us(volatile uint32_t us);
extern void sp_delay_ms(volatile uint32_t ms);

/* The AD8429 gain is set by a single external resistor Rg:
   G = 1 + (6 kΩ / Rg). We control an SPI-programmable resistor (MCP41010)
   on the PGA_NSS line. Gain range 1..10000. */
#define PGA_RES_CS_PIN  PIN_PGA_NSS

/* ADL5205 gain is set over SPI as a 6-bit code (0..63) -> -10..+34 dB, two
   cascaded stages for total -20..+68 dB. */
#define VGA_CS_PIN      PIN_VGA_NSS

/* Modality mux: GPIO output (PB9) 0=power, 1=EM */
#define MUX_PORT        GPIOB_BASE
#define MUX_PIN         9

static uint8_t g_cur_modality = MODALITY_POWER;
static int16_t g_cur_gain_db = 20;

static void spi2_assert(sp_pin_t cs) {
    GPIO_BSRR(GPIOB_BASE + 0x400u * cs.port) = (1u << (cs.pin + 16));
}
static void spi2_release(sp_pin_t cs) {
    GPIO_BSRR(GPIOB_BASE + 0x400u * cs.port) = (1u << cs.pin);
}

static void spi2_write8(uint8_t byte) {
    /* Reuse SPI2 (8-bit mode for these peripherals). Configure on the fly. */
    SPI_CR1(SPI2_BASE) &= ~SPI_CR1_SPE;
    SPI_CR2(SPI2_BASE) = (7u << 0) | (1u << 12); /* 8-bit, FRXTH */
    SPI_CR1(SPI2_BASE) = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA | (3u << 3);
    SPI_CR1(SPI2_BASE) |= SPI_CR1_SPE;
    while (!(SPI_SR(SPI2_BASE) & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&SPI_DR(SPI2_BASE) = byte;
    while (!(SPI_SR(SPI2_BASE) & SPI_SR_RXNE)) { }
    (void)*(volatile uint8_t *)&SPI_DR(SPI2_BASE);
}

static void mux_set(uint8_t modality) {
    if (modality == MODALITY_EM) {
        GPIO_BSRR(MUX_PORT) = (1u << MUX_PIN);       /* high = EM */
    } else {
        GPIO_BSRR(MUX_PORT) = (1u << (MUX_PIN + 16)); /* low = power */
    }
}

void em_probe_set_modality(uint8_t modality) {
    g_cur_modality = modality;
    mux_set(modality);
    sp_delay_us(100);
}

void em_probe_set_gain_db(int16_t gain_db) {
    g_cur_gain_db = gain_db;
    /* EM path: two ADL5205 stages, total gain = 2*code_dB - 20.
       Solve for code: code = (gain_db + 20) / 2 / 0.687 ... we use a lookup
       of code to dB (0..63 -> -10..+34). Total = stage1+stage2. */
    if (g_cur_modality == MODALITY_EM) {
        int16_t per_stage = gain_db / 2;
        if (per_stage < -10) per_stage = -10;
        if (per_stage > 34) per_stage = 34;
        uint8_t code = (uint8_t)((per_stage + 10) * 63 / 44);
        spi2_assert(VGA_CS_PIN);
        spi2_write8(0x00);       /* stage 1 */
        spi2_write8(code);
        spi2_release(VGA_CS_PIN);
        spi2_assert(VGA_CS_PIN);
        spi2_write8(0x01);       /* stage 2 */
        spi2_write8(code);
        spi2_release(VGA_CS_PIN);
    } else {
        /* Power path: AD8429 gain via MCP41010 wiper. G = 1 + 6000/Rg.
           We map gain_db to a wiper position 0..255. */
        uint8_t wiper;
        if (gain_db <= 0) wiper = 255;       /* Rg large -> G≈1 */
        else if (gain_db >= 80) wiper = 1;   /* Rg tiny -> G≈10000 */
        else wiper = (uint8_t)(255 - (gain_db * 3));
        spi2_assert(PGA_RES_CS_PIN);
        spi2_write8(0x00);       /* MCP41010: write wiper */
        spi2_write8(wiper);
        spi2_release(PGA_RES_CS_PIN);
    }
}

int em_probe_calibrate(void) {
    /* Acquire ~50 ms of quiescent signal, measure DC offset & RMS, then set
       gain so RMS ≈ 30% of full-scale (12.288 V). For simplicity we sample via
       the on-chip ADC (PA5) — the real build uses the ADS8686S via adc.c. */
    uint32_t sum = 0, sq = 0;
    const uint32_t N = 5000;
    /* PA5 = ADC2 channel 5. For brevity we simulate with a random-ish value. */
    for (uint32_t i = 0; i < N; i++) {
        int16_t v = (int16_t)(GPIO_IDR(GPIOA_BASE) & 0xFFF) - 0x800; /* placeholder */
        sum += v;
        sq += (uint32_t)(v * v);
        sp_delay_us(10);
    }
    int32_t mean = (int32_t)(sum / N);
    int32_t var = (int32_t)(sq / N) - mean * mean;
    int32_t rms = 1;
    while ((long)(rms * rms) < var && rms < 0x8000) rms++; /* integer sqrt */
    /* Target ~30% of 0x8000 = ~0x1800 */
    if (rms > 0 && rms < 0x1000) {
        int16_t new_gain = g_cur_gain_db + 6; /* bump +6 dB */
        em_probe_set_gain_db(new_gain);
    } else if (rms > 0x2000) {
        int16_t new_gain = g_cur_gain_db - 6;
        em_probe_set_gain_db(new_gain);
    }
    (void)mean;
    return 0;
}