/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Register Definitions — STM32H563ZIT6
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GHOSTBUS_REGISTERS_H
#define GHOSTBUS_REGISTERS_H

#include <stdint.h>
#include <stddef.h>

/* =========================================================================
 * STM32H563 Memory Map (relevant subsets)
 * ========================================================================= */

#define PERIPH_BASE        (0x40000000UL)
#define PERIPH_BASE_AHB    (0x40020000UL)
#define PERIPH_BASE_APB1   (0x40000000UL)
#define PERIPH_BASE_APB2   (0x44000000UL)

/* AHB peripherals */
#define GPIOA_BASE         (PERIPH_BASE_AHB + 0x0000)
#define GPIOB_BASE          (PERIPH_BASE_AHB + 0x0400)
#define GPIOC_BASE          (PERIPH_BASE_AHB + 0x0800)
#define GPIOD_BASE          (PERIPH_BASE_AHB + 0x0C00)
#define GPIOE_BASE          (PERIPH_BASE_AHB + 0x1000)
#define GPIOH_BASE          (PERIPH_BASE_AHB + 0x1C00)

#define RCC_BASE            (PERIPH_BASE_AHB + 0x7400)
#define PWR_BASE            (PERIPH_BASE_AHB + 0x7800)

#define DMA1_BASE           (PERIPH_BASE_AHB + 0x6000)
#define DMA1_Stream0_BASE   (DMA1_BASE + 0x0010)

/* APB peripherals */
#define USART1_BASE         (PERIPH_BASE_APB1 + 0x4400) /* USART1 on APB1 in H5 */
#define SPI1_BASE           (PERIPH_BASE_APB1 + 0x5C00) /* placeholder mapping */
#define I2C1_BASE           (PERIPH_BASE_APB1 + 0x5400)
#define TIM2_BASE           (PERIPH_BASE_APB1 + 0x0800)
#define TIM3_BASE           (PERIPH_BASE_APB1 + 0x0400)
#define ADC1_BASE           (PERIPH_BASE_AHB + 0x3000) /* ADC on AHB in H5 */

/* SAES crypto accelerator */
#define SAES_BASE           (PERIPH_BASE_AHB + 0x4000)
#define PKA_BASE            (PERIPH_BASE_AHB + 0x5000)
#define RNG_BASE            (PERIPH_BASE_AHB + 0x6800)

/* =========================================================================
 * Register access macros
 * ========================================================================= */

#define __IO volatile
#define REG32(addr) (*(volatile uint32_t *)(addr))

#define HWREG(addr)        ((volatile uint32_t *)(addr))

/* =========================================================================
 * GPIO register layout (STM32 port-style)
 * ========================================================================= */

typedef struct {
    __IO uint32_t MODER;     /* 0x00 */
    __IO uint32_t OTYPER;    /* 0x04 */
    __IO uint32_t OSPEEDR;   /* 0x08 */
    __IO uint32_t PUPDR;     /* 0x0C */
    __IO uint32_t IDR;       /* 0x10 */
    __IO uint32_t ODR;       /* 0x14 */
    __IO uint32_t BSRR;      /* 0x18 */
    __IO uint32_t LCKR;      /* 0x1C */
    __IO uint32_t AFRL;      /* 0x20 */
    __IO uint32_t AFRH;      /* 0x24 */
    __IO uint32_t BRR;       /* 0x28 */
    __IO uint32_t SECCFGR;   /* 0x2C (H5 secure) */
} GPIO_TypeDef;

#define GPIOA       ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB       ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC       ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD       ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE       ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOH       ((GPIO_TypeDef *) GPIOH_BASE)

/* GPIO mode bits */
#define GPIO_MODE_INPUT      0x00
#define GPIO_MODE_OUTPUT     0x01
#define GPIO_MODE_AF         0x02
#define GPIO_MODE_ANALOG     0x03

#define GPIO_OTYPE_PP        0x00
#define GPIO_OTYPE_OD        0x01

#define GPIO_OSPEED_LOW      0x00
#define GPIO_OSPEED_HIGH     0x03

#define GPIO_PUPD_NONE       0x00
#define GPIO_PUPD_PULLUP     0x01
#define GPIO_PUPD_PULLDOWN   0x02

/* =========================================================================
 * RCC (Reset and Clock Control) — minimal
 * ========================================================================= */

