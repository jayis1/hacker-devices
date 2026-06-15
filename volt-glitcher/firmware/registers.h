/*
 * registers.h — STM32F407VGT6 register definitions for Volt Glitcher
 * Direct register access for performance-critical glitch timing paths
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * ARM Cortex-M4F Core Peripherals
 * ======================================================================== */

#define SCS_BASE            0xE000E000UL
#define SCB_BASE            (SCS_BASE + 0x0D00UL)
#define NVIC_BASE           (SCS_BASE + 0x0100UL)   /* 0xE000E100 */
#define SCB_SHP_BASE        (SCS_BASE + 0x0D18UL)
#define SYSTICK_BASE        (SCS_BASE + 0x0010UL)   /* 0xE000E010 */

/* SCB registers */
#define SCB_CPUID           (*((volatile uint32_t *)(SCB_BASE + 0x00)))
#define SCB_ICSR            (*((volatile uint32_t *)(SCB_BASE + 0x04)))
#define SCB_VTOR            (*((volatile uint32_t *)(SCB_BASE + 0x08)))
#define SCB_AIRCR           (*((volatile uint32_t *)(SCB_BASE + 0x0C)))
#define SCB_SCR             (*((volatile uint32_t *)(SCB_BASE + 0x10)))
#define SCB_CCR             (*((volatile uint32_t *)(SCB_BASE + 0x14)))
#define SCB_SHPR1           (*((volatile uint32_t *)(SCB_BASE + 0x18)))
#define SCB_SHPR2           (*((volatile uint32_t *)(SCB_BASE + 0x1C)))
#define SCB_SHPR3           (*((volatile uint32_t *)(SCB_BASE + 0x20)))

/* SysTick registers */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_Type;
#define SYSTICK              ((SysTick_Type *)SYSTICK_BASE)

/* SysTick CTRL bits */
#define SYSTICK_CTRL_ENABLE      (1UL << 0)
#define SYSTICK_CTRL_TICKINT     (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE   (1UL << 2)
#define SYSTICK_CTRL_COUNTFLAG   (1UL << 16)

/* NVIC registers */
typedef struct {
    volatile uint32_t ISER[8];      /* Interrupt Set Enable */
    volatile uint32_t ICER[8];      /* Interrupt Clear Enable */
    volatile uint32_t ISPR[8];      /* Interrupt Set Pending */
    volatile uint32_t ICPR[8];      /* Interrupt Clear Pending */
    volatile uint32_t IABR[8];      /* Interrupt Active Bit */
    volatile uint8_t  IP[240];      /* Interrupt Priority */
    volatile uint32_t STIR;         /* Software Trigger */
} NVIC_Type;
#define NVIC                 ((NVIC_Type *)NVIC_BASE)

/* ========================================================================
 * STM32F407 Peripheral Base Addresses
 * ======================================================================== */

#define PERIPH_BASE          0x40000000UL
#define APB1PERIPH_BASE      PERIPH_BASE
#define APB2PERIPH_BASE      (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE      (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE      (PERIPH_BASE + 0x10000000UL)

/* ========================================================================
 * GPIO Registers (AHB1)
 * ======================================================================== */

#define GPIOA_BASE           (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE           (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE           (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE           (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE           (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOH_BASE           (AHB1PERIPH_BASE + 0x1400UL)

typedef struct {
    volatile uint32_t MODER;       /* Port mode register */
    volatile uint32_t OTYPER;      /* Output type register */
    volatile uint32_t OSPEEDR;     /* Output speed register */
    volatile uint32_t PUPDR;       /* Pull-up/pull-down register */
    volatile uint32_t IDR;         /* Input data register */
    volatile uint32_t ODR;        /* Output data register */
    volatile uint32_t BSRR;       /* Bit set/reset register */
    volatile uint32_t LCKR;       /* Configuration lock register */
    volatile uint32_t AFR[2];     /* Alternate function registers [0]=AFRL [1]=AFRH */
} GPIO_TypeDef;

#define GPIOA                ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB                ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC                ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD                ((GPIO_TypeDef *)GPIOD_BASE)

/* GPIO MODER bits (2 bits per pin) */
#define GPIO_MODER_INPUT      0x0UL
#define GPIO_MODER_OUTPUT     0x1UL
#define GPIO_MODER_AF         0x2UL
#define GPIO_MODER_ANALOG     0x3UL

