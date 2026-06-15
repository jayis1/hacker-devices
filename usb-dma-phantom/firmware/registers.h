/*
 * registers.h — STM32F423 peripheral register definitions
 * USB DMA Phantom firmware
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 *
 * Based on STM32F4 reference manual (RM0090) and STM32F423 datasheet.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ---- Peripheral base addresses ---- */
#define PERIPH_BASE        0x40000000UL
#define APB1PERIPH_BASE    PERIPH_BASE
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE    (PERIPH_BASE + 0x10000000UL)

/* ---- APB1 peripherals ---- */
#define TIM2_BASE          (APB1PERIPH_BASE + 0x0000UL)
#define TIM6_BASE          (APB1PERIPH_BASE + 0x0100UL)
#define USART4_BASE        (APB1PERIPH_BASE + 0x0C00UL)
#define I2C1_BASE          (APB1PERIPH_BASE + 0x5400UL)
#define SPI4_BASE          (APB1PERIPH_BASE + 0x3400UL)
#define RTC_BASE           (APB1PERIPH_BASE + 0x2800UL)
#define PWR_BASE           (APB1PERIPH_BASE + 0x7000UL)

/* ---- APB2 peripherals ---- */
#define ADC1_BASE          (APB2PERIPH_BASE + 0x2400UL)
#define SYSCFG_BASE        (APB2PERIPH_BASE + 0x3800UL)
#define EXTI_BASE          (APB2PERIPH_BASE + 0x3C00UL)

/* ---- AHB1 peripherals ---- */
#define GPIOA_BASE         (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE         (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE         (AHB1PERIPH_BASE + 0x0800UL)
#define DMA1_BASE           (AHB1PERIPH_BASE + 0x6000UL)
#define DMA2_BASE           (AHB1PERIPH_BASE + 0x6400UL)
#define FLASH_BASE_REG     (AHB1PERIPH_BASE + 0x3C00UL)
#define CRC_BASE            (AHB1PERIPH_BASE + 0x3000UL)

/* ---- AHB2 peripherals ---- */
#define OTG_FS_BASE        (AHB2PERIPH_BASE + 0x00000UL)
#define RNG_BASE           (AHB2PERIPH_BASE + 0x60800UL)
#define AES_BASE           (AHB2PERIPH_BASE + 0x61000UL)
#define HASH_BASE          (AHB2PERIPH_BASE + 0x60400UL)

/* ---- RCC registers ---- */
#define RCC                ((RCC_TypeDef *)0x40023800UL)

typedef struct {
    __IO uint32_t CR;            /* Offset 0x00: Clock control register */
    __IO uint32_t PLLCFGR;      /* Offset 0x04: PLL configuration register */
    __IO uint32_t CFGR;         /* Offset 0x08: Clock configuration register */
    __IO uint32_t CIR;          /* Offset 0x0C: Clock interrupt register */
    __IO uint32_t AHB1ENR;      /* Offset 0x30: AHB1 peripheral clock enable */
    __IO uint32_t AHB2ENR;      /* Offset 0x34: AHB2 peripheral clock enable */
    __IO uint32_t AHB3ENR;      /* Offset 0x38: AHB3 peripheral clock enable */
    __IO uint32_t APB1ENR;      /* Offset 0x40: APB1 peripheral clock enable */
    __IO uint32_t APB2ENR;      /* Offset 0x44: APB2 peripheral clock enable */
    __IO uint32_t AHB1LPENR;    /* Offset 0x50: AHB1 low-power enable */
    __IO uint32_t AHB2LPENR;    /* Offset 0x54: AHB2 low-power enable */
    __IO uint32_t APB1LPENR;    /* Offset 0x60: APB1 low-power enable */
    __IO uint32_t APB2LPENR;    /* Offset 0x64: APB2 low-power enable */
    __IO uint32_t BDCR;         /* Offset 0x70: Backup domain control */
    __IO uint32_t CSR;          /* Offset 0x74: Clock control & status */
    __IO uint32_t DCKCFGR;      /* Offset 0x8C: Dedicated clocks config */
    __IO uint32_t DCKCFGR2;     /* Offset 0x90: Dedicated clocks config 2 */
} RCC_TypeDef;