typedef struct {
    __IO uint32_t CR;        /* 0x00 */
    __IO uint32_t ICSCR;     /* 0x04 */
    __IO uint32_t CFGR;      /* 0x08 */
    __IO uint32_t RESERVED0; /* 0x0C */
    __IO uint32_t PLL1CFGR;  /* 0x10 */
    __IO uint32_t PLL1DIVR;  /* 0x14 */
    __IO uint32_t PLL1FRACR; /* 0x18 */
    __IO uint32_t PLL1CSGR;  /* 0x1C */
    /* ... truncated for brevity; we use the AHB/RCC enable regs below ... */
    __IO uint32_t RESERVED1[14];
    __IO uint32_t CCIPR1;    /* 0x60 */
    __IO uint32_t CCIPR2;    /* 0x64 */
    __IO uint32_t CCIPR3;    /* 0x68 */
    __IO uint32_t CCIPR4;    /* 0x6C */
    __IO uint32_t CCIPR5;    /* 0x70 */
    __IO uint32_t RESERVED2[4];
    __IO uint32_t AHB1ENR;   /* 0x84 */
    __IO uint32_t AHB2ENR;   /* 0x88 */
    __IO uint32_t AHB3ENR;   /* 0x8C */
    __IO uint32_t RESERVED3;
    __IO uint32_t APB1LENR;  /* 0x94 */
    __IO uint32_t APB1HENR;  /* 0x98 */
    __IO uint32_t APB2ENR;   /* 0x9C */
    __IO uint32_t APB3ENR;   /* 0xA0 */
} RCC_TypeDef;

#define RCC         ((RCC_TypeDef *) RCC_BASE)

/* RCC enable bit positions (subset) */
#define RCC_AHB1ENR_GPIOAEN   (1U << 0)
#define RCC_AHB1ENR_GPIOBEN   (1U << 1)
#define RCC_AHB1ENR_GPIOCEN   (1U << 2)
#define RCC_AHB1ENR_GPIODEN   (1U << 3)
#define RCC_AHB1ENR_GPIOEEN   (1U << 4)
#define RCC_AHB1ENR_GPIOHEN   (1U << 7)
#define RCC_AHB1ENR_DMA1EN    (1U << 24)
#define RCC_AHB1ENR_ADC1EN    (1U << 20)

#define RCC_APB1LENR_USART1EN (1U << 14)
#define RCC_APB1LENR_I2C1EN   (1U << 21)
#define RCC_APB1LENR_SPI1EN   (1U << 15)
#define RCC_APB1LENR_TIM2EN   (1U << 0)
#define RCC_APB1LENR_TIM3EN   (1U << 1)

#define RCC_AHB2ENR_SAESEN    (1U << 0)
#define RCC_AHB2ENR_PKAEN     (1U << 2)
#define RCC_AHB2ENR_RNGEN     (1U << 6)

/* =========================================================================
 * USART register layout (for BLE co-processor UART bridge)
 * ========================================================================= */

typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t CR3;    /* 0x08 */
    __IO uint32_t BRR;    /* 0x0C */
    __IO uint32_t GTPR;   /* 0x10 */
    __IO uint32_t RTOR;   /* 0x14 */
    __IO uint32_t RQR;    /* 0x18 */
    __IO uint32_t ISR;    /* 0x1C */
    __IO uint32_t ICR;    /* 0x20 */
    __IO uint32_t RDR;    /* 0x24 */
    __IO uint32_t TDR;    /* 0x28 */
} USART_TypeDef;

#define USART1       ((USART_TypeDef *) USART1_BASE)

#define USART_CR1_UE          (1U << 0)
#define USART_CR1_RE          (1U << 2)
#define USART_CR1_TE          (1U << 3)
#define USART_CR1_RXNEIE      (1U << 5)
#define USART_CR1_TXEIE       (1U << 7)
#define USART_ISR_RXNE        (1U << 5)
#define USART_ISR_TXE        (1U << 7)
#define USART_ISR_TC          (1U << 6)

/* =========================================================================
 * ADC (for impedance sensing during pin discovery)
 * ========================================================================= */

typedef struct {
    __IO uint32_t ISR;    /* 0x00 */
    __IO uint32_t IER;    /* 0x04 */
    __IO uint32_t CR;    /* 0x08 */
    __IO uint32_t CFGR;  /* 0x0C */
    __IO uint32_t CFGR2; /* 0x10 */
    __IO uint32_t SMPR1; /* 0x14 */
    __IO uint32_t SMPR2; /* 0x18 */
    __IO uint32_t RESERVED0[2];
    __IO uint32_t AWD1TR;/* 0x20 */
    __IO uint32_t AWD2TR;/* 0x24 */
    __IO uint32_t CHAWDSSR; /* 0x28 */
    __IO uint32_t RESERVED1[5];
    __IO uint32_t DR;    /* 0x40 */
} ADC_TypeDef;

#define ADC1         ((ADC_TypeDef *) ADC1_BASE)

#define ADC_CR_ADEN          (1U << 0)
#define ADC_CR_ADCAL         (1U << 16)
#define ADC_ISR_ADRDY        (1U << 0)
#define ADC_ISR_EOC          (1U << 2)
#define ADC_CFGR_CONT        (1U << 2)

/* =========================================================================
 * SAES (AES accelerator) — minimal subset
 * ========================================================================= */

