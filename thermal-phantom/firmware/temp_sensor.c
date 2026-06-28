/*
 * temp_sensor.c - MLX90614 IR + DS18B20 contact temperature sensors
 *
 * MLX90614: I2C non-contact IR thermometer for target surface temp
 * DS18B20: 1-Wire contact sensor on Peltier cold plate
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "temp_sensor.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* Cached readings */
static float mlx_temp = 25.0f;
static float ds18b20_temp = 25.0f;
static float mcu_temp = 25.0f;
static bool mlx_present = false;
static bool ds18b20_present = false;

/* ============================================================
 * I2C low-level driver for MLX90614
 * ============================================================ */

static void i2c_init(void)
{
    uint32_t i2c_base = I2C1_BASE;
    
    /* Configure GPIO for I2C1: PB8=SCL, PB9=SDA, AF4 */
    uint32_t pb_base = GPIOB_BASE;
    
    /* SCL (PB8) */
    GPIO_MODER(pb_base) &= ~(3U << (8 * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_AF << (8 * 2));
    GPIO_OTYPER(pb_base) |= (1U << 8);  /* Open-drain */
    GPIO_OSPEEDR(pb_base) |= (GPIO_SPEED_HIGH << (8 * 2));
    REG32(pb_base + GPIO_AFRL_OFFSET) &= ~(0xFU << (8 * 4));
    REG32(pb_base + GPIO_AFRL_OFFSET) |= (4U << (8 * 4));  /* AF4 = I2C */
    
    /* SDA (PB9) */
    GPIO_MODER(pb_base) &= ~(3U << (9 * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_AF << (9 * 2));
    GPIO_OTYPER(pb_base) |= (1U << 9);
    GPIO_OSPEEDR(pb_base) |= (GPIO_SPEED_HIGH << (9 * 2));
    REG32(pb_base + GPIO_AFRL_OFFSET) &= ~(0xFU << (9 * 4));
    REG32(pb_base + GPIO_AFRL_OFFSET) |= (4U << (9 * 4));
    
    /* Configure I2C timing for 100kHz */
    I2C_TIMINGR(i2c_base) = 0x10909CEC;  /* 100kHz at 120MHz PCLK1 */
    I2C_CR1(i2c_base) = I2C_CR1_PE;  /* Enable peripheral */
}

static bool i2c_write_byte(uint8_t addr, uint8_t cmd)
{
    uint32_t i2c_base = I2C1_BASE;
    uint32_t timeout = 10000;
    
    /* Clear flags */
    I2C_ICR(i2c_base) = 0x3F38;
    
    /* Set address and command */
    I2C_CR2(i2c_base) = (addr << 1) | (1U << 16) |  /* 1 byte to write */
                        I2C_CR2_START;
    
    /* Wait for TXIS */
    while (!(I2C_ISR(i2c_base) & I2C_ISR_TXIS) && !(I2C_ISR(i2c_base) & I2C_ISR_NACKF)) {
        if (--timeout == 0) return false;
    }
    
    if (I2C_ISR(i2c_base) & I2C_ISR_NACKF)
        return false;
    
    I2C_TXDR(i2c_base) = cmd;
    
    /* Wait for transfer complete */
    timeout = 10000;
    while (!(I2C_ISR(i2c_base) & (I2C_ISR_TC | I2C_ISR_NACKF))) {
        if (--timeout == 0) return false;
    }
    
    if (I2C_ISR(i2c_base) & I2C_ISR_NACKF)
        return false;
    
    return true;
}

static bool i2c_read_bytes(uint8_t addr, uint8_t cmd, uint8_t *data, uint8_t len)
{
    uint32_t i2c_base = I2C1_BASE;
    uint32_t timeout = 10000;
    
    if (!i2c_write_byte(addr, cmd))
        return false;
    
    /* Now read 'len' bytes */
    I2C_CR2(i2c_base) = (addr << 1) | 1U |  /* Read direction */
                        ((uint32_t)len << I2C_CR2_NBYTES_SHIFT) |
                        I2C_CR2_START | I2C_CR2_STOP;
    
    for (uint8_t i = 0; i < len; i++) {
        timeout = 10000;
        while (!(I2C_ISR(i2c_base) & I2C_ISR_RXNE) && !(I2C_ISR(i2c_base) & I2C_ISR_NACKF)) {
            if (--timeout == 0) return false;
        }
        if (I2C_ISR(i2c_base) & I2C_ISR_NACKF)
            return false;
        data[i] = (uint8_t)I2C_RXDR(i2c_base);
    }
    
    /* Wait for stop */
    timeout = 10000;
    while (!(I2C_ISR(i2c_base) & I2C_ISR_STOPF)) {
        if (--timeout == 0) break;
    }
    
    I2C_ICR(i2c_base) = 0x3F38;
    return true;
}

/* ============================================================
 * MLX90614 IR Temperature Sensor
 * ============================================================ */

#define MLX_REG_TA    0x06  /* Ambient temperature */
#define MLX_REG_TOBJ1 0x07  /* Object temperature 1 */

static float mlx_raw_to_temp(uint8_t low, uint8_t high, uint8_t pec)
{
    (void)pec;  /* PEC not verified in this simplified driver */
    uint16_t raw = (uint16_t)low | ((uint16_t)high << 8);
    /* MLX90614: temp = raw * 0.02 - 273.15 (Kelvin to Celsius) */
    float temp = raw * 0.02f - 273.15f;
    return temp;
}

bool mlx_read_temp(float *temp_c)
{
    uint8_t data[3];
    
    if (!i2c_read_bytes(MLX_I2C_ADDR, MLX_REG_TOBJ1, data, 3)) {
        mlx_present = false;
        return false;
    }
    
    mlx_temp = mlx_raw_to_temp(data[0], data[1], data[2]);
    mlx_present = true;
    
    if (temp_c)
        *temp_c = mlx_temp;
    
    return true;
}

float mlx_get_temp(void)
{
    return mlx_temp;
}

bool mlx_is_present(void)
{
    return mlx_present;
}

/* ============================================================
 * DS18B20 1-Wire Temperature Sensor
 * ============================================================ */

/* 1-Wire timing macros */
#define DS18B20_PIN_LOW()   GPIO_CLR(DS18B20_PORT, DS18B20_PIN)
#define DS18B20_PIN_HIGH()  GPIO_SET(DS18B20_PORT, DS18B20_PIN)
#define DS18B20_PIN_READ()  GPIO_READ(DS18B20_PORT, DS18B20_PIN)

#define ONEWIRE_DELAY_US(us)  do { \
    for (volatile int _i = 0; _i < (us) * 48; _i++) __NOP(); \
} while(0)

static void ds18b20_set_output(void)
{
    uint32_t port = DS18B20_PORT;
    GPIO_MODER(port) &= ~(3U << (DS18B20_PIN * 2));
    GPIO_MODER(port) |= (GPIO_MODE_OUTPUT_PP << (DS18B20_PIN * 2));
}

static void ds18b20_set_input(void)
{
    uint32_t port = DS18B20_PORT;
    GPIO_MODER(port) &= ~(3U << (DS18B20_PIN * 2));
    /* Input mode with pull-up */
    GPIO_PUPDR(port) &= ~(3U << (DS18B20_PIN * 2));
    GPIO_PUPDR(port) |= (GPIO_PULLUP << (DS18B20_PIN * 2));
}

static bool onewire_reset(void)
{
    bool presence;
    
    ds18b20_set_output();
    DS18B20_PIN_LOW();
    ONEWIRE_DELAY_US(480);  /* Reset pulse */
    
    ds18b20_set_input();
    ONEWIRE_DELAY_US(70);   /* Wait for presence pulse */
    
    presence = (DS18B20_PIN_READ() == 0);
    
    ONEWIRE_DELAY_US(410);  /* Recovery */
    
    return presence;
}

static void onewire_write_bit(uint8_t bit)
{
    ds18b20_set_output();
    DS18B20_PIN_LOW();
    
    if (bit) {
        ONEWIRE_DELAY_US(6);
        ds18b20_set_input();
        ONEWIRE_DELAY_US(64);
    } else {
        ONEWIRE_DELAY_US(60);
        ds18b20_set_input();
        ONEWIRE_DELAY_US(10);
    }
}

static uint8_t onewire_read_bit(void)
{
    uint8_t bit;
    
    ds18b20_set_output();
    DS18B20_PIN_LOW();
    ONEWIRE_DELAY_US(4);
    
    ds18b20_set_input();
    ONEWIRE_DELAY_US(10);
    
    bit = DS18B20_PIN_READ();
    
    ONEWIRE_DELAY_US(50);
    return bit;
}

static void onewire_write(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(data & 1);
        data >>= 1;
    }
}

