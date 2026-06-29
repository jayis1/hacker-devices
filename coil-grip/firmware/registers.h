/*
 * registers.h — STM32H743 register definitions used by CoilGrip firmware
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Minimal hand-rolled register map for the peripherals CoilGrip touches.
 * We deliberately avoid pulling in the full ST HAL to keep the firmware
 * self-contained and reviewable. Offsets follow RM0433 (STM32H743).
 */

#ifndef COILGRIP_REGISTERS_H
#define COILGRIP_REGISTERS_H

#include <stdint.h>

#define __IO volatile

/* ---- RCC ---------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CR;         /* 0x00 */
    __IO uint32_t HSIDIV;      /* 0x04 */
    __IO uint32_t CFGR;       /* 0x08 */
    __IO uint32_t D1CFGR;     /* 0x0C */
    __IO uint32_t D2CFGR;     /* 0x10 */
    __IO uint32_t D3CFGR;     /* 0x14 */
    __IO uint32_t RESERVED0[2];
    __IO uint32_t CKCFGR;     /* 0x20 */
    __IO uint32_t RESERVED1[7];
    __IO uint32_t D1CCIPR;    /* 0x40 */
    __IO uint32_t D2CCIP1R;   /* 0x44 */
    __IO uint32_t D2CCIP2R;   /* 0x48 */
    __IO uint32_t D3CCIPR;    /* 0x4C */
    __IO uint32_t RESERVED2[2];
    __IO uint32_t CIER;       /* 0x58 */
    __IO uint32_t CIFR;       /* 0x5C */
    __IO uint32_t CICR;       /* 0x60 */
    __IO uint32_t RESERVED3[4];
    __IO uint32_t BDCR;       /* 0x70 */
    __IO uint32_t CSR;        /* 0x74 */
    __IO uint32_t RESERVED4[24];
    __IO uint32_t AHB1ENR;    /* 0xD8 */
    __IO uint32_t AHB2ENR;    /* 0xDC */
    __IO uint32_t AHB3ENR;    /* 0xE0 */
    __IO uint32_t AHB4ENR;    /* 0xE4 */
    __IO uint32_t APB1LENR;   /* 0xE8 */
    __IO uint32_t APB1HENR;   /* 0xEC */
    __IO uint32_t APB2ENR;    /* 0xF0 */
    __IO uint32_t APB3ENR;    /* 0xF4 */
    __IO uint32_t APB4ENR;    /* 0xF8 */
} rcc_t;

#define RCC                     ((rcc_t *) RCC_BASE)

/* RCC bits */
#define RCC_CR_HSIRDY           (1U << 10)
#define RCC_CR_HSERDY           (1U << 17)
#define RCC_CR_HSIDIV_DIV1      (0U << 3)
#define RCC_CFGR_SW_HSI          (0U)
#define RCC_CFGR_SW_HSE         (1U)
#define RCC_CFGR_SW_PLL1        (2U)
#define RCC_CFGR_SWS_PLL1       (2U << 3)
#define RCC_AHB4ENR_GPIOAEN      (1U << 0)
#define RCC_AHB4ENR_GPIOBEN      (1U << 1)
#define RCC_AHB4ENR_GPIOCEN      (1U << 2)
#define RCC_AHB4ENR_GPIODEN      (1U << 3)
#define RCC_AHB4ENR_GPIOEEN      (1U << 4)
#define RCC_AHB4ENR_GPIOHEN      (1U << 7)
#define RCC_APB1LENR_SPI2EN      (1U << 14)
#define RCC_APB1LENR_I2C1EN      (1U << 21)
#define RCC_APB1HENR_USART3EN    (1U << 18)
#define RCC_APB2ENR_SPI1EN       (1U << 12)
#define RCC_APB2ENR_TIM2EN       (1U << 0)
#define RCC_APB2ENR_HRTIM1EN     (1U << 29)
#define RCC_AHB1ENR_DMA1EN       (1U << 0)
#define RCC_AHB3ENR_QSPIEN       (1U << 14)
#define RCC_AHB4ENR_ADC12EN      (1U << 12) /* D1CCIPR */

