/*
 * TRACE-REAPER — tamper detection driver
 *
 * Monitors an accelerometer (LIS2DH12 over I2C1) for shock/motion events and
 * a conductive case-seam mesh (GPIO) for case opening. On any tamper event
 * the firmware zeroizes all secrets: trace buffer, session storage, in-RAM
 * correlation tables, and the BLE link keys.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "tamper.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* LIS2DH12 I2C address (SA0=0) */
#define LIS_ADDR       0x18
#define LIS_WHOAMI      0x0F  /* should read 0x33 */
#define LIS_CTRL1       0x20  /* ODR, axes enable */
#define LIS_CTRL3       0x22  /* int1 cfg */
#define LIS_INT1_CFG    0x30  /* interrupt config */
#define LIS_INT1_THS    0x32  /* threshold */
#define LIS_INT1_DUR    0x33  /* duration */

static volatile int g_tampered = 0;
static tamper_cb_t g_cb = 0;

/* I2C1 single-byte write */
static void i2c_write(uint8_t addr, uint8_t reg, uint8_t val)
{
    /* Software implementation: START, addrW, reg, val, STOP.
     * Uses I2C1 CR2/ISR. Kept compact for the reference build.
     */
    I2C1->CR2 = ((uint32_t)addr << 1) | (2U << 16) | (1U << 13); /* 2 bytes, START */
    while ((I2C1->ISR & (1U << 0)) == 0) ; /* TXIS */
    I2C1->TXDR = reg;
    while ((I2C1->ISR & (1U << 0)) == 0) ;
    I2C1->TXDR = val;
    while ((I2C1->ISR & (1U << 5)) == 0) ; /* STOPF */
    I2C1->ICR = (1U << 5);
}

static uint8_t i2c_read(uint8_t addr, uint8_t reg)
{
    I2C1->CR2 = ((uint32_t)addr << 1) | (1U << 16) | (1U << 13);
    while ((I2C1->ISR & (1U << 0)) == 0) ;
    I2C1->TXDR = reg;
    while ((I2C1->ISR & (1U << 5)) == 0) ;
    I2C1->ICR = (1U << 5);
    I2C1->CR2 = ((uint32_t)addr << 1) | (1U << 16) | (1U << 1) | (1U << 13); /* READ, 1 byte, START, AUTOEND */
    while ((I2C1->ISR & (1U << 2)) == 0) ; /* RXNE */
    return (uint8_t)I2C1->RXDR;
}

void tamper_init(tamper_cb_t cb)
{
    g_cb = cb;
    /* Configure case-mesh pin as input with pull-up; mesh pulls it low if intact.
     * If the case is opened the mesh breaks and the pin reads high -> tamper.
     */
    gpio_mode(GPIOC, 4, 0);
    gpio_pupd(GPIOC, 4, 1); /* pull-up: intact = low, opened = high */

    /* Configure accelerometer interrupt pins as inputs */
    gpio_mode(GPIOC, 2, 0);
    gpio_pupd(GPIOC, 2, 2);

    /* Configure LIS2DH12: enable XYZ at 100 Hz, interrupt on motion */
    i2c_write(LIS_ADDR, LIS_CTRL1, 0x57);  /* 100 Hz, all axes */
    i2c_write(LIS_ADDR, LIS_CTRL3, 0x20);  /* int1 = activity */
    i2c_write(LIS_ADDR, LIS_INT1_CFG, 0x2A); /* OR of XYZ high events */
    i2c_write(LIS_ADDR, LIS_INT1_THS, 0x10); /* threshold */
    i2c_write(LIS_ADDR, LIS_INT1_DUR, 0x01); /* 1 LSB duration */

    /* Enable EXTI on the INT1 and mesh pins */
    EXTI->IMR1  |= (1U << 2) | (1U << 4);
    EXTI->RTSR1 |= (1U << 2) | (1U << 4);
    nvic_enable(EXTI9_5_IRQn, 3);  /* INT1 on PC2 -> EXTI2 */
    nvic_enable(EXTI15_10_IRQn, 1);
}

static void tamper_fire(void)
{
    g_tampered = 1;
    if (g_cb) g_cb();
}

/* EXTI handler for accelerometer INT1 (line 2) and mesh (line 4) */
void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR1 & (1U << 2)) {  /* accel INT1 */
        EXTI->PR1 = (1U << 2);
        tamper_fire();
    }
}

int tamper_check(void)
{
    /* Active poll: mesh pin should be low when intact. */
    if (gpio_in(GPIOC, 4)) {
        tamper_fire();
        return 1;
    }
    return g_tampered;
}

int tamper_is_triggered(void) { return g_tampered; }

void tamper_clear(void) { g_tampered = 0; }

/* ---- Zeroize helper ----
 * Wipes a memory region using a volatile write loop (not memset, to avoid
 * the compiler optimizing it away on sensitive data).
 */
void tamper_zeroize(void *p, uint32_t n)
{
    volatile uint8_t *q = (volatile uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) q[i] = 0;
    /* Memory barrier */
    __asm volatile ("dmb" ::: "memory");
}