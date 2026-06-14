/*
 * i2c_eeprom.c — AT24C02 I2C EEPROM driver for STM32F407
 * I2C1: SCL=PA15, SDA=PB6
 */

#include "i2c_eeprom.h"
#include "registers.h"
#include "board.h"

/* I2C1 register base */
#define I2C1            ((I2C_TypeDef *)I2C1_BASE)

/* I2C CR1 bits */
#define I2C_CR1_PE      (1 << 0)
#define I2C_CR1_START   (1 << 8)
#define I2C_CR1_STOP    (1 << 9)
#define I2C_CR1_ACK     (1 << 10)
#define I2C_CR1_SWRST   (1 << 15)

/* I2C SR1 bits */
#define I2C_SR1_SB      (1 << 0)
#define I2C_SR1_ADDR     (1 << 1)
#define I2C_SR1_BTF      (1 << 2)
#define I2C_SR1_TXE      (1 << 7)
#define I2C_SR1_RXNE     (1 << 6)

int i2c_eeprom_init(void) {
    /* Enable I2C1 clock */
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* Reset I2C1 */
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* Configure I2C1: 400 kHz fast mode
     * APB1 = 42 MHz, CCR = APB1 / (2 * 400000) = 52.5 → 53
     */
    I2C1->CR2 = 42;  /* FREQ = 42 MHz */
    I2C1->CCR = 53 | (1 << 15);  /* Fast mode, CCR = 53 */
    I2C1->TRISE = 43;  /* (42 MHz / 1 MHz) + 1 */

    /* Enable I2C1 */
    I2C1->CR1 |= I2C_CR1_PE | I2C_CR1_ACK;

    return 0;
}

static int i2c_wait_flag(uint32_t flag, uint32_t timeout) {
    while (!(I2C1->SR1 & flag)) {
        if (--timeout == 0) return -1;
    }
    return 0;
}

int i2c_eeprom_write(uint8_t dev_addr, uint16_t mem_addr, const uint8_t *data, uint16_t len) {
    uint32_t timeout = 100000;

    /* Send START */
    I2C1->CR1 |= I2C_CR1_START;
    if (i2c_wait_flag(I2C_SR1_SB, timeout)) return -1;

    /* Send device address (write) */
    I2C1->DR = (dev_addr << 1);
    if (i2c_wait_flag(I2C_SR1_ADDR, timeout)) return -1;
    (void)I2C1->SR2;  /* Clear ADDR flag */

    /* Send memory address */
    I2C1->DR = (uint8_t)(mem_addr & 0xFF);
    if (i2c_wait_flag(I2C_SR1_TXE, timeout)) return -1;

    /* Write data (respect page boundaries) */
    uint16_t written = 0;
    while (written < len) {
        /* Calculate remaining bytes in current page */
        uint16_t page_offset = (mem_addr + written) % EEPROM_PAGE_SIZE;
        uint16_t page_remaining = EEPROM_PAGE_SIZE - page_offset;
        uint16_t chunk = (len - written) < page_remaining ? (len - written) : page_remaining;

        for (uint16_t i = 0; i < chunk; i++) {
            if (i2c_wait_flag(I2C_SR1_TXE, timeout)) return -1;
            I2C1->DR = data[written + i];
        }

        written += chunk;

        /* If more data, send STOP and wait for write cycle, then restart */
        if (written < len) {
            I2C1->CR1 |= I2C_CR1_STOP;
            /* Wait for EEPROM write cycle (~5 ms) */
            for (volatile int i = 0; i < 500000; i++);

            /* Re-start for next page */
            I2C1->CR1 |= I2C_CR1_START;
            if (i2c_wait_flag(I2C_SR1_SB, timeout)) return -1;

            I2C1->DR = (dev_addr << 1);
            if (i2c_wait_flag(I2C_SR1_ADDR, timeout)) return -1;
            (void)I2C1->SR2;

            I2C1->DR = (uint8_t)((mem_addr + written) & 0xFF);
            if (i2c_wait_flag(I2C_SR1_TXE, timeout)) return -1;
        }
    }

    /* Send STOP */
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}

int i2c_eeprom_read(uint8_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t len) {
    uint32_t timeout = 100000;

    /* Send START */
    I2C1->CR1 |= I2C_CR1_START;
    if (i2c_wait_flag(I2C_SR1_SB, timeout)) return -1;

    /* Send device address (write) */
    I2C1->DR = (dev_addr << 1);
    if (i2c_wait_flag(I2C_SR1_ADDR, timeout)) return -1;
    (void)I2C1->SR2;

    /* Send memory address */
    I2C1->DR = (uint8_t)(mem_addr & 0xFF);
    if (i2c_wait_flag(I2C_SR1_BTF, timeout)) return -1;

    /* Repeated START for read */
    I2C1->CR1 |= I2C_CR1_START;
    if (i2c_wait_flag(I2C_SR1_SB, timeout)) return -1;

    /* Send device address (read) */
    I2C1->DR = (dev_addr << 1) | 1;

    if (len == 1) {
        /* Single byte read: NACK, then read */
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if (i2c_wait_flag(I2C_SR1_ADDR, timeout)) return -1;
        (void)I2C1->SR2;
        I2C1->CR1 |= I2C_CR1_STOP;
        data[0] = I2C1->DR;
    } else {
        /* Multi-byte read: ACK all except last */
        I2C1->CR1 |= I2C_CR1_ACK;
        if (i2c_wait_flag(I2C_SR1_ADDR, timeout)) return -1;
        (void)I2C1->SR2;

        for (uint16_t i = 0; i < len - 1; i++) {
            if (i2c_wait_flag(I2C_SR1_RXNE, timeout)) return -1;
            data[i] = I2C1->DR;
        }

        /* Last byte: NACK */
        I2C1->CR1 &= ~I2C_CR1_ACK;
        I2C1->CR1 |= I2C_CR1_STOP;
        if (i2c_wait_flag(I2C_SR1_RXNE, timeout)) return -1;
        data[len - 1] = I2C1->DR;
    }

    I2C1->CR1 |= I2C_CR1_ACK;  /* Re-enable ACK for next transaction */
    return 0;
}