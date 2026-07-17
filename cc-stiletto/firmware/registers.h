/*
 * registers.h — Peripheral register base addresses and bit definitions for the
 * STM32G474 used by CC-Stiletto's bare-metal drivers.
 *
 * Only definitions actually referenced by the firmware are included; the file is
 * intentionally minimal to keep the tool's own attack surface auditable.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef CC_STILETTO_REGISTERS_H
#define CC_STILETTO_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses (from STM32G474 reference manual, section 2.3) --------- */
#define PERIPH_BASE         (0x40000000u)
#define AHB1_BASE           (0x40000000u)
#define AHB2_BASE            (0x48000000u)
#define APB1_BASE            (0x40000000u)
#define APB1P_BASE           (0x40000000u)
#define APB2_BASE            (0x40010000u)

/* System control */
#define RCC_BASE            (APB1_BASE + 0x0000)
#define PWR_BASE            (APB1_BASE + 0x7000)
#define FLASH_R_BASE        (AHB1_BASE + 0x2000)
#define SYSCFG_BASE         (APB2_BASE + 0x0000)

/* GPIO banks (AHB2) */
#define GPIOA_BASE          (AHB2_BASE + 0x0000)
#define GPIOB_BASE          (AHB2_BASE + 0x0400)
#define GPIOC_BASE          (AHB2_BASE + 0x0800)
#define GPIOD_BASE          (AHB2_BASE + 0x0C00)
#define GPIOF_BASE          (AHB2_BASE + 0x1400)

/* DMA / DMAMUX */
#define DMA1_BASE           (AHB1_BASE + 0x6000)
#define DMAMUX1_BASE        (AHB1_BASE + 0x7C00)

/* Timers */
#define TIM1_BASE           (APB2_BASE + 0x2C00)
#define TIM2_BASE           (APB1_BASE + 0x0000)
#define TIM3_BASE           (APB1_BASE + 0x0400)
#define TIM6_BASE           (APB1_BASE + 0x1000)
#define TIM16_BASE          (APB2_BASE + 0x4400)

/* HRTIM (high-resolution timer) — Advanced Timer on APB2 */
#define HRTIM1_BASE         (APB2_BASE + 0x6800)
#define HRTIM_TIMA_OFFSET   0x20u
#define HRTIM_TIMB_OFFSET   0x60u
#define HRTIM_COMMON_OFFSET 0x3C0u

/* I2C peripherals */
#define I2C1_BASE           (APB1_BASE + 0x5400)
#define I2C2_BASE           (APB1_BASE + 0x5800)
#define I2C3_BASE            (APB1_BASE + 0x5C00)
#define I2C4_BASE            (APB1P_BASE + 0x8400)

/* USART */
#define USART1_BASE         (APB2_BASE + 0x3800)
#define USART2_BASE         (APB1_BASE + 0x4400)
#define USART3_BASE         (APB1_BASE + 0x4800)

/* ADC */
#define ADC1_BASE           (AHB1_BASE + 0x5000)
#define ADC2_BASE           (AHB1_BASE + 0x5400)
#define ADC12_COMMON        (AHB1_BASE + 0x5800)

/* USB device */
#define USB_BASE            (APB1_BASE + 0x5C00 + 0x0000)
#define USB_RAM_BASE        (0x40006000u)

/* CMP (comparators) */
#define COMP1_BASE          (AHB2_BASE + 0x1800)
#define COMP2_BASE          (AHB2_BASE + 0x1804)
#define COMP12_COMMON       (AHB2_BASE + 0x1800)

/* ---- Peripheral type cast macros ------------------------------------------ */
#define RCC                 ((rcc_reg_t *) RCC_BASE)
#define PWR                 ((pwr_reg_t *) PWR_BASE)
#define FLASH_R             ((flash_reg_t *) FLASH_R_BASE)
#define SYSCFG              ((syscfg_reg_t *) SYSCFG_BASE)

#define GPIOA               ((gpio_reg_t *) GPIOA_BASE)
#define GPIOB               ((gpio_reg_t *) GPIOB_BASE)
#define GPIOC               ((gpio_reg_t *) GPIOC_BASE)
#define GPIOD               ((gpio_reg_t *) GPIOD_BASE)

#define TIM1                ((tim_reg_t *) TIM1_BASE)
#define TIM2                ((tim_reg_t *) TIM2_BASE)
#define TIM6                ((tim_reg_t *) TIM6_BASE)
#define HRTIM1              ((hrtim_reg_t *) HRTIM1_BASE)

