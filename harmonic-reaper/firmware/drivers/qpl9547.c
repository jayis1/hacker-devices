/*
 * qpl9547.c — PA + AGC driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The QPL9547 is a 2.4 GHz power amplifier with integrated digital
 * attenuator (30 dB range, 0.5 dB steps) controlled via SPI. We also
 * drive a 6-bit R-2R DAC on the PA's VGS bias pin for fast coarse AGC
 * in pulse mode (the SPI attenuator can't settle within a 200 ns pulse,
 * but the analog bias DAC can).
 *
 * Total gain control = 30 dB (SPI) + ~18 dB (DAC) = 48 dB range.
 */
#include "../registers.h"
#include "../board.h"
#include "qpl9547.h"

/* Power table: maps dBm → (atten_code, dac_code).
 * atten_code: 0..63 (0.5 dB/step, 0 = max gain)
 * dac_code:   0..63 (bias voltage, 0 = min bias / max attenuation)
 *
 * This table was calibrated against a reference target at 30 cm.
 */
typedef struct { int8_t dbm; uint8_t atten; uint8_t dac; } power_entry_t;
static const power_entry_t power_table[] = {
    { -10, 60, 10 },
    {  -5, 50, 15 },
    {   0, 40, 25 },
    {  +5, 30, 35 },
    { +10, 20, 48 },
    { +15,  5, 58 },
};
#define POWER_TABLE_LEN (sizeof(power_table)/sizeof(power_table[0]))

static int8_t  current_power_dbm = 0;
static uint8_t current_atten = 40;
static uint8_t current_dac = 25;

/* ---- SPI write to the QPL9547 digital attenuator (6-bit word) ---- */
static void qpl_spi_write(uint8_t atten_code)
{
    /* Same SPIM0 bus as ADF4159; we just assert QPL9547 CS instead */
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << QPL9547_CS_PIN);  /* CS low */

    uint8_t buf = atten_code & 0x3Fu;
    SPIM_TXD_PTR(ADF4159_SPI_BASE) = (uint32_t)&buf;
    SPIM_TXD_MAXCNT(ADF4159_SPI_BASE) = 1;
    SPIM_RXD_MAXCNT(ADF4159_SPI_BASE) = 0;
    SPIM_START_TX(ADF4159_SPI_BASE);
    while (!SPIM_END_EVENT(ADF4159_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(ADF4159_SPI_BASE);

    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << QPL9547_CS_PIN);  /* CS high */
}

/* ---- 6-bit R-2R DAC drive (parallel GPIO) ---- */
static void qpl_dac_write(uint8_t code)
{
    code &= 0x3Fu;
    uint32_t port0_mask = ((code & 0x01u) << AGC_DAC_D0_PIN) |
                          ((code & 0x02u) << (AGC_DAC_D1_PIN - 1));
    /* P1 pins are on a separate port base */
    uint32_t port1_base = NRF_GPIO_BASE + 0x1000u;
    uint32_t port1_mask = ((code & 0x04u) << (AGC_DAC_D2_PIN - 32 - 2)) |
                          ((code & 0x08u) << (AGC_DAC_D3_PIN - 32 - 3)) |
                          ((code & 0x10u) << (AGC_DAC_D4_PIN - 32 - 4)) |
                          ((code & 0x20u) << (AGC_DAC_D5_PIN - 32 - 5));

    /* Clear all bits first, then set */
    GPIO_OUTCLR(NRF_GPIO_BASE) =
        ((1u << AGC_DAC_D0_PIN) | (1u << AGC_DAC_D1_PIN));
    GPIO_OUTCLR(port1_base) =
        ((1u << (AGC_DAC_D2_PIN & 31u)) |
         (1u << (AGC_DAC_D3_PIN & 31u)) |
         (1u << (AGC_DAC_D4_PIN & 31u)) |
         (1u << (AGC_DAC_D5_PIN & 31u)));

    GPIO_OUTSET(NRF_GPIO_BASE) = port0_mask;
    GPIO_OUTSET(port1_base) = port1_mask;
}

void qpl9547_init(void)
{
    /* PA TX_EN low (off) */
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << QPL9547_TX_EN_PIN);
    /* Start at minimum output (max attenuation) for safety */
    qpl_spi_write(63);
    qpl_dac_write(0);
    current_atten = 63;
    current_dac = 0;
    current_power_dbm = -15;   /* effectively off */
}

void qpl9547_set_power(int8_t dbm)
{
    /* Clamp */
    if (dbm < -10) dbm = -10;
    if (dbm > +15) dbm = +15;

    /* Find closest table entry */
    uint8_t best = 0;
    int8_t best_err = 127;
    for (uint32_t i = 0; i < POWER_TABLE_LEN; i++) {
        int8_t err = dbm - power_table[i].dbm;
        if (err < 0) err = -err;
        if (err < best_err) { best_err = err; best = (uint8_t)i; }
    }
    current_atten = power_table[best].atten;
    current_dac = power_table[best].dac;
    current_power_dbm = power_table[best].dbm;

    qpl_spi_write(current_atten);
    qpl_dac_write(current_dac);
}

void qpl9547_step_power(int8_t delta_db)
{
    int16_t new_dbm = (int16_t)current_power_dbm + delta_db;
    qpl9547_set_power((int8_t)new_dbm);
}

void qpl9547_tx_enable(uint8_t en)
{
    if (en) GPIO_OUTSET(NRF_GPIO_BASE) = (1u << QPL9547_TX_EN_PIN);
    else   GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << QPL9547_TX_EN_PIN);
}

int8_t qpl9547_get_power(void) { return current_power_dbm; }

/* EOF — qpl9547.c — jayis1 */