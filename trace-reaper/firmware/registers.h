/*
 * TRACE-REAPER — STM32H743 register map (subset)
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef TRACE_REAPER_REGISTERS_H
#define TRACE_REAPER_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses (from STM32H743 reference manual RM0433) ---- */
#define PERIPH_BASE         0x40000000UL
#define PERIPH_APB1_BASE    (PERIPH_BASE + 0x00000000UL)
#define PERIPH_APB2_BASE    (PERIPH_BASE + 0x00010000UL)
#define PERIPH_APB3_BASE    (PERIPH_BASE + 0x00020000UL)
#define PERIPH_AHB1_BASE    (PERIPH_BASE + 0x00020000UL)
#define PERIPH_AHB2_BASE    (PERIPH_BASE + 0x08020000UL)
#define PERIPH_AHB3_BASE    (PERIPH_BASE + 0x10000000UL)
#define PERIPH_AHB4_BASE    (PERIPH_BASE + 0x18020000UL)

/* RCC */
#define RCC_BASE             (PERIPH_AHB4_BASE + 0x4400UL)
#define RCC_CR               (*(volatile uint32_t *)(RCC_BASE + 0x00UL))
#define RCC_CFGR             (*(volatile uint32_t *)(RCC_BASE + 0x10UL))
#define RCC_PLL1CFGR         (*(volatile uint32_t *)(RCC_BASE + 0x28UL))
#define RCC_AHB1ENR          (*(volatile uint32_t *)(RCC_BASE + 0xD8UL))
#define RCC_AHB2ENR          (*(volatile uint32_t *)(RCC_BASE + 0xDCUL))
#define RCC_AHB3ENR          (*(volatile uint32_t *)(RCC_BASE + 0xE0UL))
#define RCC_AHB4ENR          (*(volatile uint32_t *)(RCC_BASE + 0xE4UL))
#define RCC_APB1LENR         (*(volatile uint32_t *)(RCC_BASE + 0xE8UL))
#define RCC_APB1HENR         (*(volatile uint32_t *)(RCC_BASE + 0xECUL))
#define RCC_APB2ENR          (*(volatile uint32_t *)(RCC_BASE + 0xF0UL))

/* PWR */
#define PWR_BASE             (PERIPH_AHB1_BASE + 0x4000UL)
#define PWR_CR1             (*(volatile uint32_t *)(PWR_BASE + 0x00UL))
#define PWR_CR3             (*(volatile uint32_t *)(PWR_BASE + 0x08UL))
#define PWR_SR1              (*(volatile uint32_t *)(PWR_BASE + 0x20UL))

/* FLASH */
#define FLASH_BASE           (PERIPH_AHB3_BASE + 0x2000UL)
#define FLASH_ACR            (*(volatile uint32_t *)(FLASH_BASE + 0x00UL))
#define FLASH_KEYR          (*(volatile uint32_t *)(FLASH_BASE + 0x08UL))
#define FLASH_SR             (*(volatile uint32_t *)(FLASH_BASE + 0x10UL))
#define FLASH_CR             (*(volatile uint32_t *)(FLASH_BASE + 0x14UL))

/* SYSCFG */
#define SYSCFG_BASE          (PERIPH_APB3_BASE + 0x0000UL)
#define SYSCFG_PMCR          (*(volatile uint32_t *)(SYSCFG_BASE + 0x04UL))
#define SYSCFG_EXTICR1       (*(volatile uint32_t *)(SYSCFG_BASE + 0x08UL))

/* GPIO ports */
#define GPIOA_BASE           (PERIPH_AHB4_BASE + 0x0000UL)
#define GPIOB_BASE           (PERIPH_AHB4_BASE + 0x0400UL)
#define GPIOC_BASE           (PERIPH_AHB4_BASE + 0x0800UL)
#define GPIOD_BASE           (PERIPH_AHB4_BASE + 0x0C00UL)
#define GPIOE_BASE           (PERIPH_AHB4_BASE + 0x1000UL)

typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;     /* 0x0C */
    volatile uint32_t IDR;       /* 0x10 */
    volatile uint32_t ODR;       /* 0x14 */
    volatile uint32_t BSRR;      /* 0x18 */
    volatile uint32_t LCKR;      /* 0x1C */
    volatile uint32_t AFRL;      /* 0x20 */
    volatile uint32_t AFRH;      /* 0x24 */
    volatile uint32_t BRR;       /* 0x28 */
    volatile uint32_t RESERVED; /* 0x2C */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)

/* ---- GPIO helpers (board.h uses PB0 etc. macros) ---- */
#ifndef _PB
#define _PORT(letter) GPIO##letter
#define _PINFULL(letter, n) ((_PORT(letter)), (n))
#define PA(n)  _PINFULL(A, n)
#define PB(n)  _PINFULL(B, n)
#define PC(n)  _PINFULL(C, n)
#define PD(n)  _PINFULL(D, n)
#endif

/* USART1 */
#define USART1_BASE          (PERIPH_APB2_BASE + 0x1000UL)
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t CR3;    /* 0x08 */
    volatile uint32_t BRR;    /* 0x0C */
    volatile uint32_t GTPR;   /* 0x10 */
    volatile uint32_t RTOR;   /* 0x14 */
    volatile uint32_t RQR;    /* 0x18 */
    volatile uint32_t ISR;    /* 0x1C */
    volatile uint32_t ICR;    /* 0x20 */
    volatile uint32_t RDR;    /* 0x24 */
    volatile uint32_t TDR;    /* 0x28 */
} USART_TypeDef;
#define USART1 ((USART_TypeDef *)USART1_BASE)

