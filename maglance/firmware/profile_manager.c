/*
 * profile_manager.c — EEPROM profile storage implementation for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Stores and retrieves 16 attack profiles in a 24LC256 I2C EEPROM.
 * Each profile occupies 64 bytes. The EEPROM has 32 KB (32768 bytes),
 * which accommodates 16 profiles (1024 bytes) plus calibration data.
 *
 * 24LC256 I2C EEPROM specifications:
 *   - 256 Kbit (32 KB) capacity
 *   - 64-byte page write buffer
 *   - Write cycle time: ~5 ms
 *   - I2C address: 0x50 (A0-A2 tied to GND)
 *   - Addressing: 16-bit word address (MSB first)
 */

#include "profile_manager.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ---- I2C helper functions ---- */

static void i2c_wait_flag(uint32_t flag, uint32_t timeout)
{
    volatile uint32_t count = timeout;
    while ((I2C1->ISR & flag) && --count) { }
}

static void i2c_wait_flag_set(uint32_t flag, uint32_t timeout)
{
    volatile uint32_t count = timeout;
    while (!(I2C1->ISR & flag) && --count) { }
}

static int i2c_write(uint8_t dev_addr, uint16_t mem_addr,
                     const uint8_t *data, uint16_t len)
{
    /* Clear any pending flags */
    I2C1->ICR = 0x3F38;

    /* Configure transfer: write to dev_addr, 2 bytes address + data */
    /* Start with NBYTES = 2 (memory address) + data bytes */
    uint16_t total = 2 + len;
    if (total > 255) return -1;  /* Max 255 bytes per transfer */

    uint32_t cr2 = 0;
    cr2 |= (dev_addr & I2C_CR2_SADD_MASK);
    cr2 |= ((uint32_t)total << I2C_CR2_NBYTES_SHIFT);
    cr2 |= I2C_CR2_AUTOEND;  /* Auto-send STOP after NBYTES */
    I2C1->CR2 = cr2 | I2C_CR2_START;

    /* Send memory address MSB */
    i2c_wait_flag_set(I2C_ISR_TXIS, 100000);
    I2C1->TXDR = (mem_addr >> 8) & 0xFF;

    /* Send memory address LSB */
    i2c_wait_flag_set(I2C_ISR_TXIS, 100000);
    I2C1->TXDR = mem_addr & 0xFF;

    /* Send data bytes */
    for (uint16_t i = 0; i < len; i++) {
        i2c_wait_flag_set(I2C_ISR_TXIS, 100000);
        I2C1->TXDR = data[i];
    }

    /* Wait for STOP */
    i2c_wait_flag_set(I2C_ISR_STOPF, 1000000);
    I2C1->ICR = I2C_ICR_STOPCF;

    /* Check for NACK */
    if (I2C1->ISR & I2C_ISR_NACKF) {
        I2C1->ICR = I2C_ICR_NACKCF;
        return -1;
    }

    return 0;
}

static int i2c_read(uint8_t dev_addr, uint16_t mem_addr,
                    uint8_t *data, uint16_t len)
{
    /* Clear flags */
    I2C1->ICR = 0x3F38;

    /* Phase 1: Write memory address (no STOP) */
    uint32_t cr2 = 0;
    cr2 |= (dev_addr & I2C_CR2_SADD_MASK);
    cr2 |= (2U << I2C_CR2_NBYTES_SHIFT);  /* 2 address bytes */
    cr2 |= I2C_CR2_RELOAD;  /* Reload mode (no auto-stop) */
    I2C1->CR2 = cr2 | I2C_CR2_START;

    /* Send address MSB */
    i2c_wait_flag_set(I2C_ISR_TXIS, 100000);
    I2C1->TXDR = (mem_addr >> 8) & 0xFF;

    /* Send address LSB */
    i2c_wait_flag_set(I2C_ISR_TXIS, 100000);
    I2C1->TXDR = mem_addr & 0xFF;

    /* Wait for TCR (transfer complete, reload) */
    i2c_wait_flag_set(I2C_ISR_TCR, 100000);

    /* Phase 2: Read data (with auto-stop) */
    cr2 = 0;
    cr2 |= (dev_addr & I2C_CR2_SADD_MASK);
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_SHIFT);
    cr2 |= I2C_CR2_AUTOEND;
    cr2 |= I2C_CR2_RD_WRN;  /* Read mode */
    I2C1->CR2 = cr2 | I2C_CR2_START;

    /* Read data bytes */
    for (uint16_t i = 0; i < len; i++) {
        i2c_wait_flag_set(I2C_ISR_RXNE, 1000000);
        data[i] = (uint8_t)I2C1->RXDR;
    }

    /* Wait for STOP */
    i2c_wait_flag_set(I2C_ISR_STOPF, 1000000);
    I2C1->ICR = I2C_ICR_STOPCF;

    return 0;
}

