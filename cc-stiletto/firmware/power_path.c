/*
 * power_path.c — VBUS / DAC / eFuse / HRTIM / INA226 driver for CC-Stiletto.
 *
 * Manages the analog power delivery path: the MAX5370 DAC that sets the VBUS
 * target voltage, the TPS25940 eFuse that limits current, the MOSFET bank +
 * HRTIM one-shot used for glitch pulses, and the two INA226 monitors that
 * report voltage/current on each leg.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#include "power_path.h"
#include "board.h"
#include "registers.h"
#include "pd_phy.h"      /* for i2c primitives (shared I2C3 with DAC) */

/* ---- I2C3 setup (shared by DAC + INA226) ---------------------------------- */
static void i2c3_setup(void)
{
    I2C3->CR1 = 0;
    I2C3->TIMINGR = (3u << 28) | (0x3Fu << 8) | (0x3Fu << 0)
                   | (0x2u << 20) | (0x4u << 16);
    I2C3->OAR1 = 0;
    I2C3->CR1 = I2C_CR1_PE;
}

static int i2c3_write(uint8_t addr8, const uint8_t *buf, uint8_t n)
{
    uint32_t to = 100000u;
    while (I2C3->ISR & I2C_ISR_BUSY) if (--to == 0) return -1;

    uint32_t cr2 = ((uint32_t)addr8 << 1) | ((uint32_t)n << 16)
                 | I2C_CR2_AUTOEND | I2C_CR2_START;
    I2C3->CR2 = cr2;
    for (uint8_t i = 0; i < n; i++) {
        to = 50000u;
        while (!(I2C3->ISR & I2C_ISR_TXIS)) {
            if (I2C3->ISR & I2C_ISR_NACKF) { I2C3->ICR = I2C_ISR_NACKF; return -2; }
            if (--to == 0) return -3;
        }
        I2C3->TXDR = buf[i];
    }
    to = 50000u;
    while (!(I2C3->ISR & I2C_ISR_STOPF)) if (--to == 0) return -4;
    I2C3->ICR = I2C_ISR_STOPF;
    return n;
}

static int i2c3_read(uint8_t addr8, uint8_t *buf, uint8_t n)
{
    uint32_t to = 100000u;
    while (I2C3->ISR & I2C_ISR_BUSY) if (--to == 0) return -1;
    uint32_t cr2 = ((uint32_t)addr8 << 1) | ((uint32_t)n << 16)
                 | I2C_CR2_NACK | I2C_CR2_AUTOEND | I2C_CR2_START;
    I2C3->CR2 = cr2;
    for (uint8_t i = 0; i < n; i++) {
        to = 50000u;
        while (!(I2C3->ISR & I2C_ISR_RXNE)) if (--to == 0) return -5;
        buf[i] = (uint8_t)I2C3->RXDR;
    }
    to = 50000u;
    while (!(I2C3->ISR & I2C_ISR_STOPF)) if (--to == 0) break;
    I2C3->ICR = I2C_ISR_STOPF;
    return n;
}

/* ---- MAX5370 DAC --------------------------------------------------------- */
/* MAX5370 register: 0x01 = output A, 0x02 = output B, etc. 8-bit code. */
#define MAX5370_REG_OUTA   0x01u
#define MAX5370_REG_OUTB   0x02u
#define MAX5370_REG_OUTC   0x03u
#define MAX5370_REG_OUTD   0x04u

static int dac_write(uint8_t ch, uint8_t code)
{
    uint8_t buf[2] = { ch, code };
    return i2c3_write((uint8_t)(DAC_ADDR_7BIT << 1), buf, 2) == 2 ? 0 : -1;
}

/* Map a target VBUS millivolt to an 8-bit DAC code.
 * The LM5180 flyback feedback divider gives: Vbus = 0.5 V + code * (48 V / 255)
 * i.e. ~188 mV per LSB. We clamp to the hardware safe ceiling. */