static uint8_t onewire_read(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data >>= 1;
        if (onewire_read_bit())
            data |= 0x80;
    }
    return data;
}

bool ds18b20_read_temp(float *temp_c)
{
    uint8_t lsb, msb;
    int16_t raw;
    
    if (!onewire_reset()) {
        ds18b20_present = false;
        return false;
    }
    
    /* Skip ROM (single device on bus) */
    onewire_write(0xCC);
    /* Convert T */
    onewire_write(0x44);
    
    /* Wait for conversion (750ms for 12-bit) */
    ONEWIRE_DELAY_US(750000);
    
    if (!onewire_reset()) {
        ds18b20_present = false;
        return false;
    }
    
    onewire_write(0xCC);
    /* Read scratchpad */
    onewire_write(0xBE);
    
    lsb = onewire_read();
    msb = onewire_read();
    
    raw = (int16_t)((msb << 8) | lsb);
    /* DS18B20 12-bit: temp = raw / 16.0 */
    ds18b20_temp = raw / 16.0f;
    ds18b20_present = true;
    
    if (temp_c)
        *temp_c = ds18b20_temp;
    
    return true;
}

float ds18b20_get_temp(void)
{
    return ds18b20_temp;
}

bool ds18b20_is_present(void)
{
    return ds18b20_present;
}

/* ============================================================
 * Combined temperature reading
 * ============================================================ */

void temp_read_all(temp_readings_t *readings)
{
    if (readings == NULL)
        return;
    
    readings->ir_temp = mlx_temp;
    readings->plate_temp = ds18b20_temp;
    readings->internal = mcu_temp;
}

/* ============================================================
 * Initialization
 * ============================================================ */

void temp_sensor_init(void)
{
    i2c_init();
    
    /* Configure DS18B20 pin with pull-up */
    uint32_t port = DS18B20_PORT;
    GPIO_MODER(port) &= ~(3U << (DS18B20_PIN * 2));
    GPIO_PUPDR(port) &= ~(3U << (DS18B20_PIN * 2));
    GPIO_PUPDR(port) |= (GPIO_PULLUP << (DS18B20_PIN * 2));
    
    /* Probe for sensors */
    mlx_present = mlx_read_temp(NULL);
    ds18b20_present = onewire_reset();
    
    /* MCU internal temp sensor calibration would go here */
    mcu_temp = 25.0f;
}