/* ---- GPIO --------------------------------------------------------------- */
typedef struct {
    __IO uint32_t MODER;    /* 0x00 */
    __IO uint32_t OTYPER;   /* 0x04 */
    __IO uint32_t OSPEEDR;  /* 0x08 */
    __IO uint32_t PUPDR;    /* 0x0C */
    __IO uint32_t IDR;      /* 0x10 */
    __IO uint32_t ODR;      /* 0x14 */
    __IO uint32_t BSRR;     /* 0x18 */
    __IO uint32_t LCKR;     /* 0x1C */
    __IO uint32_t AFRL;     /* 0x20 */
    __IO uint32_t AFRH;     /* 0x24 */
} gpio_t;

#define GPIOA   ((gpio_t *) GPIOA_BASE)
#define GPIOB   ((gpio_t *) GPIOB_BASE)
#define GPIOC   ((gpio_t *) GPIOC_BASE)
#define GPIOD   ((gpio_t *) GPIOD_BASE)
#define GPIOE   ((gpio_t *) GPIOE_BASE)
#define GPIOH   ((gpio_t *) GPIOH_BASE)

/* GPIO modes */
#define GPIO_MODE_INPUT         0
#define GPIO_MODE_OUTPUT_PP     1
#define GPIO_MODE_AF            2
#define GPIO_MODE_ANALOG        3

/* ---- PWR ---------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CR1;      /* 0x00 */
    __IO uint32_t CR2;      /* 0x04 */
    __IO uint32_t CR3;      /* 0x08 */
    __IO uint32_t VOSR;     /* 0x0C */
    __IO uint32_t RESERVED0[2];
    __IO uint32_t CPUCR;    /* 0x18 */
    __IO uint32_t RESERVED1[7];
    __IO uint32_t D3CR;     /* 0x34 */
    __IO uint32_t SRDPCR;   /* 0x38 */
} pwr_t;

#define PWR     ((pwr_t *) PWR_BASE)

/* ---- SPI ---------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CR1;      /* 0x00 */
    __IO uint32_t CR2;      /* 0x04 */
    __IO uint32_t CFG1;     /* 0x08 */
    __IO uint32_t CFG2;     /* 0x0C */
    __IO uint32_t IER;      /* 0x10 */
    __IO uint32_t SR;       /* 0x14 */
    __IO uint32_t IFCR;     /* 0x18 */
    __IO uint32_t TXDR;     /* 0x1C (32-bit) */
    __IO uint32_t RXDR;     /* 0x20 (32-bit) */
    __IO uint32_t RESERVED0;
    __IO uint32_t UDRCFG;   /* 0x28 */
    __IO uint32_t TSER;     /* 0x2C */
} spi_t;

#define SPI1    ((spi_t *) SPI1_BASE)
#define SPI2    ((spi_t *) SPI2_BASE)

#define SPI_CR1_SPE             (1U << 0)
#define SPI_CR1_CSTART          (1U << 1)
#define SPI_CR1_HSPI            (1U << 3)
#define SPI_CFG1_MASTER         (1U << 2)
#define SPI_CFG1_DSIZE_8BIT     (7U << 0)
#define SPI_CFG1_TXDMAEN        (1U << 15)
#define SPI_CFG1_RXDMAEN        (1U << 14)
#define SPI_CFG2_CPOL           (1U << 0)
#define SPI_CFG2_CPHA           (1U << 1)
#define SPI_CFG2_MASTER         (1U << 22)
#define SPI_CFG2_SSOE           (1U << 11)
#define SPI_SR_TXP              (1U << 0)
#define SPI_SR_RXP              (1U << 2)
#define SPI_SR_EOT              (1U << 3)
#define SPI_SR_OVR              (1U << 6)
#define SPI_IFCR_EOTC           (1U << 3)