int power_path_set_vbus(uint16_t mv)
{
    if (mv > VBUS_ABS_MAX_MV) mv = VBUS_ABS_MAX_MV;
    uint32_t code = ((uint32_t)(mv - 500u) * 255u) / 47500u;
    if (code > 255u) code = 255u;
    return dac_write(MAX5370_REG_OUTA, (uint8_t)code);
}

/* eFuse current limit: the TPS25940 uses an external resistor + a DAC-driven
 * offset.  We map 1500..5000 mA to an 8-bit code on DAC channel B. */
int power_path_set_ilimit(uint16_t ma)
{
    if (ma < 1500u) ma = 1500u;
    if (ma > 5000u) ma = 5000u;
    uint32_t code = ((uint32_t)(ma - 1500u) * 255u) / 3500u;
    return dac_write(MAX5370_REG_OUTB, (uint8_t)code);
}

/* ---- MOSFET bank + HRTIM glitch ----------------------------------------- */
/* The VBUS MOSFET bank gate is driven by HRTIM Timer A output 1. In normal
 * operation we hold it high (source enabled) via the set/reset registers.
 * For a glitch pulse we program a one-shot that pulls the gate low for
 * `low_ns` nanoseconds, then re-asserts.  HRTIM tick = 1 / 170 MHz ≈ 5.88 ns. */

static void hrtim_init(void)
{
    /* Enable HRTIM clock already done in RCC init. Configure Timer A:
     * period = 0xFFFF, CMP1 = period (continuous high), output active high. */
    HRTIM1->MCR = 0;   /* disable master */
    HRTIM1->TIMA_PER = 0x10000u;
    HRTIM1->TIMA_CMP1 = 0x10000u;
    HRTIM1->TIMA_CMP2 = 0x08000u;
    HRTIM1->TIMA_SET = 0x01u;    /* set output on period */
    HRTIM1->TIMA_RST = 0x02u;    /* reset on CMP2 */
    HRTIM1->TIMA_CR = 0x01u;     /* enable Timer A */
}

void power_path_source_enable(bool en)
{
    if (en) {
        /* eFuse enable + MOSFET gate high via HRTIM continuous-high output */
        GPIOC->BSRR = (1u << EFUSE_EN_PIN);    /* PC13 high */
        HRTIM1->TIMA_CMP2 = 0x10000u;          /* never reset -> stays high */
        HRTIM1->MCR |= (1u << 4);              /* enable Timer A output */
    } else {
        HRTIM1->MCR &= ~(1u << 4);
        GPIOC->BRR = (1u << EFUSE_EN_PIN);
    }
}

void power_path_sink_enable(bool en)
{
    /* Sink path uses a separate MOSFET (Q4) toggled via PC15. For simplicity
     * we mirror the source-enable path here; a full design would use a
     * bidirectional load switch. */
    if (en) GPIOC->BSRR = (1u << 15u);
    else    GPIOC->BRR  = (1u << 15u);
}

void power_path_glitch_pulse(uint32_t low_ns)
{
    /* Program CMP2 to `low_ns` worth of HRTIM ticks, then re-arm the timer so
     * the output drops for that duration and returns high. */
    uint32_t ticks = (low_ns * (HRTIM_FREQ_HZ / 1000000u)) / 1000u;
    if (ticks < 1u) ticks = 1u;
    if (ticks > 0xFFF0u) ticks = 0xFFF0u;
    HRTIM1->TIMA_CMP2 = ticks;
    HRTIM1->TIMA_CR = 0x01u;    /* re-trigger */
}

/* ---- INA226 telemetry --------------------------------------------------- */
#define INA226_REG_CONF   0x00u
#define INA226_REG_VSHUNT 0x01u
#define INA226_REG_VBUS   0x02u
#define INA226_REG_POWER  0x03u
#define INA226_REG_CURR   0x04u
#define INA226_REG_CAL    0x05u

