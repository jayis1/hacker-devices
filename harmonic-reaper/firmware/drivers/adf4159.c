/*
 * adf4159.c — TX synthesizer driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The ADF4159 is a 6 GHz fractional-N synthesizer. We run it at 2.4 GHz
 * fundamental with an external VCO. Control is 3-wire SPI (CLK, DATA, LE)
 * with 24-bit latch writes. The device has 11 registers (R0..R10) loaded
 * in a specific order on power-up.
 *
 * We use the SPI0 peripheral on the nRF52840 (see board.h).
 */
#include "../registers.h"
#include "../board.h"
#include "adf4159.h"

/* Register addresses (ADF4159 datasheet Rev. C, Table 7) */
#define ADF_REG_R0_FRAC     0x00u   /* FRAC/INT/PR1 */
#define ADF_REG_R1_MOD      0x01u   /* MOD2, phase */
#define ADF_REG_R2_CTRL     0x02u   /* control bits, muxout */
#define ADF_REG_R3_PHASEADJ 0x03u
#define ADF_REG_R4_FIXED    0x04u   /* R divider, charge pump */
#define ADF_REG_R5_FIXED    0x05u   /* R divider */
#define ADF_REG_R6_FIXED    0x06u
#define ADF_REG_R7_FIXED    0x07u
#define ADF_REG_R8_FIXED    0x08u
#define ADF_REG_R9_FIXED    0x09u
#define ADF_REG_R10_FIXED   0x0Au

/* Register write control bits (R0 format) */
#define ADF_R0_INT_MSB(pos) ((uint32_t)(pos) << 14)
#define ADF_CTRL_DBGEN      (1u << 2)
#define ADF_CTRL_ADCEN      (1u << 1)
#define ADF_CTRL_ADCS       (1u)

/* Reference oscillator: 25 MHz TCXO, VCO = 2400 MHz, PFD = 25 MHz */
#define ADF_REF_HZ          25000000u
#define ADF_INT_VALUE       96u          /* INT = 96 → 2400 MHz @ 25 MHz PFD */
#define ADF_FRAC_DEFAULT    0u
#define ADF_MOD_DEFAULT     1u           /* MOD=1 → fractional disabled */

/* ---- low-level SPI write (24-bit word, MSB first) ---- */
static void adf_spi_write(uint32_t word)
{
    /* LE (latch enable) high during idle; pulse low during write */
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << ADF4159_LE_PIN);

    /* SPIM0 send 3 bytes */
    uint8_t buf[3];
    buf[0] = (uint8_t)(word >> 16);
    buf[1] = (uint8_t)(word >> 8);
    buf[2] = (uint8_t)(word);

    SPIM_TXD_PTR(ADF4159_SPI_BASE) = (uint32_t)buf;
    SPIM_TXD_MAXCNT(ADF4159_SPI_BASE) = 3;
    SPIM_RXD_MAXCNT(ADF4159_SPI_BASE) = 0;
    SPIM_START_TX(ADF4159_SPI_BASE);
    while (!SPIM_END_EVENT(ADF4159_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(ADF4159_SPI_BASE);

    /* LE low to latch, then back high */
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << ADF4159_LE_PIN);
    nrf_delay_us(1);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << ADF4159_LE_PIN);
}

/* ---- register write with address encoding (bits 21:3 = data, 2:0 = addr) ---- */
static void adf_write_reg(uint8_t addr, uint32_t data)
{
    uint32_t word = ((data & 0x1FFFFFu) << 3) | (addr & 0x07u);
    adf_spi_write(word);
}