/* GPIO OSPEEDR bits */
#define GPIO_OSPEEDR_LOW      0x0UL
#define GPIO_OSPEEDR_MED      0x1UL
#define GPIO_OSPEEDR_HIGH     0x2UL
#define GPIO_OSPEEDR_VHIGH    0x3UL

/* GPIO PUPDR bits */
#define GPIO_PUPDR_NONE       0x0UL
#define GPIO_PUPDR_PULLUP     0x1UL
#define GPIO_PUPDR_PULLDOWN   0x2UL

/* GPIO BSRR encoding */
#define GPIO_BSRR_SET(n)      (1UL << (n))
#define GPIO_BSRR_RESET(n)   (1UL << ((n) + 16))

/* ========================================================================
 * RCC Registers (AHB1)
 * ======================================================================== */

#define RCC_BASE             (AHB1PERIPH_BASE + 0x3800UL)

typedef struct {
    volatile uint32_t CR;           /* Clock control */
    volatile uint32_t PLLCFGR;      /* PLL configuration */
    volatile uint32_t CFGR;         /* Clock configuration */
    volatile uint32_t CIR;          /* Clock interrupt */
    volatile uint32_t AHB1RSTR;     /* AHB1 peripheral reset */
    volatile uint32_t AHB2RSTR;     /* AHB2 peripheral reset */
    volatile uint32_t AHB3RSTR;     /* AHB3 peripheral reset */
    volatile uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;     /* APB1 peripheral reset */
    volatile uint32_t APB2RSTR;     /* APB2 peripheral reset */
    volatile uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;      /* AHB1 peripheral clock enable */
    volatile uint32_t AHB2ENR;      /* AHB2 peripheral clock enable */
    volatile uint32_t AHB3ENR;      /* AHB3 peripheral clock enable */
    volatile uint32_t RESERVED2;
    volatile uint32_t APB1ENR;      /* APB1 peripheral clock enable */
    volatile uint32_t APB2ENR;      /* APB2 peripheral clock enable */
    volatile uint32_t RESERVED3[2];
    volatile uint32_t AHB1LPENR;    /* AHB1 LP clock enable */
    volatile uint32_t AHB2LPENR;    /* AHB2 LP clock enable */
    volatile uint32_t AHB3LPENR;    /* AHB3 LP clock enable */
    volatile uint32_t RESERVED4;
    volatile uint32_t APB1LPENR;    /* APB1 LP clock enable */
    volatile uint32_t APB2LPENR;    /* APB2 LP clock enable */
    volatile uint32_t RESERVED5[2];
    volatile uint32_t BDCR;         /* Backup domain control */
    volatile uint32_t CSR;          /* Clock control & status */
    volatile uint32_t RESERVED6[2];
    volatile uint32_t SSCGR;       /* Spread spectrum clock */
    volatile uint32_t PLLI2SCFGR;  /* PLLI2S configuration */
    volatile uint32_t PLLSAICFGR;  /* PLLSAI configuration */
    volatile uint32_t DCKCFGR;      /* Dedicated clocks config */
} RCC_TypeDef;

#define RCC                  ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION         (1UL << 0)
#define RCC_CR_HSEON         (1UL << 16)
#define RCC_CR_HSIRDY        (1UL << 1)
#define RCC_CR_HSERDY        (1UL << 17)
#define RCC_CR_PLLON         (1UL << 24)
#define RCC_CR_PLLRDY        (1UL << 25)

/* RCC AHB1ENR bits — peripheral clock enables */
#define RCC_AHB1ENR_GPIOAEN    (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN    (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN    (1UL << 2)
#define RCC_AHB1ENR_GPIODEN    (1UL << 3)
#define RCC_AHB1ENR_CRCEN      (1UL << 12)
#define RCC_AHB1ENR_DMA1EN     (1UL << 21)
#define RCC_AHB1ENR_DMA2EN     (1UL << 22)

/* RCC APB1ENR bits */
#define RCC_APB1ENR_TIM2EN     (1UL << 0)
#define RCC_APB1ENR_TIM3EN     (1UL << 1)
#define RCC_APB1ENR_TIM4EN     (1UL << 2)
#define RCC_APB1ENR_USART2EN   (1UL << 17)
#define RCC_APB1ENR_I2C1EN     (1UL << 21)
#define RCC_APB1ENR_SPI3EN     (1UL << 15)
#define RCC_APB1ENR_PWREN      (1UL << 28)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_TIM1EN     (1UL << 0)
#define RCC_APB2ENR_USART1EN   (1UL << 4)
#define RCC_APB2ENR_SPI1EN     (1UL << 12)
#define RCC_APB2ENR_ADC1EN     (1UL << 8)
#define RCC_APB2ENR_SYSCFGEN   (1UL << 14)

