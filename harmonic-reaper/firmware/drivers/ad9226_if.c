/*
 * ad9226_if.c — 3f0 IF channel driver
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#include "../registers.h"
#include "../board.h"
#include "ad9226_if.h"

/* The ADL5802 mixer bias is set via an I²C DAC (MCP4725 @ 0x60) on the
 * TWIM0 bus (shared with the DRV2605 haptic). We write a 12-bit code
 * proportional to the desired bias current (0..100%).
 *
 * We use a minimal bit-banged I²C master here because the nRF52840's
 * TWIM peripheral is allocated to the haptic driver; this channel is
 * set once at boot and rarely changed, so bit-banging is fine.
 */
#define I2C_SDA_PIN   HAPTIC_SDA_PIN
#define I2C_SCL_PIN   HAPTIC_SCL_PIN
#define ADL5802_DAC_ADDR 0x60u

static void i2c_delay(void) { nrf_delay_us(5); }

static void i2c_sda_low(void)  { GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << I2C_SDA_PIN); }
static void i2c_sda_high(void) { GPIO_OUTSET(NRF_GPIO_BASE) = (1u << I2C_SDA_PIN); }
static void i2c_scl_low(void)  { GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << I2C_SCL_PIN); }
static void i2c_scl_high(void) { GPIO_OUTSET(NRF_GPIO_BASE) = (1u << I2C_SCL_PIN); }

static uint8_t i2c_sda_read(void)
{
    /* Reconfigure SDA as input, read, then restore as output */
    GPIO_PIN_CNF(NRF_GPIO_BASE, I2C_SDA_PIN) = GPIO_CNF_DIR_INPUT;
    uint8_t v = (GPIO_OUT(NRF_GPIO_BASE) >> I2C_SDA_PIN) & 1u;  /* simplified */
    GPIO_PIN_CNF(NRF_GPIO_BASE, I2C_SDA_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    return v;
}

static void i2c_start(void)
{
    i2c_sda_high(); i2c_scl_high(); i2c_delay();
    i2c_sda_low();  i2c_delay();
    i2c_scl_low();  i2c_delay();
}

static void i2c_stop(void)
{
    i2c_sda_low(); i2c_delay();
    i2c_scl_high(); i2c_delay();
    i2c_sda_high(); i2c_delay();
}

static uint8_t i2c_write_byte(uint8_t b)
{
    for (int i = 7; i >= 0; i--) {
        if (b & (1u << i)) i2c_sda_high(); else i2c_sda_low();
        i2c_delay();
        i2c_scl_high(); i2c_delay();
        i2c_scl_low(); i2c_delay();
    }
    /* ACK clock */
    i2c_scl_high(); i2c_delay();
    uint8_t ack = !i2c_sda_read();  /* ACK = SDA low during 9th clock */
    i2c_scl_low(); i2c_delay();
    return ack;
}

static void i2c_write_reg(uint8_t addr, uint8_t reg, uint16_t val)
{
    i2c_start();
    i2c_write_byte(addr << 1);    /* write address */
    i2c_write_byte(reg);
    i2c_write_byte((uint8_t)(val >> 8));
    i2c_write_byte((uint8_t)(val & 0xFFu));
    i2c_stop();
}

void ad9226_if_init(void)
{
    /* ADC PD pin: output, high (powered down initially) */
    GPIO_PIN_CNF(NRF_GPIO_BASE, AD9226_PD_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << AD9226_PD_PIN);

    /* Mixer EN pin: output, low (off initially) */
    GPIO_PIN_CNF(NRF_GPIO_BASE, ADL5802_EN_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << ADL5802_EN_PIN);

    /* I²C pins as open-drain outputs, idle high */
    GPIO_PIN_CNF(NRF_GPIO_BASE, I2C_SDA_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_PIN_CNF(NRF_GPIO_BASE, I2C_SCL_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    i2c_sda_high();
    i2c_scl_high();
}

void ad9226_power_down(uint8_t pd)
{
    if (pd) GPIO_OUTSET(NRF_GPIO_BASE) = (1u << AD9226_PD_PIN);
    else   GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << AD9226_PD_PIN);
}

void adl5802_enable(uint8_t en)
{
    if (en) {
        GPIO_OUTSET(NRF_GPIO_BASE) = (1u << ADL5802_EN_PIN);
        /* Default bias = 50% */
        adl5802_set_bias(50);
    } else {
        GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << ADL5802_EN_PIN);
    }
}

void adl5802_set_bias(uint8_t bias_pct)
{
    if (bias_pct > 100) bias_pct = 100;
    /* Map 0..100% to 12-bit DAC code (0..4095).
     * The ADL5802 bias current is roughly linear in the DAC voltage.
     * We target 0.5V (min) at 0% and 3.3V (max) at 100%. */
    uint16_t dac_code = (uint16_t)((bias_pct * 4095u) / 100u);
    /* MCP4725: write to volatile DAC register (reg=0x40) */
    i2c_write_reg(ADL5802_DAC_ADDR, 0x40u, dac_code);
}

/* EOF — ad9226_if.c — jayis1 */