/* RCC CR bits */
#define RCC_CR_HSION       (1UL << 0)
#define RCC_CR_HSEON       (1UL << 1)
#define RCC_CR_HSERDY      (1UL << 17)
#define RCC_CR_PLLON       (1UL << 24)
#define RCC_CR_PLLRDY      (1UL << 25)
#define RCC_CR_HSI48ON     (1UL << 28)

/* RCC PLLCFGR bits */
#define RCC_PLLCFGR_PLLSRC_HSE  (1UL << 22)

/* RCC CFGR bits */
#define RCC_CFGR_SW_PLL    (2UL << 0)
#define RCC_CFGR_SWS_PLL   (2UL << 2)

/* RCC AHB1ENR bits */
#define RCC_AHB1ENR_GPIOAEN   (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN   (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN   (1UL << 2)
#define RCC_AHB1ENR_DMA2EN    (1UL << 22)
#define RCC_AHB1ENR_CRCEN     (1UL << 12)
#define RCC_AHB1ENR_BKPSRAMEN (1UL << 18)

/* RCC APB1ENR bits */
#define RCC_APB1ENR_SPI4EN    (1UL << 13)
#define RCC_APB1ENR_USART4EN  (1UL << 19)
#define RCC_APB1ENR_I2C1EN    (1UL << 21)
#define RCC_APB1ENR_TIM2EN     (1UL << 0)
#define RCC_APB1ENR_TIM6EN     (1UL << 4)
#define RCC_APB1ENR_PWREN      (1UL << 28)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_ADC1EN    (1UL << 8)
#define RCC_APB2ENR_SYSCFGEN  (1UL << 14)

/* RCC AHB2ENR bits */
#define RCC_AHB2ENR_OTGFSEN   (1UL << 7)
#define RCC_AHB2ENR_RNGEN      (1UL << 6)
#define RCC_AHB2ENR_AESEN      (1UL << 4)
#define RCC_AHB2ENR_HASHEN     (1UL << 5)

/* ---- GPIO registers ---- */
typedef struct {
    __IO uint32_t MODER;      /* Offset 0x00: Port mode register */
    __IO uint32_t OTYPER;     /* Offset 0x04: Port output type register */
    __IO uint32_t OSPEEDR;    /* Offset 0x08: Port output speed register */
    __IO uint32_t PUPDR;      /* Offset 0x0C: Port pull-up/pull-down register */
    __IO uint32_t IDR;        /* Offset 0x10: Port input data register */
    __IO uint32_t ODR;        /* Offset 0x14: Port output data register */
    __IO uint32_t BSRR;       /* Offset 0x18: Port bit set/reset register */
    __IO uint32_t LCKR;       /* Offset 0x1C: Port configuration lock register */
    __IO uint32_t AFR[2];     /* Offset 0x20-0x24: Alternate function registers */
} GPIO_TypeDef;

#define GPIOA  ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB  ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC  ((GPIO_TypeDef *)GPIOC_BASE)

/* GPIO MODER values */
#define GPIO_MODE_INPUT      0x0
#define GPIO_MODE_OUTPUT     0x1
#define GPIO_MODE_AF         0x2
#define GPIO_MODE_ANALOG     0x3

/* GPIO OSPEEDR values */
#define GPIO_SPEED_LOW       0x0
#define GPIO_SPEED_MED       0x1
#define GPIO_SPEED_HIGH      0x2
#define GPIO_SPEED_VHIGH     0x3

/* ---- SPI registers ---- */
typedef struct {
    __IO uint32_t CR1;        /* Offset 0x00: Control register 1 */
    __IO uint32_t CR2;        /* Offset 0x04: Control register 2 */
    __IO uint32_t SR;         /* Offset 0x08: Status register */
    __IO uint32_t DR;         /* Offset 0x0C: Data register */
    __IO uint32_t CRCPR;      /* Offset 0x10: CRC polynomial register */
    __IO uint32_t RXCRCR;    /* Offset 0x14: RX CRC register */
    __IO uint32_t TXCRCR;    /* Offset 0x18: TX CRC register */
    __IO uint32_t I2SCFGR;   /* Offset 0x1C: I2S configuration register */
} SPI_TypeDef;