/* ========================================================================
 * SPI Registers
 * ======================================================================== */

#define SPI1_BASE            (APB2PERIPH_BASE + 0x3000UL)
#define SPI2_BASE            (APB1PERIPH_BASE + 0x3800UL)
#define SPI3_BASE            (APB1PERIPH_BASE + 0x3C00UL)

typedef struct {
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t SR;         /* Status register */
    volatile uint32_t DR;        /* Data register */
    volatile uint32_t CRCPR;     /* CRC polynomial */
    volatile uint32_t RXCRCR;    /* RX CRC */
    volatile uint32_t TXCRCR;    /* TX CRC */
} SPI_TypeDef;

#define SPI1                 ((SPI_TypeDef *)SPI1_BASE)
#define SPI2                 ((SPI_TypeDef *)SPI2_BASE)

/* SPI CR1 bits */
#define SPI_CR1_CPHA         (1UL << 0)
#define SPI_CR1_CPOL         (1UL << 1)
#define SPI_CR1_MSTR         (1UL << 2)
#define SPI_CR1_BR_DIV2      (0UL << 3)
#define SPI_CR1_BR_DIV4      (1UL << 3)
#define SPI_CR1_BR_DIV8      (2UL << 3)
#define SPI_CR1_BR_DIV16     (3UL << 3)
#define SPI_CR1_BR_DIV32     (4UL << 3)
#define SPI_CR1_BR_DIV64     (5UL << 3)
#define SPI_CR1_BR_DIV128    (6UL << 3)
#define SPI_CR1_BR_DIV256    (7UL << 3)
#define SPI_CR1_SPE          (1UL << 6)
#define SPI_CR1_LSBFIRST     (1UL << 7)
#define SPI_CR1_SSI          (1UL << 8)
#define SPI_CR1_SSM          (1UL << 9)
#define SPI_CR1_RXONLY       (1UL << 10)
#define SPI_CR1_CRCEN        (1UL << 13)
#define SPI_CR1_BIDIMODE     (1UL << 15)

/* SPI SR bits */
#define SPI_SR_RXNE          (1UL << 0)
#define SPI_SR_TXE           (1UL << 1)
#define SPI_SR_BSY           (1UL << 7)

/* ========================================================================
 * USART Registers
 * ======================================================================== */

#define USART1_BASE          (APB2PERIPH_BASE + 0x1000UL)
#define USART2_BASE          (APB1PERIPH_BASE + 0x4400UL)

typedef struct {
    volatile uint32_t SR;         /* Status register */
    volatile uint32_t DR;         /* Data register */
    volatile uint32_t BRR;        /* Baud rate register */
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t CR3;        /* Control register 3 */
} USART_TypeDef;

#define USART1               ((USART_TypeDef *)USART1_BASE)
#define USART2               ((USART_TypeDef *)USART2_BASE)

/* USART CR1 bits */
#define USART_CR1_UE         (1UL << 13)
#define USART_CR1_TE         (1UL << 3)
#define USART_CR1_RE         (1UL << 2)
#define USART_CR1_RXNEIE     (1UL << 5)

/* USART SR bits */
#define USART_SR_TXE         (1UL << 7)
#define USART_SR_RXNE        (1UL << 5)
#define USART_SR_TC          (1UL << 6)

/* ========================================================================
 * Timer Registers
 * ======================================================================== */

#define TIM1_BASE            (APB2PERIPH_BASE + 0x0000UL)
#define TIM2_BASE            (APB1PERIPH_BASE + 0x0000UL)
#define TIM3_BASE            (APB1PERIPH_BASE + 0x0400UL)
#define TIM4_BASE            (APB1PERIPH_BASE + 0x0800UL)