/* ---- I2C1 initialization ---- */

static void i2c1_init(void)
{
    /* Enable I2C1 clock */
    SET_BIT(RCC->APB1ENR1, RCC_APB1ENR1_I2C1EN);

    /* Configure PB8 (SCL) and PB9 (SDA) as AF4 (I2C1) */
    for (int pin = 8; pin <= 9; pin++) {
        uint32_t moder = I2C1_PORT->MODER;
        moder &= ~(0x03U << (pin * 2));
        moder |= (GPIO_MODE_AF << (pin * 2));
        I2C1_PORT->MODER = moder;

        /* AF4 for I2C1 on PB8/PB9 */
        uint32_t afrh = I2C1_PORT->AFRH;
        afrh &= ~(0x0FU << ((pin - 8) * 4));
        afrh |= (0x04U << ((pin - 8) * 4));
        I2C1_PORT->AFRH = afrh;

        /* Open-drain output */
        I2C1_PORT->OTYPER |= (1U << pin);

        /* High speed */
        I2C1_PORT->OSPEEDR |= (GPIO_SPEED_VHIGH << (pin * 2));

        /* Pull-up (I2C requires external pull-ups, but enable internal) */
        uint32_t pupdr = I2C1_PORT->PUPDR;
        pupdr &= ~(0x03U << (pin * 2));
        pupdr |= (GPIO_PUPD_PU << (pin * 2));
        I2C1_PORT->PUPDR = pupdr;
    }

    /* Disable I2C before configuration */
    CLEAR_BIT(I2C1->CR1, I2C_CR1_PE);

    /* Configure timing for 400 kHz fast mode:
     * PCLK1 = 170 MHz
     * PRESC = 0, SCLL = 0x0F, SCLH = 0x0F, SDADEL = 0x02, SCLDEL = 0x04
     * (Simplified — exact values depend on STM32G474 I2C timing calculator)
     */
    I2C1->TIMINGR = 0x00B01A2C;  /* 400 kHz at 170 MHz PCLK */

    /* Enable I2C */
    SET_BIT(I2C1->CR1, I2C_CR1_PE);

    /* Small delay */
    for (volatile int i = 0; i < 1000; i++) { }
}

/* ---- EEPROM page write delay ---- */

static void eeprom_wait_write_complete(void)
{
    /*
     * After a write, the EEPROM enters a write cycle (~5 ms).
     * During this time, it will NACK any I2C address.
     * We poll by attempting a dummy write until it ACKs.
     */
    for (int attempt = 0; attempt < 100; attempt++) {
        I2C1->ICR = 0x3F38;

        uint32_t cr2 = 0;
        cr2 |= (EEPROM24LC256_ADDR & I2C_CR2_SADD_MASK);
        cr2 |= (0U << I2C_CR2_NBYTES_SHIFT);  /* 0 bytes */
        cr2 |= I2C_CR2_AUTOEND;
        I2C1->CR2 = cr2 | I2C_CR2_START;

        i2c_wait_flag_set(I2C_ISR_STOPF, 100000);
        I2C1->ICR = I2C_ICR_STOPCF;

        if (!(I2C1->ISR & I2C_ISR_NACKF)) {
            return;  /* ACK received — write complete */
        }
        I2C1->ICR = I2C_ICR_NACKCF;

        /* Small delay between polls */
        for (volatile int i = 0; i < 5000; i++) { }
    }
}

/* ---- Public API ---- */

int profile_manager_init(void)
{
    i2c1_init();

    /* Verify EEPROM is present by reading byte 0 */
    uint8_t test;
    if (i2c_read(EEPROM24LC256_ADDR, 0x0000, &test, 1) != 0) {
        return -1;
    }

    return 0;
}