void adf4159_init(void)
{
    /* Configure SPIM0 pins */
    SPIM_PSEL_SCK(ADF4159_SPI_BASE)  = ADF4159_SCK_PIN;
    SPIM_PSEL_MOSI(ADF4159_SPI_BASE) = ADF4159_MOSI_PIN;
    SPIM_PSEL_MISO(ADF4159_SPI_BASE) = ADF4159_MISO_PIN;
    SPIM_FREQUENCY(ADF4159_SPI_BASE) = SPIM_FREQ_8M;
    SPIM_CONFIG(ADF4159_SPI_BASE)   = SPIM_MODE0;
    SPIM_ENABLE(ADF4159_SPI_BASE)   = 1u;

    /* Enable ADF4159 chip */
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << ADF4159_CE_PIN);
    nrf_delay_ms(5);

    /* Initialization register sequence (ADF4159 datasheet §initialization) */
    adf_write_reg(ADF_REG_R10_FIXED, 0x00001F);  /* R10 — fixed */
    adf_write_reg(ADF_REG_R9_FIXED,  0x00C123);  /* R9 — fixed, ramp config */
    adf_write_reg(ADF_REG_R8_FIXED,  0x000000);  /* R8 — fixed */
    adf_write_reg(ADF_REG_R7_FIXED,  0x000000);  /* R7 — fixed */
    adf_write_reg(ADF_REG_R6_FIXED,  0x000080);  /* R6 — fixed, cp gain */
    adf_write_reg(ADF_REG_R5_FIXED,  0x00C001);  /* R5 — fixed */
    adf_write_reg(ADF_REG_R4_FIXED,  0x008058);  /* R4 — R-divider=1, CP=5mA */
    adf_write_reg(ADF_REG_R3_PHASEADJ, 0x000000); /* R3 */
    adf_write_reg(ADF_REG_R2_CTRL,   0x010110);  /* R2 — control, LD enabled */
    adf_write_reg(ADF_REG_R1_MOD,    (ADF_MOD_DEFAULT << 8)); /* R1 — MOD */
    adf_write_reg(ADF_REG_R0_FRAC,   ADF_INT_VALUE);  /* R0 — INT=96 */

    /* Default to CW mode (no ramp) */
    adf_write_reg(ADF_REG_R9_FIXED, 0x00C123);  /* clear ramp enable */

    /* Set frequency to 2.4 GHz */
    adf4159_set_freq(2400000000u);
}

void adf4159_set_freq(uint32_t freq_hz)
{
    if (freq_hz < 2000000000u || freq_hz > 2600000000u) return;
    /* INT = freq / PFD, FRAC = fractional part */
    uint32_t int_val = freq_hz / ADF_REF_HZ;
    uint32_t rem = freq_hz % ADF_REF_HZ;
    /* Use MOD = 1000 for sub-Hz resolution */
    uint32_t mod = 1000u;
    uint32_t frac = (rem * mod) / ADF_REF_HZ;

    /* Write R1 (MOD) then R0 (INT/FRAC) */
    adf_write_reg(ADF_REG_R1_MOD, (mod << 8) | (frac & 0xFFu));
    adf_write_reg(ADF_REG_R0_FRAC, (int_val & 0x7FFFu) | ((frac >> 8) << 15));
}

void adf4159_enable(uint8_t en)
{
    if (en) {
        GPIO_OUTSET(NRF_GPIO_BASE) = (1u << ADF4159_CE_PIN);
    } else {
        GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << ADF4159_CE_PIN);
    }
}

void adf4159_set_pulse(uint32_t prf_hz, uint16_t pulse_width_ns)
{
    if (prf_hz == 0) {
        /* CW mode — disable ramp generator */
        adf_write_reg(ADF_REG_R9_FIXED, 0x00C123);  /* ramp disable */
        return;
    }
    /* Configure ADF4159 ramp generator for pulsed CW.
     * Ramp step = pulse_width_ns, ramp interval = 1/prf_hz.
     * The ADF4159 has a built-in ramp generator that toggles the RF output
     * with programmable width/period — perfect for pulsed NLJD operation. */
    uint32_t step = (pulse_width_ns * ADF_REF_HZ) / 1000000000u;
    uint32_t period = ADF_REF_HZ / prf_hz;
    adf_write_reg(ADF_REG_R9_FIXED, 0x00C103 | (1u << 16));  /* ramp enable */
    adf_write_reg(ADF_REG_R8_FIXED, step & 0x1FFFFFu);
    adf_write_reg(ADF_REG_R7_FIXED, period & 0x1FFFFFu);
}

void adf4159_set_quiet(uint8_t en)
{
    /* In quiet mode we dither the PRF by ±15% on every pulse using
     * the ADF4159's pseudo-random ramp dither bit. This spreads the
     * spectral signature so an adversary's RF activity monitor sees
     * noise rather than a stable pulse train. */
    uint32_t r9 = (en) ? (0x00C103 | (1u << 17)) : (0x00C103 | (1u << 16));
    adf_write_reg(ADF_REG_R9_FIXED, r9);
}

/* EOF — adf4159.c — jayis1 */