typedef struct {
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t SMCR;      /* Slave mode control */
    volatile uint32_t DIER;      /* DMA/interrupt enable */
    volatile uint32_t SR;        /* Status register */
    volatile uint32_t EGR;      /* Event generation */
    volatile uint32_t CCMR1;    /* Capture/compare mode 1 */
    volatile uint32_t CCMR2;    /* Capture/compare mode 2 */
    volatile uint32_t CCER;     /* Capture/compare enable */
    volatile uint32_t CNT;      /* Counter */
    volatile uint32_t PSC;      /* Prescaler */
    volatile uint32_t ARR;      /* Auto-reload */
    volatile uint32_t RCR;      /* Repetition counter (TIM1 only) */
    volatile uint32_t CCR1;     /* Capture/compare 1 */
    volatile uint32_t CCR2;     /* Capture/compare 2 */
    volatile uint32_t CCR3;     /* Capture/compare 3 */
    volatile uint32_t CCR4;     /* Capture/compare 4 */
    volatile uint32_t BDTR;     /* Break and dead-time (TIM1 only) */
    volatile uint32_t DCR;      /* DMA control */
    volatile uint32_t DMAR;     /* DMA address for burst */
} TIM_TypeDef;

#define TIM1                 ((TIM_TypeDef *)TIM1_BASE)
#define TIM2                 ((TIM_TypeDef *)TIM2_BASE)
#define TIM3                 ((TIM_TypeDef *)TIM3_BASE)
#define TIM4                 ((TIM_TypeDef *)TIM4_BASE)

/* TIM CR1 bits */
#define TIM_CR1_CEN          (1UL << 0)
#define TIM_CR1_UDIS          (1UL << 1)
#define TIM_CR1_URS          (1UL << 2)
#define TIM_CR1_OPM          (1UL << 3)
#define TIM_CR1_DIR          (1UL << 4)
#define TIM_CR1_CMS_MASK     (3UL << 5)

/* TIM DIER bits */
#define TIM_DIER_UIE         (1UL << 0)
#define TIM_DIER_CC1IE       (1UL << 1)
#define TIM_DIER_CC2IE       (1UL << 2)
#define TIM_DIER_CC3IE       (1UL << 3)
#define TIM_DIER_CC4IE       (1UL << 4)
#define TIM_DIER_TIE         (1UL << 6)

/* TIM SR bits */
#define TIM_SR_UIF           (1UL << 0)
#define TIM_SR_CC1IF         (1UL << 1)
#define TIM_SR_CC2IF         (1UL << 2)
#define TIM_SR_CC3IF         (1UL << 3)
#define TIM_SR_CC4IF         (1UL << 4)

/* TIM CCER bits */
#define TIM_CCER_CC1E        (1UL << 0)
#define TIM_CCER_CC1P        (1UL << 1)
#define TIM_CCER_CC2E        (1UL << 4)
#define TIM_CCER_CC3E        (1UL << 8)
#define TIM_CCER_CC4E        (1UL << 12)

/* TIM CCMR1 — Output Compare mode for CH1 */
#define TIM_CCMR1_OC1M_MASK  (7UL << 4)
#define TIM_CCMR1_OC1M_FROZEN   (0UL << 4)
#define TIM_CCMR1_OC1M_ACTIVE   (1UL << 4)
#define TIM_CCMR1_OC1M_INACTIVE (2UL << 4)
#define TIM_CCMR1_OC1M_TOGGLE    (3UL << 4)
#define TIM_CCMR1_OC1M_PWM1     (6UL << 4)
#define TIM_CCMR1_OC1M_PWM2     (7UL << 4)
#define TIM_CCMR1_OC1PE        (1UL << 3)

/* ========================================================================
 * ADC Registers
 * ======================================================================== */

#define ADC1_BASE            (APB2PERIPH_BASE + 0x2000UL)

typedef struct {
    volatile uint32_t SR;         /* Status register */
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t SMPR1;      /* Sample time register 1 */
    volatile uint32_t SMPR2;      /* Sample time register 2 */
    volatile uint32_t JOFR1;     /* Injected channel offset 1 */
    volatile uint32_t JOFR2;     /* Injected channel offset 2 */
    volatile uint32_t JOFR3;     /* Injected channel offset 3 */
    volatile uint32_t JOFR4;     /* Injected channel offset 4 */
    volatile uint32_t HTR;       /* Watchdog high threshold */
    volatile uint32_t LTR;       /* Watchdog low threshold */
    volatile uint32_t SQR1;      /* Regular sequence register 1 */
    volatile uint32_t SQR2;      /* Regular sequence register 2 */
    volatile uint32_t SQR3;      /* Regular sequence register 3 */
    volatile uint32_t JSQR;      /* Injected sequence */
    volatile uint32_t JDR1;      /* Injected data 1 */
    volatile uint32_t JDR2;      /* Injected data 2 */
    volatile uint32_t JDR3;      /* Injected data 3 */
    volatile uint32_t JDR4;      /* Injected data 4 */
    volatile uint32_t DR;        /* Regular data */
} ADC_TypeDef;