static void ina226_init(uint8_t addr7)
{
    uint8_t buf[3];
    buf[0] = INA226_REG_CONF;
    buf[1] = 0x41u;   /* 1.1 ms conversion, 1.1 ms shunt, avg=1 */
    buf[2] = 0x9Fu;
    i2c3_write((uint8_t)(addr7 << 1), buf, 3);
    buf[0] = INA226_REG_CAL;
    buf[1] = 0x08u;   /* calibration for 5 mΩ shunt, ~3.2 mA/LSB */
    buf[2] = 0x00u;
    i2c3_write((uint8_t)(addr7 << 1), buf, 3);
}

static int ina226_read16(uint8_t addr7, uint8_t reg, uint16_t *out)
{
    uint8_t cmd = reg;
    if (i2c3_write((uint8_t)(addr7 << 1), &cmd, 1) != 1) return -1;
    uint8_t b[2];
    if (i2c3_read((uint8_t)(addr7 << 1) | 1u, b, 2) != 2) return -1;
    *out = ((uint16_t)b[0] << 8) | b[1];
    return 0;
}

void power_path_read_src(uint16_t *mv, int16_t *ma)
{
    uint16_t vbus = 0, vshunt = 0;
    ina226_read16(INA_SRC_ADDR_7BIT, INA226_REG_VBUS, &vbus);
    ina226_read16(INA_SRC_ADDR_7BIT, INA226_REG_VSHUNT, &vshunt);
    /* INA226 VBUS: 1.25 mV/LSB -> mV */
    *mv = (uint16_t)((uint32_t)vbus * 125u / 100u);
    /* Shunt: 2.5 µV/LSB; with 5 mΩ shunt -> 0.5 mA/LSB; signed. */
    int16_t signed_shunt = (int16_t)vshunt;
    *ma = (int16_t)(signed_shunt / 2);
}

void power_path_read_snk(uint16_t *mv, int16_t *ma)
{
    uint16_t vbus = 0, vshunt = 0;
    ina226_read16(INA_SNK_ADDR_7BIT, INA226_REG_VBUS, &vbus);
    ina226_read16(INA_SNK_ADDR_7BIT, INA226_REG_VSHUNT, &vshunt);
    *mv = (uint16_t)((uint32_t)vbus * 125u / 100u);
    int16_t signed_shunt = (int16_t)vshunt;
    *ma = (int16_t)(signed_shunt / 2);
}

/* ---- Temperature (NTC via ADC1 channel 5) -------------------------------- */
uint8_t power_path_read_temp(void)
{
    /* Start a conversion on channel 5, poll EOC, read DR.
     * This is a minimal single-shot sequence. */
    ADC1->CR = 0;                        /* ensure disabled */
    ADC1->CR = (1u << 0);                /* ADEN */
    /* Configure sequence: channel 5, length 1 */
    ADC1->SQR1 = (5u << 6) | (0u << 0);   /* L=0 (1 conv), SQ1=ch5 */
    ADC1->CR |= (1u << 2);               /* ADSTART */
    uint32_t to = 10000u;
    while (!(ADC1->ISR & (1u << 2))) if (--to == 0) return 255u;
    uint16_t raw = (uint16_t)ADC1->DR;
    /* Rough NTC mapping: 12-bit ADC, 3.3 V ref, NTC 10k to GND, 10k pull-up.
     * Steinhart-Hart omitted for brevity; use a linear approximation. */
    /* 25 °C ≈ 2048, 85 °C ≈ 700, slope ≈ -0.10 °C/LSB */
    int32_t t = 25 + (int32_t)(2048 - raw) / 10;
    if (t < 0) t = 0;
    if (t > 255) t = 255;
    return (uint8_t)t;
}

/* ---- eFuse fault -------------------------------------------------------- */
bool power_path_efuse_fault(void)
{
    return (GPIOC->IDR & (1u << EFUSE_FLT_PIN)) == 0;   /* active low */
}

/* ---- Init --------------------------------------------------------------- */
void power_path_init(void)
{
    i2c3_setup();
    hrtim_init();
    ina226_init(INA_SRC_ADDR_7BIT);
    ina226_init(INA_SNK_ADDR_7BIT);
    /* Start with VBUS DAC at 5 V safe default */
    power_path_set_vbus(5000u);
    power_path_set_ilimit(3000u);
}