#define SPI4  ((SPI_TypeDef *)SPI4_BASE)

/* SPI CR1 bits */
#define SPI_CR1_CPHA     (1UL << 0)
#define SPI_CR1_CPOL     (1UL << 1)
#define SPI_CR1_MSTR     (1UL << 2)
#define SPI_CR1_BR_SHIFT  (3UL)
#define SPI_CR1_SPE      (1UL << 6)
#define SPI_CR1_LSBFIRST (1UL << 7)
#define SPI_CR1_SSI      (1UL << 8)
#define SPI_CR1_SSM      (1UL << 9)
#define SPI_CR1_BIDIMODE (1UL << 15)

/* SPI SR bits */
#define SPI_SR_RXNE      (1UL << 0)
#define SPI_SR_TXE       (1UL << 1)
#define SPI_SR_BSY       (1UL << 7)

/* SPI CR2 bits */
#define SPI_CR2_DMAEN     (1UL << 0)
#define SPI_CR2_TXDMAEN   (1UL << 1)
#define SPI_CR2_RXNEIE   (1UL << 6)
#define SPI_CR2_TXEIE     (1UL << 7)

/* ---- USART registers ---- */
typedef struct {
    __IO uint32_t SR;         /* Offset 0x00: Status register */
    __IO uint32_t DR;         /* Offset 0x04: Data register */
    __IO uint32_t BRR;        /* Offset 0x08: Baud rate register */
    __IO uint32_t CR1;       /* Offset 0x0C: Control register 1 */
    __IO uint32_t CR2;       /* Offset 0x10: Control register 2 */
    __IO uint32_t CR3;       /* Offset 0x14: Control register 3 */
} USART_TypeDef;

#define USART4  ((USART_TypeDef *)USART4_BASE)

/* USART CR1 bits */
#define USART_CR1_UE        (1UL << 13)
#define USART_CR1_TE        (1UL << 3)
#define USART_CR1_RE        (1UL << 2)
#define USART_CR1_RXNEIE    (1UL << 5)
#define USART_CR1_TXEIE     (1UL << 7)
#define USART_CR1_TCIE      (1UL << 6)

/* USART CR3 bits */
#define USART_CR3_CTSE      (1UL << 9)
#define USART_CR3_RTSE      (1UL << 8)

/* ---- DMA registers ---- */
typedef struct {
    __IO uint32_t CR;         /* Offset 0x00: Stream configuration */
    __IO uint32_t NDTR;      /* Offset 0x04: Number of data register */
    __IO uint32_t PAR;        /* Offset 0x08: Peripheral address register */
    __IO uint32_t M0AR;      /* Offset 0x0C: Memory 0 address register */
    __IO uint32_t M1AR;      /* Offset 0x10: Memory 1 address register */
    __IO uint32_t FCR;        /* Offset 0x14: FIFO control register */
} DMA_Stream_TypeDef;

typedef struct {
    __IO uint32_t LISR;       /* Offset 0x00: Low interrupt status */
    __IO uint32_t HISR;       /* Offset 0x04: High interrupt status */
    __IO uint32_t LIFCR;      /* Offset 0x08: Low interrupt flag clear */
    __IO uint32_t HIFCR;      /* Offset 0x0C: High interrupt flag clear */
} DMA_TypeDef;

#define DMA1  ((DMA_TypeDef *)DMA1_BASE)
#define DMA2  ((DMA_TypeDef *)DMA2_BASE)

/* DMA SxCR bits */
#define DMA_SxCR_EN        (1UL << 0)
#define DMA_SxCR_TCIE      (1UL << 1)
#define DMA_SxCR_MINC       (1UL << 10)
#define DMA_SxCR_PINC       (1UL << 9)
#define DMA_SxCR_DIR_SHIFT  (6UL)
#define DMA_SxCR_CHSEL_SHIFT (25UL)