#define ADC1                 ((ADC_TypeDef *)ADC1_BASE)

/* ADC CR1 bits */
#define ADC_CR1_RES_MASK     (3UL << 24)
#define ADC_CR1_RES_12BIT    (0UL << 24)
#define ADC_CR1_RES_10BIT    (1UL << 24)
#define ADC_CR1_RES_8BIT     (2UL << 24)
#define ADC_CR1_RES_6BIT     (3UL << 24)
#define ADC_CR1_EOCIE        (1UL << 5)
#define ADC_CR1_AWDIE        (1UL << 6)
#define ADC_CR1_AWDCH_MASK   (0x1FUL << 0)

/* ADC CR2 bits */
#define ADC_CR2_ADON         (1UL << 0)
#define ADC_CR2_CONT         (1UL << 1)
#define ADC_CR2_SWSTART     (1UL << 30)
#define ADC_CR2_EXTEN_MASK   (3UL << 28)
#define ADC_CR2_EXTEN_RISING (1UL << 28)

/* ADC SR bits */
#define ADC_SR_EOC           (1UL << 1)
#define ADC_SR_AWD           (1UL << 0)
#define ADC_SR_STRT          (1UL << 4)

/* ADC sample time bits (per channel in SMPR2 for ch 0-9) */
#define ADC_SMPR2_SMP0_MASK   (7UL << 0)
#define ADC_SMPR2_SMP_3CYC   (0UL << 0)
#define ADC_SMPR2_SMP_15CYC  (1UL << 0)
#define ADC_SMPR2_SMP_28CYC  (2UL << 0)
#define ADC_SMPR2_SMP_56CYC  (3UL << 0)
#define ADC_SMPR2_SMP_84CYC  (4UL << 0)
#define ADC_SMPR2_SMP_112CYC (5UL << 0)
#define ADC_SMPR2_SMP_144CYC (6UL << 0)
#define ADC_SMPR2_SMP_480CYC (7UL << 0)

/* ========================================================================
 * DMA Registers
 * ======================================================================== */

#define DMA1_BASE            (AHB1PERIPH_BASE + 0x6000UL)
#define DMA2_BASE            (AHB1PERIPH_BASE + 0x6400UL)

typedef struct {
    volatile uint32_t ISR;        /* Interrupt status (shared) */
    volatile uint32_t RESERVED0;
    volatile uint32_t IFCR;       /* Interrupt flag clear (shared) */
    volatile uint32_t RESERVED1;
    struct {
        volatile uint32_t PAR;    /* Peripheral address */
        volatile uint32_t MAR;    /* Memory address */
        volatile uint32_t NDTR;   /* Number of data items */
        volatile uint32_t CR;     /* Control */
        volatile uint32_t FCR;    /* FIFO control */
    } STREAM[8];
} DMA_TypeDef;

#define DMA1                 ((DMA_TypeDef *)DMA1_BASE)
#define DMA2                 ((DMA_TypeDef *)DMA2_BASE)

/* DMA Stream CR bits */
#define DMA_SxCR_EN          (1UL << 0)
#define DMA_SxCR_TCIE        (1UL << 4)
#define DMA_SxCR_HTIE        (1UL << 3)
#define DMA_SxCR_TEIE        (1UL << 2)
#define DMA_SxCR_DIR_MASK    (3UL << 6)
#define DMA_SxCR_DIR_P2M     (0UL << 6)
#define DMA_SxCR_DIR_M2P     (1UL << 6)
#define DMA_SxCR_DIR_M2M     (2UL << 6)
#define DMA_SxCR_MINC        (1UL << 10)
#define DMA_SxCR_CIRC        (1UL << 8)
#define DMA_SxCR_PL_MASK     (3UL << 16)
#define DMA_SxCR_PL_HIGH     (3UL << 16)

/* ========================================================================
 * I2C Registers
 * ======================================================================== */

#define I2C1_BASE            (APB1PERIPH_BASE + 0x5400UL)