/* I2C1 */
#define I2C1_BASE            (PERIPH_APB1_BASE + 0x5400UL)
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} I2C_TypeDef;
#define I2C1  ((I2C_TypeDef *)I2C1_BASE)

/* SPI1 */
#define SPI1_BASE            (PERIPH_APB2_BASE + 0x3000UL)
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t CFG1;   /* 0x08 */
    volatile uint32_t CFG2;   /* 0x0C */
    volatile uint32_t IER;    /* 0x10 */
    volatile uint32_t SR;     /* 0x14 */
    volatile uint32_t IFCR;   /* 0x18 */
    volatile uint32_t TXDR;   /* 0x1C - 32-bit TX */
    volatile uint32_t RXDR;   /* 0x20 - 32-bit RX */
    volatile uint32_t CPR;    /* 0x24 */
    volatile uint32_t RXCRC;  /* 0x28 */
    volatile uint32_t TXCRC;  /* 0x2C */
} SPI_TypeDef;
#define SPI1  ((SPI_TypeDef *)SPI1_BASE)

/* EXTI */
#define EXTI_BASE            (PERIPH_AHB3_BASE + 0x0800UL)
typedef struct {
    volatile uint32_t IMR1;
    volatile uint32_t EMR1;
    volatile uint32_t RTSR1;
    volatile uint32_t FTSR1;
    volatile uint32_t SWIER1;
    volatile uint32_t PR1;
} EXTI_TypeDef;
#define EXTI ((EXTI_TypeDef *)EXTI_BASE)

/* HSEM (hardware semaphore) */
#define HSEM_BASE            (PERIPH_AHB3_BASE + 0x2400UL)
#define HSEM_CR(n) (*(volatile uint32_t *)(HSEM_BASE + 0x000UL + 4*(n)))
#define HSEM_R(n)  (*(volatile uint32_t *)(HSEM_BASE + 0x080UL + 4*(n)))
#define HSEM_C1IER  (*(volatile uint32_t *)(HSEM_BASE + 0x100UL))
#define HSEM_C1ICR  (*(volatile uint32_t *)(HSEM_BASE + 0x188UL))
#define HSEM_C1ISR  (*(volatile uint32_t *)(HSEM_BASE + 0x190UL))

/* SysTick (Cortex-M core) */
#define SysTick_BASE         0xE000E010UL
#define SysTick_CTRL         (*(volatile uint32_t *)(SysTick_BASE + 0x00))
#define SysTick_LOAD         (*(volatile uint32_t *)(SysTick_BASE + 0x04))
#define SysTick_VAL          (*(volatile uint32_t *)(SysTick_BASE + 0x08))
#define SysTick_CALIB        (*(volatile uint32_t *)(SysTick_BASE + 0x0C))

#define SysTick_CTRL_ENABLE   (1U << 0)
#define SysTick_CTRL_TICKINT  (1U << 1)
#define SysTick_CTRL_CLKSOURCE (1U << 2)

/* NVIC */
#define NVIC_BASE            0xE000E100UL
#define NVIC_ISER(n)         (*(volatile uint32_t *)(NVIC_BASE + 0x00 + 4*(n)))
#define NVIC_ICER(n)         (*(volatile uint32_t *)(NVIC_BASE + 0x80 + 4*(n)))
#define NVIC_IP(n)           (*(volatile uint8_t *)(0xE000E400UL + (n)))

/* SCB */
#define SCB_BASE             0xE000ED00UL
#define SCB_VTOR             (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_AIRCR            (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_SCR              (*(volatile uint32_t *)(SCB_BASE + 0x10))

/* IRQ numbers (subset) */
#define EXTI1_IRQn           7
#define EXTI9_5_IRQn         23
#define EXTI15_10_IRQn       40
#define USART1_IRQn          37
#define SPI1_IRQn            35
#define TIM2_IRQn            28

/* Interrupt enable helpers */
static inline void nvic_enable(unsigned irqn, unsigned prio)
{
    NVIC_ISER(irqn >> 5) = (1U << (irqn & 31));
    NVIC_IP(irqn) = (uint8_t)(prio & 0xF);
}

static inline void nvic_disable(unsigned irqn)
{
    NVIC_ICER(irqn >> 5) = (1U << (irqn & 31));
}

/* GPIO bit helpers */
static inline void gpio_out(GPIO_TypeDef *p, uint8_t n, uint8_t v)
{
    if (v) p->BSRR = (1U << n);
    else   p->BSRR = (1U << (n + 16));
}

static inline uint8_t gpio_in(GPIO_TypeDef *p, uint8_t n)
{
    return (uint8_t)((p->IDR >> n) & 1U);
}

static inline void gpio_mode(GPIO_TypeDef *p, uint8_t n, uint8_t mode)
{
    p->MODER = (p->MODER & ~(3U << (2*n))) | ((uint32_t)mode << (2*n));
}

static inline void gpio_pupd(GPIO_TypeDef *p, uint8_t n, uint8_t pupd)
{
    p->PUPDR = (p->PUPDR & ~(3U << (2*n))) | ((uint32_t)pupd << (2*n));
}

#endif /* TRACE_REAPER_REGISTERS_H */