/* ---- I2C registers ---- */
typedef struct {
    __IO uint32_t CR1;        /* Offset 0x00: Control register 1 */
    __IO uint32_t CR2;        /* Offset 0x04: Control register 2 */
    __IO uint32_t OAR1;      /* Offset 0x08: Own address register 1 */
    __IO uint32_t OAR2;      /* Offset 0x0C: Own address register 2 */
    __IO uint32_t DR;         /* Offset 0x10: Data register */
    __IO uint32_t SR1;       /* Offset 0x14: Status register 1 */
    __IO uint32_t SR2;       /* Offset 0x18: Status register 2 */
    __IO uint32_CCR;        /* Offset 0x1C: Clock control register */
    __IO uint32_t TRISE;     /* Offset 0x20: Rise time register */
} I2C_TypeDef;

#define I2C1  ((I2C_TypeDef *)I2C1_BASE)

/* I2C CR1 bits */
#define I2C_CR1_PE         (1UL << 0)
#define I2C_CR1_START      (1UL << 8)
#define I2C_CR1_STOP       (1UL << 9)
#define I2C_CR1_ACK        (1UL << 10)

/* I2C SR1 bits */
#define I2C_SR1_SB         (1UL << 0)
#define I2C_SR1_ADDR       (1UL << 1)
#define I2C_SR1_TXE        (1UL << 7)
#define I2C_SR1_RXNE       (1UL << 6)
#define I2C_SR1_BTF        (1UL << 8)

/* ---- ADC registers ---- */
typedef struct {
    __IO uint32_t SR;          /* Offset 0x00: Status register */
    __IO uint32_t CR1;         /* Offset 0x04: Control register 1 */
    __IO uint32_t CR2;         /* Offset 0x08: Control register 2 */
    __IO uint32_t SMPR1;      /* Offset 0x0C: Sample time register 1 */
    __IO uint32_t SMPR2;      /* Offset 0x10: Sample time register 2 */
    __IO uint32_t JOFR[4];    /* Offset 0x14-0x20: Injected offset */
    __IO uint32_t HTR;        /* Offset 0x24: Watchdog high threshold */
    __IO uint32_t LTR;        /* Offset 0x28: Watchdog low threshold */
    __IO uint32_t JDR[4];    /* Offset 0x3C-0x48: Injected data */
    __IO uint32_t DR;          /* Offset 0x4C: Regular data register */
} ADC_TypeDef;

#define ADC1  ((ADC_TypeDef *)ADC1_BASE)

/* ADC CR1 bits */
#define ADC_CR1_EOCIE       (1UL << 5)

/* ADC CR2 bits */
#define ADC_CR2_ADON        (1UL << 0)
#define ADC_CR2_CONT        (1UL << 1)
#define ADC_CR2_SWSTART     (1UL << 30)

/* ---- PWR registers ---- */
typedef struct {
    __IO uint32_t CR;          /* Offset 0x00: Control register */
    __IO uint32_t CSR;         /* Offset 0x04: Control/status register */
} PWR_TypeDef;

#define PWR   ((PWR_TypeDef *)PWR_BASE)

#define PWR_CR_ODEN      (1UL << 16)
#define PWR_CSR_ODRDY     (1UL << 17)

/* ---- FLASH registers ---- */
typedef struct {
    __IO uint32_t ACR;         /* Offset 0x00: Access control register */
    __IO uint32_t KEYR;       /* Offset 0x04: Key register */
    __IO uint32_t OPTKEYR;   /* Offset 0x08: Option key register */
    __IO uint32_t SR;          /* Offset 0x0C: Status register */
    __IO uint32_t CR;          /* Offset 0x10: Control register */
    __IO uint32_t OPTCR;      /* Offset 0x14: Option control register */
} FLASH_TypeDef;

#define FLASH  ((FLASH_TypeDef *)FLASH_BASE_REG)

#define FLASH_ACR_PRFTEN    (1UL << 0)
#define FLASH_ACR_ICEN      (1UL << 1)
#define FLASH_ACR_DCEN      (1UL << 2)

/* ---- RNG registers ---- */
typedef struct {
    __IO uint32_t CR;          /* Offset 0x00: Control register */
    __IO uint32_t SR;          /* Offset 0x04: Status register */
    __IO uint32_t DR;          /* Offset 0x08: Data register */
} RNG_TypeDef;

#define RNG   ((RNG_TypeDef *)RNG_BASE)