typedef struct {
    volatile uint32_t CR1;        /* Control register 1 */
    volatile uint32_t CR2;        /* Control register 2 */
    volatile uint32_t OAR1;      /* Own address 1 */
    volatile uint32_t OAR2;      /* Own address 2 */
    volatile uint32_t DR;         /* Data register */
    volatile uint32_t SR1;       /* Status register 1 */
    volatile uint32_t SR2;       /* Status register 2 */
    volatile uint32_t CCR;       /* Clock control register */
    volatile uint32_t TRISE;     /* TRISE register */
} I2C_TypeDef;

#define I2C1                 ((I2C_TypeDef *)I2C1_BASE)

/* I2C CR1 bits */
#define I2C_CR1_PE           (1UL << 0)
#define I2C_CR1_START        (1UL << 8)
#define I2C_CR1_STOP         (1UL << 9)
#define I2C_CR1_ACK          (1UL << 10)

/* I2C SR1 bits */
#define I2C_SR1_SB           (1UL << 0)
#define I2C_SR1_ADDR         (1UL << 1)
#define I2C_SR1_RXNE         (1UL << 6)
#define I2C_SR1_TXE          (1UL << 7)
#define I2C_SR1_BTF          (1UL << 2)

/* ========================================================================
 * EXTI Registers
 * ======================================================================== */

#define EXTI_BASE            (APB2PERIPH_BASE + 0x3C00UL)

typedef struct {
    volatile uint32_t IMR;        /* Interrupt mask */
    volatile uint32_t EMR;       /* Event mask */
    volatile uint32_t RTSR;      /* Rising trigger selection */
    volatile uint32_t FTSR;      /* Falling trigger selection */
    volatile uint32_t SWIER;     /* Software interrupt event */
    volatile uint32_t PR;         /* Pending register */
} EXTI_TypeDef;

#define EXTI                 ((EXTI_TypeDef *)EXTI_BASE)

/* ========================================================================
 * SYSCFG Registers
 * ======================================================================== */

#define SYSCFG_BASE          (APB2PERIPH_BASE + 0x3800UL)

typedef struct {
    volatile uint32_t MEMRMP;
    volatile uint32_t PMC;
    volatile uint32_t EXTICR[4];
    volatile uint32_t CMPCR;
} SYSCFG_TypeDef;

#define SYSCFG               ((SYSCFG_TypeDef *)SYSCFG_BASE)

/* ========================================================================
 * FLASH Registers
 * ======================================================================== */

#define FLASH_BASE           0x40023C00UL

typedef struct {
    volatile uint32_t ACR;        /* Access control */
    volatile uint32_t KEYR;      /* Key */
    volatile uint32_t OPTKEYR;   /* Option key */
    volatile uint32_t SR;        /* Status */
    volatile uint32_t CR;        /* Control */
    volatile uint32_t OPTCR;    /* Option control */
} FLASH_TypeDef;

#define FLASH                ((FLASH_TypeDef *)FLASH_BASE)

#define FLASH_ACR_LATENCY_MASK  (0x7UL << 0)
#define FLASH_ACR_PRFTEN        (1UL << 8)
#define FLASH_ACR_ICEN          (1UL << 9)
#define FLASH_ACR_DCEN          (1UL << 10)

/* ========================================================================
 * PWR Registers
 * ======================================================================== */

#define PWR_BASE             (APB1PERIPH_BASE + 0x7000UL)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CSR;
} PWR_TypeDef;

#define PWR                  ((PWR_TypeDef *)PWR_BASE)

#define PWR_CR_VOS_MASK      (3UL << 14)
#define PWR_CR_VOS_SCALE1    (3UL << 14)
#define PWR_CR_VOS_SCALE2    (2UL << 14)
#define PWR_CR_VOS_SCALE3    (1UL << 14)

/* ========================================================================
 * Interrupt Numbers (STM32F407-specific subset)
 * ======================================================================== */

#define IRQn_NON_NEGATIVE   0