/* ---- I2C ---------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CR1;      /* 0x00 */
    __IO uint32_t CR2;      /* 0x04 */
    __IO uint32_t OAR1;     /* 0x08 */
    __IO uint32_t OAR2;     /* 0x0C */
    __IO uint32_t TIMINGR;  /* 0x10 */
    __IO uint32_t TIMEOUTR; /* 0x14 */
    __IO uint32_t ISR;      /* 0x18 */
    __IO uint32_t ICR;      /* 0x1C */
    __IO uint32_t PECR;     /* 0x20 */
    __IO uint32_t RXDR;     /* 0x24 */
    __IO uint32_t TXDR;     /* 0x28 */
} i2c_t;

#define I2C1    ((i2c_t *) I2C1_BASE)
#define I2C4    ((i2c_t *) I2C4_BASE)

#define I2C_CR1_PE              (1U << 0)
#define I2C_CR1_TXIE            (1U << 1)
#define I2C_CR1_RXIE            (1U << 2)
#define I2C_CR2_START           (1U << 13)
#define I2C_CR2_STOP            (1U << 14)
#define I2C_CR2_NBYTES(n)       (((uint32_t)(n)) << 16)
#define I2C_CR2_AUTOEND         (1U << 20)
#define I2C_ISR_TXE             (1U << 0)
#define I2C_ISR_RXNE            (1U << 2)
#define I2C_ISR_STOPF           (1U << 5)
#define I2C_ISR_NACKF           (1U << 6)
#define I2C_ISR_TC              (1U << 6)
#define I2C_ICR_STOPCF          (1U << 5)

/* ---- USART -------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CR1;      /* 0x00 */
    __IO uint32_t CR2;      /* 0x04 */
    __IO uint32_t CR3;      /* 0x08 */
    __IO uint32_t BRR;      /* 0x0C */
    __IO uint32_t GTDR;     /* 0x10 */
    __IO uint32_t RTOR;     /* 0x14 */
    __IO uint32_t RQR;      /* 0x18 */
    __IO uint32_t ISR;      /* 0x1C */
    __IO uint32_t ICR;      /* 0x20 */
    __IO uint32_t TDR;      /* 0x24 */
    __IO uint32_t RDR;      /* 0x28 */
} usart_t;

#define USART3  ((usart_t *) USART3_BASE)

#define USART_CR1_UE            (1U << 0)
#define USART_CR1_TE            (1U << 3)
#define USART_CR1_RE            (1U << 5)
#define USART_CR1_RXNEIE        (1U << 5)
#define USART_ISR_TXE           (1U << 7)
#define USART_ISR_RXNE          (1U << 5)
#define USART_ISR_TC            (1U << 6)

/* ---- HRTIM (high-resolution timer) --------------------------------------
 * Used to generate the 140 kHz Qi PWM with sub-ns jitter.
 * Only the fields CoilGrip actually touches are modelled. */
typedef struct {
    __IO uint32_t MCR;          /* 0x00 Master control                  */
    __IO uint32_t ISR;          /* 0x04 Interrupt status                */
    __IO uint32_t ICR;          /* 0x08 Interrupt clear                 */
    __IO uint32_t IER;          /* 0x0C Interrupt enable                */
    __IO uint32_t RESERVED0[4];
    __IO uint32_t CNTR;         /* 0x20 Master counter                  */
    __IO uint32_t PERER;        /* 0x28 Master period                   */
    __IO uint32_t REPER;        /* 0x2C Master repetition               */
    __IO uint32_t CMP1ER;       /* 0x30 Master compare 1                */
    __IO uint32_t CMP2ER;       /* 0x34 Master compare 2                */
    __IO uint32_t RESERVED1[6];
    __IO uint32_t TIMx[10][16]; /* per-timer registers (10 timers)    */
} hrtim_t;

#define HRTIM1  ((hrtim_t *) HRTIM1_BASE)

/* HRTIM Master control register bits */
#define HRTIM_MCR_MCEN          (1U << 0)
#define HRTIM_MCR_MDACEN         (1U << 1)
#define HRTIM_MCR_TBDMEN        (1U << 2)