#define I2C1                ((i2c_reg_t *) I2C1_BASE)
#define I2C2                ((i2c_reg_t *) I2C2_BASE)
#define I2C3                ((i2c_reg_t *) I2C3_BASE)

#define USART1              ((usart_reg_t *) USART1_BASE)
#define USART2              ((usart_reg_t *) USART2_BASE)

#define ADC1                ((adc_reg_t *) ADC1_BASE)

/* ---- Register layout typedefs (minimal, only fields used) ----------------- */

typedef struct {
    volatile uint32_t CR;        /* 0x00 */
    volatile uint32_t CFGR;      /* 0x08 */
    volatile uint32_t CIR;       /* 0x0C */
    volatile uint32_t APB1RSTR;  /* 0x10 */
    volatile uint32_t APB2RSTR;  /* 0x14 */
    volatile uint32_t AHB1RSTR;  /* 0x18 */
    volatile uint32_t AHB2RSTR;  /* 0x1C */
    volatile uint32_t APB1ENR;   /* 0x40 */
    volatile uint32_t APB2ENR;   /* 0x44 */
    volatile uint32_t AHB1ENR;   /* 0x48 */
    volatile uint32_t AHB2ENR;   /* 0x4C */
    volatile uint32_t APB1ENR1;  /* reserved alignment */
} rcc_reg_t;

typedef struct {
    volatile uint32_t MODER;     /* 0x00 */
    volatile uint32_t OTYPER;    /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;     /* 0x0C */
    volatile uint32_t IDR;       /* 0x10 */
    volatile uint32_t ODR;       /* 0x14 */
    volatile uint32_t BSRR;       /* 0x18 — set/reset atomic */
    volatile uint32_t LCKR;      /* 0x1C */
    volatile uint32_t AFRL;      /* 0x20 */
    volatile uint32_t AFRH;       /* 0x24 */
    volatile uint32_t BRR;       /* 0x28 — reset atomic */
} gpio_reg_t;

typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t OAR1;        /* 0x08 */
    volatile uint32_t OAR2;       /* 0x0C */
    volatile uint32_t TIMINGR;    /* 0x10 */
    volatile uint32_t TIMEOUNR;   /* 0x14 */
    volatile uint32_t ISR;        /* 0x18 — interrupt & status */
    volatile uint32_t ICR;        /* 0x1C — interrupt clear */
    volatile uint32_t PECR;       /* 0x20 */
    volatile uint32_t RXDR;       /* 0x24 */
    volatile uint32_t TXDR;       /* 0x28 */
} i2c_reg_t;

/* I2C ISR bit definitions */
#define I2C_ISR_TXE     (1u << 0)
#define I2C_ISR_TXIS    (1u << 1)
#define I2C_ISR_RXNE    (1u << 2)
#define I2C_ISR_ADDR    (1u << 3)
#define I2C_ISR_NACKF   (1u << 4)
#define I2C_ISR_STOPF   (1u << 5)
#define I2C_ISR_TC      (1u << 6)
#define I2C_ISR_TCR     (1u << 7)
#define I2C_ISR_BUSY    (1u << 15)

/* I2C CR1 bits */
#define I2C_CR1_PE      (1u << 0)   /* peripheral enable */
#define I2C_CR1_TXIE    (1u << 1)
#define I2C_CR1_RXIE    (1u << 2)
#define I2C_CR1_ADDRIE  (1u << 3)
#define I2C_CR1_NACKIE  (1u << 4)
#define I2C_CR1_STOPIE  (1u << 5)
#define I2C_CR1_TCIE    (1u << 6)
#define I2C_CR1_ANFOFF  (1u << 12)

/* I2C CR2 bits */
#define I2C_CR2_START   (1u << 13)
#define I2C_CR2_STOP    (1u << 14)
#define I2C_CR2_NACK    (1u << 15)
#define I2C_CR2_RELOAD  (1u << 24)
#define I2C_CR2_AUTOEND (1u << 25)

typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t SMCR;  /* 0x08 */
    volatile uint32_t DIER;  /* 0x0C */
    volatile uint32_t SR;    /* 0x10 */
    volatile uint32_t EGR;   /* 0x14 */
    volatile uint32_t CCMR1; /* 0x18 */
    volatile uint32_t CCMR2; /* 0x1C */
    volatile uint32_t CCER;  /* 0x20 */
    volatile uint32_t CNT;   /* 0x24 */
    volatile uint32_t PSC;   /* 0x28 */
    volatile uint32_t ARR;   /* 0x2C */
    volatile uint32_t CCR1;  /* 0x34 */
    volatile uint32_t CCR2;  /* 0x38 */
    volatile uint32_t CCR3;  /* 0x3C */
    volatile uint32_t CCR4;  /* 0x40 */
} tim_reg_t;

typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CR3;      /* 0x08 */
    volatile uint32_t BRR;      /* 0x0C */
    volatile uint32_t GTPR;     /* 0x10 */
    volatile uint32_t RTOR;     /* 0x14 */
    volatile uint32_t RQR;      /* 0x18 */
    volatile uint32_t ISR;      /* 0x1C */
    volatile uint32_t ICR;      /* 0x20 */
    volatile uint32_t RDR;      /* 0x24 */
    volatile uint32_t TDR;      /* 0x28 */
} usart_reg_t;

typedef struct {
    volatile uint32_t ISR;      /* 0x00 */
    volatile uint32_t IER;      /* 0x04 */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t CFGR;      /* 0x0C */
    volatile uint32_t SQR1;      /* 0x10 */
    volatile uint32_t SQR2;      /* 0x14 */
    volatile uint32_t DR;        /* 0x40 */
} adc_reg_t;

typedef struct {
    volatile uint32_t MCR;        /* 0x00 master control */
    volatile uint32_t ISR;        /* 0x04 master ISR */
    volatile uint32_t IER;        /* 0x08 master IER */
    volatile uint32_t CR1;        /* 0x0C common control 1 */
    volatile uint32_t CR2;        /* 0x10 common control 2 */
    volatile uint32_t DLLCR;      /* 0x14 DLL */
    volatile uint8_t  _pad[0x20u - 0x18u];
    volatile uint32_t TIMA_CR;    /* 0x20 Timer A control */
    volatile uint32_t TIMA_ISR;   /* 0x24 */
    volatile uint32_t TIMA_IER;   /* 0x28 */
    volatile uint32_t TIMA_CMP1;  /* 0x2C compare 1 */
    volatile uint32_t TIMA_CMP2;  /* 0x30 compare 2 */
    volatile uint32_t TIMA_CMP3;  /* 0x34 compare 3 */
    volatile uint32_t TIMA_CNT;   /* 0x38 counter */
    volatile uint32_t TIMA_PER;   /* 0x3C period */
    volatile uint32_t TIMA_SET;    /* 0x40 set */
    volatile uint32_t TIMA_RST;    /* 0x44 reset */
} hrtim_reg_t;

/* ---- RCC enable bits used by this project ---------------------------------- */
#define RCC_AHB2ENR_GPIOAEN   (1u << 0)
#define RCC_AHB2ENR_GPIOBEN   (1u << 1)
#define RCC_AHB2ENR_GPIOCEN   (1u << 2)
#define RCC_AHB2ENR_GPIOFEN   (5u)
#define RCC_APB1ENR_I2C1EN    (1u << 21)
#define RCC_APB1ENR_I2C2EN    (1u << 22)
#define RCC_APB1ENR_I2C3EN    (1u << 23)
#define RCC_APB1ENR_TIM2EN    (1u << 0)
#define RCC_APB1ENR_TIM6EN    (1u << 4)
#define RCC_APB1ENR_USBEN     (1u << 23)
#define RCC_APB2ENR_TIM1EN    (1u << 11)
#define RCC_APB2ENR_USART1EN (1u << 14)
#define RCC_APB2ENR_HRTIM1EN  (1u << 20)
#define RCC_AHB1ENR_DMA1EN    (1u << 0)
#define RCC_AHB2ENR_ADC12EN   (1u << 28)

/* ---- GPIO mode definitions ------------------------------------------------- */
#define GPIO_MODE_INPUT       0u
#define GPIO_MODE_OUTPUT      1u
#define GPIO_MODE_AF          2u
#define GPIO_MODE_ANALOG      3u

#define GPIO_OTYPE_PP         0u  /* push-pull */
#define GPIO_OTYPE_OD         1u  /* open-drain */
#define GPIO_PUPD_NONE        0u
#define GPIO_PUPD_PU          1u
#define GPIO_PUPD_PD          2u
#define GPIO_SPEED_LOW        0u
#define GPIO_SPEED_HIGH       3u

#endif /* CC_STILETTO_REGISTERS_H */