typedef struct {
    __IO uint32_t CR;      /* 0x00 */
    __IO uint32_t SR;      /* 0x04 */
    __IO uint32_t SUSPR;   /* 0x08 */
    __IO uint32_t KEYR0;   /* 0x0C */
    __IO uint32_t KEYR1;   /* 0x10 */
    __IO uint32_t KEYR2;   /* 0x14 */
    __IO uint32_t KEYR3;   /* 0x18 */
    __IO uint32_t KEYR4;   /* 0x1C */
    __IO uint32_t KEYR5;   /* 0x20 */
    __IO uint32_t KEYR6;   /* 0x24 */
    __IO uint32_t KEYR7;   /* 0x28 */
    __IO uint32_t IVR0;    /* 0x2C */
    __IO uint32_t IVR1;    /* 0x30 */
    __IO uint32_t IVR2;    /* 0x34 */
    __IO uint32_t IVR3;    /* 0x38 */
    __IO uint32_t DINR;    /* 0x3C */
    __IO uint32_t DOUTR;   /* 0x40 */
} SAES_TypeDef;

#define SAES         ((SAES_TypeDef *) SAES_BASE)

#define SAES_CR_EN            (1U << 0)
#define SAES_CR_MODE_MASK     (0x7U << 2)
#define SAES_CR_MODE_GCM      (0x2U << 2)
#define SAES_CR_KEYSIZE_256   (1U << 4)
#define SAES_SR_BUSY          (1U << 0)
#define SAES_SR_CCF          (1U << 1)
#define SAES_SR_KEYVALID      (1U << 7)

/* =========================================================================
 * PKA (public-key accelerator) for ECDH
 * ========================================================================= */

typedef struct {
    __IO uint32_t CR;      /* 0x00 */
    __IO uint32_t SR;      /* 0x04 */
    __IO uint32_t CLRFR;   /* 0x08 */
    __IO uint32_t RAM;     /* 0x0C */  /* indirect via RAMADDR/RAMDATA */
    __IO uint32_t RAMADDR; /* 0x10 */
    __IO uint32_t RAMDATA; /* 0x14 */
} PKA_TypeDef;

#define PKA          ((PKA_TypeDef *) PKA_BASE)

#define PKA_CR_EN            (1U << 0)
#define PKA_CR_START         (1U << 1)
#define PKA_CR_MODE_ECDH     (0x03U)
#define PKA_SR_BUSY          (1U << 0)
#define PKA_SR_PROCENDF      (1U << 1)

/* =========================================================================
 * RNG (true random number generator)
 * ========================================================================= */

typedef struct {
    __IO uint32_t CR;     /* 0x00 */
    __IO uint32_t SR;     /* 0x04 */
    __IO uint32_t DR;     /* 0x08 */
} RNG_TypeDef;

#define RNG          ((RNG_TypeDef *) RNG_BASE)

#define RNG_CR_EN            (1U << 2)
#define RNG_SR_DRDY          (1U << 0)
#define RNG_SR_CEIS          (1U << 1)
#define RNG_SR_SEIS          (1U << 2)

/* =========================================================================
 * NVIC (Cortex-M33) — minimal
 * ========================================================================= */

#define NVIC_BASE           (0xE000E100UL)
#define NVIC_ISER0          REG32(NVIC_BASE + 0x000)
#define NVIC_ICER0          REG32(NVIC_BASE + 0x080)
#define NVIC_ISPR0          REG32(NVIC_BASE + 0x100)
#define NVIC_ICPR0          REG32(NVIC_BASE + 0x180)

#define SCB_BASE            (0xE000ED00UL)
#define SCB_VTOR            REG32(SCB_BASE + 0x008)
#define SCB_CPACR           REG32(SCB_BASE + 0x088)
#define SCB_SCR_SLEEPONEXIT REG32(SCB_BASE + 0x010)
#define SCB_SCR             REG32(SCB_BASE + 0x010)

#define SysTick_BASE        (0xE000E010UL)
#define SysTick_CSR         REG32(SysTick_BASE + 0x00)
#define SysTick_RVR         REG32(SysTick_BASE + 0x04)
#define SysTick_CVR         REG32(SysTick_BASE + 0x08)

#define SysTick_CSR_ENABLE  (1U << 0)
#define SysTick_CSR_CLKSRC  (1U << 2)
#define SysTick_CSR_TICKINT (1U << 1)

/* =========================================================================
 * Common helper macros
 * ========================================================================= */

#define BIT(n)            (1U << (n))
#define ARRAY_SIZE(a)     (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)          ((a) < (b) ? (a) : (b))
#define MAX(a,b)          ((a) > (b) ? (a) : (b))

#define GHOSTBUS_FW_VERSION \
    ((FW_VERSION_MAJOR << 16) | (FW_VERSION_MINOR << 8) | FW_VERSION_PATCH)

#endif /* GHOSTBUS_REGISTERS_H */