typedef enum {
    /* Cortex-M4 core */
    NonMaskableInt_IRQn   = -14,
    HardFault_IRQn        = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn         = -11,
    UsageFault_IRQn       = -10,
    SVCall_IRQn           = -5,
    DebugMonitor_IRQn     = -4,
    PendSV_IRQn           = -2,
    SysTick_IRQn          = -1,

    /* STM32F407 external */
    WWDG_IRQn             = 0,
    PVD_IRQn              = 1,
    TAMP_STAMP_IRQn       = 2,
    RTC_WKUP_IRQn         = 3,
    FLASH_IRQn            = 4,
    RCC_IRQn              = 5,
    EXTI0_IRQn            = 6,
    EXTI1_IRQn            = 7,
    EXTI2_IRQn            = 8,
    EXTI3_IRQn            = 9,
    EXTI4_IRQn            = 10,
    DMA1_Stream0_IRQn     = 11,
    DMA1_Stream1_IRQn     = 12,
    DMA1_Stream2_IRQn     = 13,
    DMA1_Stream3_IRQn     = 14,
    DMA1_Stream4_IRQn     = 15,
    DMA1_Stream5_IRQn     = 16,
    DMA1_Stream6_IRQn     = 17,
    ADC_IRQn              = 18,
    TIM1_BRK_TIM9_IRQn    = 24,
    TIM1_UP_TIM10_IRQn    = 25,
    TIM1_TRG_COM_TIM11_IRQn = 26,
    TIM1_CC_IRQn          = 27,
    TIM2_IRQn             = 28,
    TIM3_IRQn             = 29,
    TIM4_IRQn             = 30,
    I2C1_EV_IRQn          = 31,
    I2C1_ER_IRQn          = 32,
    SPI1_IRQn             = 35,
    SPI2_IRQn             = 36,
    USART1_IRQn           = 37,
    USART2_IRQn           = 38,
    OTG_FS_IRQn           = 67,
    DMA2_Stream0_IRQn     = 56,
    DMA2_Stream1_IRQn     = 57,
    DMA2_Stream2_IRQn     = 58,
    DMA2_Stream3_IRQn     = 59,
} IRQn_Type;

/* ========================================================================
 * Bit manipulation helpers
 * ======================================================================== */

#define BIT(n)               (1UL << (n))
#define SET_BIT(reg, bit)    ((reg) |= (bit))
#define CLEAR_BIT(reg, bit)  ((reg) &= ~(bit))
#define READ_BIT(reg, bit)   ((reg) & (bit))
#define MODIFY_REG(reg, clearmask, setmask) \
    ((reg) = (((reg) & ~(clearmask)) | (setmask)))

/* ========================================================================
 * FPGA Register Map (accessed via SPI)
 * ======================================================================== */

/* These are 16-bit registers inside the FPGA bitstream,
 * addressed via the command byte in SPI transactions.
 * The FPGA implements a simple register-read/write protocol:
 *   CMD byte: [R/W#][ADDR5:0][DONTCARE:0]
 *   R/W# = 0 for write, 1 for read
 *   ADDR5:0 maps to the registers below
 */

#define FPGA_REG_VERSION       0x00    /* R/O: FPGA bitstream version (BCD) */
#define FPGA_REG_STATUS        0x01    /* R/O: Status flags */
#define FPGA_REG_CTRL          0x02    /* R/W: Global control */
#define FPGA_REG_TRIG_MODE     0x03    /* R/W: Trigger mode select */
#define FPGA_REG_TRIG_DELAY_LO 0x04    /* R/W: Trigger-to-glitch delay [15:0] */
#define FPGA_REG_TRIG_DELAY_HI 0x05    /* R/W: Trigger-to-glitch delay [23:16] */
#define FPGA_REG_GLITCH_WIDTH_LO 0x06 /* R/W: Glitch pulse width [15:0] (100ps units) */
#define FPGA_REG_GLITCH_WIDTH_HI 0x07 /* R/W: Glitch pulse width [23:16] */
#define FPGA_REG_GLITCH_SHAPE  0x08   /* R/W: Glitch shape select */
#define FPGA_REG_GLITCH_MODE   0x09   /* R/W: Glitch mode (VCC/EM/CLK) */
#define FPGA_REG_PLL_CTRL      0x0A   /* R/W: PLL control & divisor */
#define FPGA_REG_PLL_FRAC_LO   0x0B  /* R/W: PLL fractional divisor [15:0] */
#define FPGA_REG_PLL_FRAC_HI   0x0C  /* R/W: PLL fractional divisor [23:16] */
#define FPGA_REG_UART_PATTERN0 0x0D   /* R/W: UART trigger pattern byte 0 */
#define FPGA_REG_UART_PATTERN1 0x0E   /* R/W: UART trigger pattern byte 1 */
#define FPGA_REG_UART_PATTERN2 0x0F   /* R/W: UART trigger pattern byte 2 */
#define FPGA_REG_UART_PATTERN3 0x10   /* R/W: UART trigger pattern byte 3 */
#define FPGA_REG_UART_MASK     0x11   /* R/W: UART pattern match mask */
#define FPGA_REG_JTAG_STATE    0x12   /* R/W: JTAG TAP state to trigger on */
#define FPGA_REG_GPIO_TRIG_CFG 0x13   /* R/W: GPIO trigger config (edge select) */
#define FPGA_REG_REPEAT_COUNT  0x14   /* R/W: Repeat count for multi-glitch */
#define FPGA_REG_REPEAT_DELAY  0x15   /* R/W: Inter-glitch delay (clock cycles) */
#define FPGA_REG_PHASE_OFFSET  0x16   /* R/W: Phase offset for clock glitch */
#define FPGA_REG_DAC_VAL       0x17   /* R/W: EM pulse amplitude DAC value */
#define FPGA_REG_TIMESTAMP_LO  0x18   /* R/O: Last trigger timestamp [15:0] */
#define FPGA_REG_TIMESTAMP_HI  0x19   /* R/O: Last trigger timestamp [31:16] */
#define FPGA_REG_GLITCH_COUNT_LO 0x1A /* R/O: Glitch fire counter [15:0] */
#define FPGA_REG_GLITCH_COUNT_HI 0x1B /* R/O: Glitch fire counter [31:16] */
#define FPGA_REG_WAVEFORM_ADDR 0x1C   /* R/W: Waveform RAM write address */
#define FPGA_REG_WAVEFORM_DATA 0x1D   /* R/W: Waveform RAM write data */
#define FPGA_REG_WAVEFORM_CTRL 0x1E   /* R/W: Waveform playback control */
#define FPGA_REG_FAN_CTRL      0x1F   /* R/W: Cooling fan PWM (0=off, 255=max) */