uint8_t profile_manager_checksum(const profile_t *profile)
{
    /* XOR checksum over first 55 bytes (excluding the checksum byte itself) */
    const uint8_t *p = (const uint8_t *)profile;
    uint8_t cs = 0;
    for (int i = 0; i < 55; i++) {
        cs ^= p[i];
    }
    return cs;
}

int profile_manager_validate(const profile_t *profile)
{
    if (profile->mode >= MODE_COUNT) return -1;
    if (profile->polarity > POLARITY_SOUTH) return -1;
    if (profile_manager_checksum(profile) != profile->checksum) return -1;
    return 0;
}

int profile_manager_save(uint8_t slot, const profile_t *profile)
{
    if (slot >= EEPROM_NUM_PROFILES) return -1;

    /* Calculate checksum */
    profile_t p = *profile;
    p.checksum = profile_manager_checksum(&p);

    /* Write to EEPROM (64 bytes fits in one page) */
    uint16_t addr = EEPROM_PROFILE_BASE + (slot * EEPROM_PROFILE_SIZE);
    if (i2c_write(EEPROM24LC256_ADDR, addr,
                   (const uint8_t *)&p, sizeof(profile_t)) != 0) {
        return -1;
    }

    /* Wait for write to complete */
    eeprom_wait_write_complete();

    return 0;
}

int profile_manager_load(uint8_t slot, profile_t *profile)
{
    if (slot >= EEPROM_NUM_PROFILES || profile == NULL) return -1;

    uint16_t addr = EEPROM_PROFILE_BASE + (slot * EEPROM_PROFILE_SIZE);
    if (i2c_read(EEPROM24LC256_ADDR, addr,
                 (uint8_t *)profile, sizeof(profile_t)) != 0) {
        return -1;
    }

    /* Validate */
    if (profile_manager_validate(profile) != 0) {
        return -1;  /* Corrupt or empty */
    }

    return 0;
}

int profile_manager_is_occupied(uint8_t slot)
{
    profile_t p;
    return (profile_manager_load(slot, &p) == 0) ? 1 : 0;
}

int profile_manager_erase(uint8_t slot)
{
    if (slot >= EEPROM_NUM_PROFILES) return -1;

    /* Write zeros to the slot */
    profile_t empty;
    memset(&empty, 0, sizeof(empty));
    empty.mode = 0xFF;  /* Invalid mode marks as empty */
    empty.checksum = profile_manager_checksum(&empty);

    uint16_t addr = EEPROM_PROFILE_BASE + (slot * EEPROM_PROFILE_SIZE);
    if (i2c_write(EEPROM24LC256_ADDR, addr,
                   (const uint8_t *)&empty, sizeof(empty)) != 0) {
        return -1;
    }

    eeprom_wait_write_complete();
    return 0;
}

int profile_manager_erase_all(void)
{
    for (uint8_t i = 0; i < EEPROM_NUM_PROFILES; i++) {
        if (profile_manager_erase(i) != 0) return -1;
    }
    return 0;
}

int profile_manager_list(uint8_t *slots, uint8_t max_slots)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < EEPROM_NUM_PROFILES && count < max_slots; i++) {
        if (profile_manager_is_occupied(i)) {
            slots[count++] = i;
        }
    }
    return count;
}

int profile_manager_save_calibration(uint8_t tip_id,
                                      const uint8_t *cal_data,
                                      uint16_t cal_len)
{
    if (tip_id == 0 || cal_len > 256) return -1;

    /* Calibration data stored after profile area */
    uint16_t addr = EEPROM_CAL_BASE + (tip_id * 256);
    if (i2c_write(EEPROM24LC256_ADDR, addr, cal_data, cal_len) != 0) {
        return -1;
    }
    eeprom_wait_write_complete();
    return 0;
}

int profile_manager_load_calibration(uint8_t tip_id,
                                      uint8_t *cal_data,
                                      uint16_t max_len)
{
    if (tip_id == 0) return -1;

    uint16_t read_len = (max_len < 256) ? max_len : 256;
    uint16_t addr = EEPROM_CAL_BASE + (tip_id * 256);
    return i2c_read(EEPROM24LC256_ADDR, addr, cal_data, read_len);
}