#define RNG_CR_RNGEN     (1UL << 2)
#define RNG_SR_DRDY      (1UL << 0)
#define RNG_SR_CEIS      (1UL << 1)
#define RNG_SR_SEIS      (1UL << 2)

/* ---- AES registers ---- */
typedef struct {
    __IO uint32_t CR;          /* Offset 0x00: Control register */
    __IO uint32_t SR;          /* Offset 0x04: Status register */
    __IO uint32_t DINR;       /* Offset 0x08: Data input register */
    __IO uint32_t DOUTR;      /* Offset 0x0C: Data output register */
    __IO uint32_t KEYR[4];   /* Offset 0x10-0x1C: Key registers */
    __IO uint32_t IVR[4];    /* Offset 0x20-0x2C: IV registers */
} AES_TypeDef;

#define AES   ((AES_TypeDef *)AES_BASE)

#define AES_CR_EN          (1UL << 0)
#define AES_CR_DATATYPE_SHIFT (2UL)
#define AES_CR_KEYSIZE_128  (0UL << 4)
#define AES_CR_KEYSIZE_256  (1UL << 4)
#define AES_CR_MODE_ENC     (0UL << 3)
#define AES_CR_MODE_DEC     (2UL << 3)
#define AES_CR_MODE_KEY     (3UL << 3)
#define AES_SR_CCF          (1UL << 0)

/* ---- HASH registers ---- */
typedef struct {
    __IO uint32_t CR;          /* Offset 0x00: Control register */
    __IO uint32_t DIN;         /* Offset 0x04: Data input register */
    __IO uint32_t STR;         /* Offset 0x08: Start register */
    __IO uint32_t HR[5];      /* Offset 0x10-0x20: Hash result */
    __IO uint32_t IMR;        /* Offset 0x40: Interrupt mask */
    __IO uint32_t SR;          /* Offset 0x44: Status register */
} HASH_TypeDef;

#define HASH  ((HASH_TypeDef *)HASH_BASE)

#define HASH_CR_INIT       (1UL << 2)
#define HASH_CR_MODE       (1UL << 6)
#define HASH_CR_ALGO_SHA256 (1UL << 7)
#define HASH_CR_DATATYPE_8B (1UL << 4)
#define HASH_SR_DCIS       (1UL << 1)
#define HASH_SR_DINIS      (1UL << 0)

/* ---- SysTick ---- */
#define SysTick  ((SysTick_TypeDef *)0xE000E010UL)

typedef struct {
    __IO uint32_t CTRL;        /* Offset 0x00: Control register */
    __IO uint32_t LOAD;        /* Offset 0x04: Reload value register */
    __IO uint32_t VAL;         /* Offset 0x08: Current value register */
    __IO uint32_t CALIB;      /* Offset 0x0C: Calibration register */
} SysTick_TypeDef;

#define SysTick_CTRL_ENABLE    (1UL << 0)
#define SysTick_CTRL_CLKSOURCE (1UL << 2)

/* ---- NVIC ---- */
#define NVIC_BASE        0xE000E100UL

typedef struct {
    __IO uint32_t ISER[8];    /* Interrupt set-enable */
    uint32_t RESERVED0[24];
    __IO uint32_t ICER[8];    /* Interrupt clear-enable */
    uint32_t RESERVED1[24];
    __IO uint32_t ISPR[8];    /* Interrupt set-pending */
    uint32_t RESERVED2[24];
    __IO uint32_t ICPR[8];    /* Interrupt clear-pending */
} NVIC_TypeDef;

#define NVIC  ((NVIC_TypeDef *)NVIC_BASE)

/* ---- Interrupt numbers ---- */
#define SPI4_IRQn          84
#define USART4_IRQn        52
#define DMA2_Stream0_IRQn   68
#define DMA2_Stream4_IRQn   72
#define EXTI15_10_IRQn     40
#define I2C1_IRQn           31
#define ADC_IRQn            18

/* ---- Core definitions ---- */
#define __IO volatile
#define STATIC_ASSERT(cond) typedef char static_assert_##line[(cond) ? 1 : -1]

#endif /* REGISTERS_H */