/* ---- ADC ---------------------------------------------------------------- */
typedef struct {
    __IO uint32_t ISR;      /* 0x00 */
    __IO uint32_t IER;      /* 0x04 */
    __IO uint32_t CR;       /* 0x08 */
    __IO uint32_t CFGR;     /* 0x0C */
    __IO uint32_t RESERVED0;
    __IO uint32_t SMPR1;    /* 0x14 */
    __IO uint32_t SMPR2;    /* 0x18 */
    __IO uint32_t RESERVED1[2];
    __IO uint32_t OFR1;     /* 0x20 */
    __IO uint32_t RESERVED2[18];
    __IO uint32_t JDR1;     /* 0x68 injected data 1 */
    __IO uint32_t JDR2;     /* 0x6C */
    __IO uint32_t JDR3;     /* 0x70 */
    __IO uint32_t JDR4;     /* 0x74 */
    __IO uint32_t DR;       /* 0x78 regular data    */
} adc_t;

#define ADC1    ((adc_t *) ADC1_BASE)

#define ADC_CR_ADEN             (1U << 0)
#define ADC_CR_ADSTART          (1U << 2)
#define ADC_CR_ADCAL            (1U << 16)
#define ADC_ISR_ADRDY           (1U << 0)

/* ---- TIM (general purpose, for glitch period timing) -------------------- */
typedef struct {
    __IO uint32_t CR1;      /* 0x00 */
    __IO uint32_t CR2;      /* 0x04 */
    __IO uint32_t SMCR;     /* 0x08 */
    __IO uint32_t DIER;     /* 0x0C */
    __IO uint32_t SR;       /* 0x10 */
    __IO uint32_t EGR;      /* 0x14 */
    __IO uint32_t CCMR1;    /* 0x18 */
    __IO uint32_t CCMR2;    /* 0x1C */
    __IO uint32_t CCER;     /* 0x20 */
    __IO uint32_t CNT;      /* 0x24 */
    __IO uint32_t PSC;      /* 0x28 */
    __IO uint32_t ARR;      /* 0x2C */
} tim_t;

#define TIM2    ((tim_t *) TIM2_BASE)
#define TIM3    ((tim_t *) TIM3_BASE)

#define TIM_CR1_CEN             (1U << 0)
#define TIM_DIER_UIE           (1U << 0)
#define TIM_SR_UIF              (1U << 0)

/* ---- DMA (channel 0 of DMA1, used for SPI1 to iCE40) -------------------- */
typedef struct {
    __IO uint32_t PAR;      /* 0x00 peripheral address            */
    __IO uint32_t MAR;      /* 0x04 memory address                */
    __IO uint32_t NDTR;     /* 0x08 count                         */
    __IO uint32_t CR;      /* 0x0C config                        */
    __IO uint32_t FCR;     /* 0x10 FIFO control                  */
} dma_str_t;

#define DMA1_STR0   ((dma_str_t *) DMA1_STR0_BASE)

#define DMA_CR_EN               (1U << 0)
#define DMA_CR_TCIE             (1U << 4)
#define DMA_CR_MINC             (1U << 10)
#define DMA_CR_PINC             (1U << 9)
#define DMA_CR_DIR_P2M          (0U << 6)
#define DMA_CR_DIR_M2P          (1U << 6)
#define DMA_CR_CHSEL(n)         (((uint32_t)(n)) << 25)

/* ---- DMAMUX ------------------------------------------------------------- */
typedef struct {
    __IO uint32_t C0CR;     /* 0x00 request generator 0        */
    __IO uint32_t C1CR;     /* 0x04                            */
    __IO uint32_t C2CR;     /* 0x08                            */
    __IO uint32_t C3CR;     /* 0x0C                            */
    __IO uint32_t RESERVED0[12];
    __IO uint32_t RG0CR;    /* 0x40                            */
} dmamux_t;

#define DMAMUX1 ((dmamux_t *) DMAMUX1_BASE)

/* ---- Helper macros ------------------------------------------------------ */
#define REG32(addr)            (*(volatile uint32_t *)(addr))
#define BIT(n)                  (1UL << (n))
#define ARRAY_SIZE(a)           (sizeof(a) / sizeof((a)[0]))
#define CLAMP(x, lo, hi)         ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

#endif /* COILGRIP_REGISTERS_H */