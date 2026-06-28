/*
 * registers.h - STM32H7 register definitions and low-level macros
 * for Thermal Phantom
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * STM32H7 base address definitions (subset, for bare-metal)
 * ============================================================ */

#define PERIPH_BASE           (0x40000000UL)
#define PERIPH_APB1_BASE      (PERIPH_BASE + 0x00000)
#define PERIPH_APB2_BASE      (PERIPH_BASE + 0x10000)
#define PERIPH_AHB1_BASE      (PERIPH_BASE + 0x20000)
#define PERIPH_AHB2_BASE      (PERIPH_BASE + 0x08000000UL)
#define PERIPH_AHB3_BASE      (PERIPH_BASE + 0x10000000UL)
#define PERIPH_AHB4_BASE      (PERIPH_BASE + 0x18000000UL)

/* --- GPIO ports --- */
#define GPIOA_BASE            (PERIPH_AHB4_BASE + 0x0000)
#define GPIOB_BASE            (PERIPH_AHB4_BASE + 0x0400)
#define GPIOC_BASE            (PERIPH_AHB4_BASE + 0x0800)
#define GPIOD_BASE            (PERIPH_AHB4_BASE + 0x0C00)
#define GPIOE_BASE            (PERIPH_AHB4_BASE + 0x1000)
#define GPIOH_BASE            (PERIPH_AHB4_BASE + 0x1C00)

/* --- RCC --- */
#define RCC_BASE              (PERIPH_AHB1_BASE + 0x4400)

/* --- Timers --- */
#define TIM1_BASE             (PERIPH_APB2_BASE + 0x0000)
#define TIM2_BASE             (PERIPH_APB1_BASE + 0x4000)
#define TIM3_BASE             (PERIPH_APB1_BASE + 0x4000)
#define TIM6_BASE             (PERIPH_APB1_BASE + 0x1000)
#define TIM7_BASE             (PERIPH_APB1_BASE + 0x1400)

/* --- USART/UART --- */
#define USART3_BASE           (PERIPH_APB1_BASE + 0x4800)
#define UART4_BASE            (PERIPH_APB1_BASE + 0x4C00)

/* --- I2C --- */
#define I2C1_BASE             (PERIPH_APB1_BASE + 0x5400)

/* --- ADC --- */
#define ADC1_BASE             (PERIPH_AHB1_BASE + 0x0000)

/* --- DMA --- */
#define DMA1_BASE             (PERIPH_AHB1_BASE + 0x2000)

/* --- EXTI --- */
#define EXTI_BASE             (PERIPH_APB2_BASE + 0xF000)
#define SYSCFG_BASE            (PERIPH_APB2_BASE + 0x0400)

/* --- COMP --- */
#define COMP_BASE             (PERIPH_AHB2_BASE + 0x0000)

/* --- CRC --- */
#define CRC_BASE              (PERIPH_AHB1_BASE + 0x4000)

/* ============================================================
 * Register access macros
 * ============================================================ */

#define REG32(addr)           (*(volatile uint32_t *)(addr))
#define REG16(addr)           (*(volatile uint16_t *)(addr))
#define REG8(addr)            (*(volatile uint8_t *)(addr))

#define SET_BIT(reg, bit)     ((reg) |= (bit))
#define CLR_BIT(reg, bit)     ((reg) &= ~(bit))
#define TOGGLE_BIT(reg, bit)  ((reg) ^= (bit))
#define READ_BIT(reg, bit)    ((reg) & (bit))

/* GPIO register offsets */
#define GPIO_MODER_OFFSET     0x00
#define GPIO_OTYPER_OFFSET    0x04
#define GPIO_OSPEEDR_OFFSET   0x08
#define GPIO_PUPDR_OFFSET     0x0C
#define GPIO_IDR_OFFSET       0x10
#define GPIO_ODR_OFFSET       0x14
#define GPIO_BSRR_OFFSET      0x18
#define GPIO_LCKR_OFFSET      0x1C
#define GPIO_AFRL_OFFSET      0x20
#define GPIO_AFRH_OFFSET      0x24

/* GPIO helper macros */
#define GPIO_MODER(port)      REG32((port) + GPIO_MODER_OFFSET)
#define GPIO_OTYPER(port)     REG32((port) + GPIO_OTYPER_OFFSET)
#define GPIO_OSPEEDR(port)    REG32((port) + GPIO_OSPEEDR_OFFSET)
#define GPIO_PUPDR(port)      REG32((port) + GPIO_PUPDR_OFFSET)
#define GPIO_IDR(port)        REG32((port) + GPIO_IDR_OFFSET)
#define GPIO_ODR(port)        REG32((port) + GPIO_ODR_OFFSET)
#define GPIO_BSRR(port)       REG32((port) + GPIO_BSRR_OFFSET)

#define GPIO_SET(port, pin)   (GPIO_BSRR(port) = (1U << (pin)))
#define GPIO_CLR(port, pin)   (GPIO_BSRR(port) = (1U << ((pin) + 16)))
#define GPIO_READ(port, pin)  ((GPIO_IDR(port) >> (pin)) & 1U)
#define GPIO_WRITE(port, pin, val)  do { \
    if (val) GPIO_SET(port, pin); \
    else GPIO_CLR(port, pin); \
} while(0)

/* GPIO mode definitions */
#define GPIO_MODE_INPUT       0x00
#define GPIO_MODE_OUTPUT_PP    0x01
#define GPIO_MODE_OUTPUT_OD    0x02
#define GPIO_MODE_AF           0x02
#define GPIO_MODE_ANALOG       0x03

#define GPIO_SPEED_LOW         0x00
#define GPIO_SPEED_MED         0x01
#define GPIO_SPEED_HIGH        0x02
#define GPIO_SPEED_VHIGH       0x03

#define GPIO_NOPULL            0x00
#define GPIO_PULLUP            0x01
#define GPIO_PULLDOWN          0x02

/* ============================================================
 * Timer register offsets
 * ============================================================ */
#define TIM_CR1_OFFSET         0x00
#define TIM_CR2_OFFSET         0x04
#define TIM_SMCR_OFFSET        0x08
#define TIM_DIER_OFFSET        0x0C
#define TIM_SR_OFFSET          0x10
#define TIM_EGR_OFFSET         0x14
#define TIM_CCMR1_OFFSET       0x18
#define TIM_CCMR2_OFFSET       0x1C
#define TIM_CCER_OFFSET        0x20
#define TIM_CNT_OFFSET         0x24
#define TIM_PSC_OFFSET         0x28
#define TIM_ARR_OFFSET         0x2C
#define TIM_CCR1_OFFSET        0x34
#define TIM_CCR2_OFFSET        0x38
#define TIM_CCR3_OFFSET        0x3C
#define TIM_CCR4_OFFSET        0x40

#define TIM_CR1(port)          REG32((port) + TIM_CR1_OFFSET)
#define TIM_CR2(port)          REG32((port) + TIM_CR2_OFFSET)
#define TIM_DIER(port)         REG32((port) + TIM_DIER_OFFSET)
#define TIM_SR(port)           REG32((port) + TIM_SR_OFFSET)
#define TIM_EGR(port)         REG32((port) + TIM_EGR_OFFSET)
#define TIM_CCMR1(port)        REG32((port) + TIM_CCMR1_OFFSET)
#define TIM_CCMR2(port)        REG32((port) + TIM_CCMR2_OFFSET)
#define TIM_CCER(port)         REG32((port) + TIM_CCER_OFFSET)
#define TIM_CNT(port)          REG32((port) + TIM_CNT_OFFSET)
#define TIM_PSC(port)          REG32((port) + TIM_PSC_OFFSET)
#define TIM_ARR(port)          REG32((port) + TIM_ARR_OFFSET)
#define TIM_CCR1(port)         REG32((port) + TIM_CCR1_OFFSET)
#define TIM_CCR2(port)         REG32((port) + TIM_CCR2_OFFSET)

/* Timer bit definitions */
#define TIM_CR1_CEN            (1U << 0)
#define TIM_CR1_ARPE           (1U << 7)
#define TIM_DIER_UIE           (1U << 0)
#define TIM_DIER_CC1IE         (1U << 1)
#define TIM_SR_UIF             (1U << 0)
#define TIM_SR_CC1IF           (1U << 1)
#define TIM_CCER_CC1E          (1U << 0)
#define TIM_CCER_CC1P          (1U << 1)

/* PWM mode 1: OC1M = 110 */
#define TIM_CCMR1_OC1M_PWM1    (6U << 4)
#define TIM_CCMR1_OC1PE        (1U << 3)

/* ============================================================
 * USART register offsets
 * ============================================================ */
#define USART_CR1_OFFSET       0x00
#define USART_CR2_OFFSET       0x04
#define USART_CR3_OFFSET       0x08
#define USART_BRR_OFFSET       0x0C
#define USART_RDR_OFFSET       0x10
#define USART_TDR_OFFSET       0x14
#define USART_ISR_OFFSET       0x1C
#define USART_ICR_OFFSET       0x20

#define USART_CR1(port)        REG32((port) + USART_CR1_OFFSET)
#define USART_CR2(port)        REG32((port) + USART_CR2_OFFSET)
#define USART_CR3(port)        REG32((port) + USART_CR3_OFFSET)
#define USART_BRR(port)        REG32((port) + USART_BRR_OFFSET)
#define USART_RDR(port)        REG32((port) + USART_RDR_OFFSET)
#define USART_TDR(port)        REG32((port) + USART_TDR_OFFSET)
#define USART_ISR(port)        REG32((port) + USART_ISR_OFFSET)
#define USART_ICR(port)        REG32((port) + USART_ICR_OFFSET)

#define USART_CR1_UE           (1U << 0)
#define USART_CR1_RE           (1U << 2)
#define USART_CR1_TE           (1U << 3)
#define USART_CR1_RXNEIE        (1U << 5)
#define USART_CR1_TCIE         (1U << 6)
#define USART_ISR_RXNE         (1U << 5)
#define USART_ISR_TXE          (1U << 7)
#define USART_ISR_TC           (1U << 6)

/* ============================================================
 * I2C register offsets (STM32H7 I2C peripheral)
 * ============================================================ */
#define I2C_CR1_OFFSET         0x00
#define I2C_CR2_OFFSET         0x04
#define I2C_OAR1_OFFSET        0x08
#define I2C_ISR_OFFSET         0x10
#define I2C_ICR_OFFSET         0x14
#define I2C_PECR_OFFSET        0x18
#define I2C_RXDR_OFFSET        0x24
#define I2C_TXDR_OFFSET        0x28
#define I2C_TIMINGR_OFFSET     0x10

#define I2C_CR1(port)          REG32((port) + I2C_CR1_OFFSET)
#define I2C_CR2(port)          REG32((port) + I2C_CR2_OFFSET)
#define I2C_ISR(port)          REG32((port) + I2C_ISR_OFFSET)
#define I2C_ICR(port)          REG32((port) + I2C_ICR_OFFSET)
#define I2C_RXDR(port)         REG32((port) + I2C_RXDR_OFFSET)
#define I2C_TXDR(port)         REG32((port) + I2C_TXDR_OFFSET)
#define I2C_TIMINGR(port)      REG32((port) + I2C_TIMINGR_OFFSET)

#define I2C_CR1_PE             (1U << 0)
#define I2C_CR2_START          (1U << 13)
#define I2C_CR2_STOP           (1U << 14)
#define I2C_CR2_NBYTES_SHIFT   16
#define I2C_ISR_TXIS           (1U << 1)
#define I2C_ISR_RXNE           (1U << 2)
#define I2C_ISR_NACKF          (1U << 4)
#define I2C_ISR_STOPF          (1U << 5)
#define I2C_ISR_TC             (1U << 6)
#define I2C_ISR_TCR            (1U << 7)
#define I2C_ISR_BUSY           (1U << 15)

/* ============================================================
 * Flash (for profile storage in user data area)
 * ============================================================ */
#define FLASH_USER_DATA_ADDR   0x08100000UL  /* Bank 2 sector 0 */
#define FLASH_PROFILE_MAGIC    0xDEADBEEF

/* ============================================================
 * CRC-16 (CCITT) for communication packets
 * ============================================================ */
#define CRC16_POLY             0x1021
#define CRC16_INIT             0xFFFF

static inline uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = CRC16_INIT;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC16_POLY;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ============================================================
 * Utility macros
 * ============================================================ */
#define ARRAY_SIZE(x)          (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b)              ((a) < (b) ? (a) : (b))
#define MAX(a, b)              ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)       (MIN(MAX((x), (lo)), (hi)))
#define UNUSED(x)              ((void)(x))

#define FIRMWARE_VERSION       "1.0.0-jayis1"
#define AUTHOR_NAME            "jayis1"

/* Memory barrier */
#define __DSB()                __asm volatile ("dsb" ::: "memory")
#define __ISB()                __asm volatile ("isb" ::: "memory")
#define __NOP()                __asm volatile ("nop")
#define __WFI()                __asm volatile ("wfi")

/* NVIC interrupt enable/disable */
#define NVIC_EnableIRQ(n)     do { REG32(0xE000E100 + ((n) >> 5) * 4) = (1 << ((n) & 31)); } while(0)
#define NVIC_DisableIRQ(n)    do { REG32(0xE000E180 + ((n) >> 5) * 4) = (1 << ((n) & 31)); } while(0)

#endif /* REGISTERS_H */