/* FPGA STATUS register bits */
#define FPGA_STATUS_CONFIGURED   (1UL << 0)
#define FPGA_STATUS_PLL_LOCKED   (1UL << 1)
#define FPGA_STATUS_TRIGGER_ARM  (1UL << 2)
#define FPGA_STATUS_GLITCH_FIRE  (1UL << 3)
#define FPGA_STATUS_FAULT        (1UL << 4)
#define FPGA_STATUS_OVERTEMP     (1UL << 5)
#define FPGA_STATUS_UART_MATCH   (1UL << 6)
#define FPGA_STATUS_JTAG_MATCH   (1UL << 7)

/* FPGA CTRL register bits */
#define FPGA_CTRL_ENABLE         (1UL << 0)
#define FPGA_CTRL_ARM           (1UL << 1)
#define FPGA_CTRL_FIRE_NOW      (1UL << 2)
#define FPGA_CTRL_RESET         (1UL << 3)
#define FPGA_CTRL_PLL_RECONFIG   (1UL << 4)
#define FPGA_CTRL_WAVEFORM_EN   (1UL << 5)

/* FPGA GPIO trigger config bits */
#define FPGA_GPIO_TRIG_RISING    (1UL << 0)
#define FPGA_GPIO_TRIG_FALLING   (1UL << 1)
#define FPGA_GPIO_TRIG_HIGH      (1UL << 2)
#define FPGA_GPIO_TRIG_LOW       (1UL << 3)

/* ========================================================================
 * USB Vendor Requests
 * ======================================================================== */

#define USB_CMD_GET_STATUS       0x01
#define USB_CMD_SET_GLITCH_CFG   0x02
#define USB_CMD_GET_GLITCH_CFG   0x03
#define USB_CMD_ARM              0x04
#define USB_CMD_DISARM           0x05
#define USB_CMD_FIRE             0x06
#define USB_CMD_GET_RESULTS      0x07
#define USB_CMD_SET_TRIGGER      0x08
#define USB_CMD_GET_TRIGGER      0x09
#define USB_CMD_FPGA_WRITE       0x0A
#define USB_CMD_FPGA_READ        0x0B
#define USB_CMD_FPGA_LOAD_BIT    0x0C
#define USB_CMD_EEPROM_READ      0x0D
#define USB_CMD_EEPROM_WRITE     0x0E
#define USB_CMD_ADC_READ         0x0F
#define USB_CMD_CALIBRATE        0x10
#define USB_CMD_SET_PROFILE      0x11
#define USB_CMD_GET_PROFILE      0x12
#define USB_CMD_WAVEFORM_LOAD    0x13
#define USB_CMD_WAVEFORM_START   0x14
#define USB_CMD_WAVEFORM_STOP    0x15
#define USB_CMD_GET_TIMESTAMP    0x16
#define USB_CMD_RESET            0xFF

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */