/*
 * registers.h — minimal STM32G474 register definitions for DRAM-Phantom
 *
 * Bare-metal, no HAL. Only the registers we touch are defined.
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef DRAMPHANTOM_REGISTERS_H
#define DRAMPHANTOM_REGISTERS_H

#include <stdint.h>

#define PERIPH_BASE    0x40000000UL
#define PERIPH_AHB_BASE 0x48000000UL

/* RCC */
#define RCC_BASE        (PERIPH_BASE + 0x1000)
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x48))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR2    (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_AHB2RSTR    (*(volatile uint32_t *)(RCC_BASE + 0x30))

#define RCC_AHB1ENR_DMA1    (1u<<0)
#define RCC_AHB1ENR_DMAMUX1  (1u<<2)
#define RCC_AHB2ENR_GPIOA   (1u<<0)
#define RCC_AHB2ENR_GPIOB   (1u<<1)
#define RCC_AHB2ENR_GPIOC   (1u<<2)
#define RCC_AHB2ENR_ADC12   (1u<<13)
#define RCC_APB1ENR1_I2C1   (1u<<21)
#define RCC_APB1ENR1_USART1 (1u<<14)  /* USART1 on APB1ENR1 on G4 */
#define RCC_APB1ENR2_USB    (1u<<23)
#define RCC_APB2ENR_SPI1     (1u<<12)
#define RCC_APB2ENR_SYSCFG   (1u<<0)

/* PWR */
#define PWR_BASE        (PERIPH_BASE + 0x7000)
#define PWR_CR1         (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR3         (*(volatile uint32_t *)(PWR_BASE + 0x08))

/* FLASH */
#define FLASH_BASE      (PERIPH_BASE + 0x2000)
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00))

/* GPIO registers (per port) */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;  /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;      /* 0x10 */
    volatile uint32_t ODR;      /* 0x14 */
    volatile uint32_t BSRR;     /* 0x18 */
    volatile uint32_t LCKR;     /* 0x1C */
    volatile uint32_t AFRL;     /* 0x20 */
    volatile uint32_t AFRH;     /* 0x24 */
    volatile uint32_t BRR;      /* 0x28 */
} gpio_t;

#define GPIOA           ((gpio_t *)(PERIPH_BASE + 0x0000 + 0x48000000 - 0x48000000 + 0x20000000))
#define GPIOB           ((gpio_t *)(0x48000400))
#define GPIOC           ((gpio_t *)(0x48000800))

/* The above trickery is fragile; use explicit addresses: */
#undef GPIOA
#undef GPIOB
#undef GPIOC
#define GPIOA           ((gpio_t *)0x48000000UL)
#define GPIOB           ((gpio_t *)0x48000400UL)
#define GPIOC           ((gpio_t *)0x48000800UL)

#define GPIO_MODE_IN     0x0
#define GPIO_MODE_OUT    0x1
#define GPIO_MODE_AF     0x2
#define GPIO_MODE_ANALOG 0x3
#define GPIO_OTYPE_PP    0x0
#define GPIO_OTYPE_OD    0x1
#define GPIO_OSPEED_LOW  0x0
#define GPIO_OSPEED_HIGH 0x2
#define GPIO_PUPD_NONE   0x0
#define GPIO_PUPD_UP     0x1
#define GPIO_PUPD_DOWN   0x2

/* SPI1 */
#define SPI1_BASE        (PERIPH_BASE + 0x4000)
#define SPI1_CR1         (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_CR2         (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI1_SR          (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR          (*(volatile uint32_t *)(SPI1_BASE + 0x0C))

#define SPI_CR1_MSTR     (1u<<2)
#define SPI_CR1_BR_DIV64 (5u<<3)
#define SPI_CR1_SPE      (1u<<6)
#define SPI_CR1_CPHA     (1u<<0)
#define SPI_CR1_CPOL     (1u<<1)
#define SPI_CR1_SSM      (1u<<9)
#define SPI_CR1_SSI      (1u<<8)
#define SPI_CR2_DS_8BIT  (7u<<8)
#define SPI_CR2_FRXTH    (1u<<12)
#define SPI_SR_RXNE      (1u<<0)
#define SPI_SR_TXE       (1u<<1)
#define SPI_SR_BSY       (1u<<7)

/* I2C1 */
#define I2C1_BASE        (PERIPH_BASE + 0x5400)
#define I2C1_CR1         (*(volatile uint32_t *)(I2C1_BASE + 0x00))
#define I2C1_CR2         (*(volatile uint32_t *)(I2C1_BASE + 0x04))
#define I2C1_ISR         (*(volatile uint32_t *)(I2C1_BASE + 0x10))
#define I2C1_TXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x28))
#define I2C1_RXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x24))
#define I2C1_TIMINGR     (*(volatile uint32_t *)(I2C1_BASE + 0x0C))
#define I2C1_OAR1        (*(volatile uint32_t *)(I2C1_BASE + 0x08))

#define I2C_CR1_PE       (1u<<0)
#define I2C_CR2_START    (1u<<13)
#define I2C_CR2_STOP     (1u<<14)
#define I2C_CR2_NBYTES_S (8u)
#define I2C_ISR_TXE      (1u<<0)
#define I2C_ISR_RXNE     (1u<<2)
#define I2C_ISR_TC       (1u<<6)
#define I2C_ISR_BUSY     (1u<<15)

/* USART1 */
#define USART1_BASE      (PERIPH_BASE + 0x4C00)
#define USART1_CR1        (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_CR2        (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR        (*(volatile uint32_t *)(USART1_BASE + 0x0C))
#define USART1_ISR        (*(volatile uint32_t *)(USART1_BASE + 0x1C))
#define USART1_TDR        (*(volatile uint32_t *)(USART1_BASE + 0x28))
#define USART1_RDR        (*(volatile uint32_t *)(USART1_BASE + 0x24))

#define USART_CR1_UE     (1u<<0)
#define USART_CR1_TE     (1u<<3)
#define USART_CR1_RE     (1u<<2)
#define USART_ISR_TXE    (1u<<7)
#define USART_ISR_RXNE   (1u<<5)

/* USB (full-speed) */
#define USB_BASE          (PERIPH_BASE + 0x5C00)
#define USB_CNTR          (*(volatile uint32_t *)(USB_BASE + 0x40))
#define USB_ISTR          (*(volatile uint32_t *)(USB_BASE + 0x44))
#define USB_DADDR         (*(volatile uint32_t *)(USB_BASE + 0x4C))
#define USB_BTABLE        (*(volatile uint32_t *)(USB_BASE + 0x50))

/* SDMMC (basic presence detect only for now) */
#define SDMMC1_BASE       (PERIPH_BASE + 0x8000)

/* SysTick */
#define SYSTICK_BASE      (0xE000E010UL)
#define SYST_CSR          (*(volatile uint32_t *)(SYST_CSR_BASE + 0x00))
#define SYSTICK_CSR        (*(volatile uint32_t *)(0xE000E010UL))
#define SYSTICK_LOAD       (*(volatile uint32_t *)(0xE000E014UL))
#define SYSTICK_VAL        (*(volatile uint32_t *)(0xE000E018UL))
#define SYST_CSR_ENABLE    (1u<<0)
#define SYST_CSR_CLKSRC    (1u<<2)
#define SYST_CSR_TICKINT   (1u<<1)

/* NVIC */
#define NVIC_ISER0         (*(volatile uint32_t *)(0xE000E100UL))
#define NVIC_ICPR0         (*(volatile uint32_t *)(0xE000E280UL))

/* EXTI */
#define EXTI_BASE          0x40010400UL
#define EXTI_IMR1          (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_RTSR1         (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_FTSR1         (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_PR1           (*(volatile uint32_t *)(EXTI_BASE + 0x14))

#endif /* DRAMPHANTOM_REGISTERS_H */