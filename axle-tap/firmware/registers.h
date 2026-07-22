/*
 * registers.h — STM32H723ZGT6 register definitions for AxleTap
 * Automotive Ethernet (100/1000BASE-T1) Tap, MITM & gPTP spoofing platform
 *
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * Minimal hand-rolled register map (no HAL dependency). Only the
 * peripherals used by AxleTap are defined: RCC, GPIO, ETH MAC1/MAC2,
 * DMA1/DMA2, USART3 (debug), USB OTG-HS, SPI6 (nRF52820), SDMMC1,
 * TIM2 (gPTP fine timer), IWDG, PWR, SYSCFG, EXTI, RNG, CRC, AES.
 */

#ifndef AXLETAP_REGISTERS_H
#define AXLETAP_REGISTERS_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Base addresses                                                       */
/* ------------------------------------------------------------------ */
#define PERIPH_BASE        (0x40000000UL)
#define AHB1_BASE          (PERIPH_BASE + 0x00020000UL)
#define AHB2_BASE          (PERIPH_BASE + 0x08020000UL)
#define AHB3_BASE          (PERIPH_BASE + 0x10000000UL)
#define AHB4_BASE          (0x58020000UL)
#define APB1_BASE          (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE          (PERIPH_BASE + 0x00010000UL)
#define APB4_BASE          (0x58000000UL)

/* RCC */
#define RCC_BASE            (0x58024400UL)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB1ENR1        (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB1ENR2        (*(volatile uint32_t *)(RCC_BASE + 0xEC))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF0))
#define RCC_APB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF4))

/* RCC bits */
#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSIRDY       (1U << 1)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_PLL1ON        (1U << 24)
#define RCC_CR_PLL1RDY      (1U << 25)
#define RCC_CR_CSION        (1U << 7)

/* PWR */
#define PWR_BASE            (0x58024800UL)
#define PWR_CR1            (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR3            (*(volatile uint32_t *)(PWR_BASE + 0x0C))
#define PWR_SR1            (*(volatile uint32_t *)(PWR_BASE + 0x14))
#define PWR_CPUCR          (*(volatile uint32_t *)(PWR_BASE + 0x50))

/* GPIO ports (A..K) */
#define GPIOA_BASE          (0x58020000UL)
#define GPIOB_BASE          (0x58020400UL)
#define GPIOC_BASE          (0x58020800UL)
#define GPIOD_BASE          (0x58020C00UL)
#define GPIOE_BASE          (0x58021000UL)
#define GPIOF_BASE          (0x58021400UL)
#define GPIOG_BASE          (0x58021800UL)
#define GPIOH_BASE          (0x58021C00UL)
#define GPIOI_BASE          (0x58022000UL)
#define GPIOJ_BASE          (0x58022400UL)
#define GPIOK_BASE          (0x58022800UL)

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
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG ((GPIO_TypeDef *)GPIOG_BASE)
#define GPIOH ((GPIO_TypeDef *)GPIOH_BASE)
#define GPIOI ((GPIO_TypeDef *)GPIOI_BASE)
#define GPIOJ ((GPIO_TypeDef *)GPIOJ_BASE)
#define GPIOK ((GPIO_TypeDef *)GPIOK_BASE)

/* GPIO modes */
#define GPIO_MODE_IN        0x0
#define GPIO_MODE_OUT        0x1
#define GPIO_MODE_AF         0x2
#define GPIO_MODE_AN         0x3

/* Ethernet MAC (STM32H7 has one MAC, two PHYs via external switch or two MACs).
 * For AxleTap we use the STM32H723 built-in Ethernet MAC for MAC1 (link A)
 * and an external SPI-controlled MAC for MAC2 (link B). The register map
 * here is for the built-in MAC; MAC2 is driven by the bridge driver over
 * a second RGMII port through an external Marvell 88E6141 L2 switch used
 * as a 2-port fabric (configured in the bridge driver).
 */
#define ETH_BASE            (0x40028000UL)

typedef struct {
    volatile uint32_t MACCR;        /* 0x00 */
    volatile uint32_t MACECR;       /* 0x04 */
    volatile uint32_t MACPFR;       /* 0x08 */
    volatile uint32_t MACWTR;       /* 0x0C */
    volatile uint32_t RESERVED0[4]; /* 0x10..0x1C */
    volatile uint32_t MACHT0R;      /* 0x20 */
    volatile uint32_t MACHT1R;      /* 0x24 */
    volatile uint32_t RESERVED1[2]; /* 0x28..0x2C */
    volatile uint32_t MACVTR;       /* 0x30 */
    volatile uint32_t RESERVED2[3]; /* 0x34..0x3C */
    volatile uint32_t MACIVIR;      /* 0x40 */
    volatile uint32_t MACTIVIR;     /* 0x44 */
    volatile uint32_t RESERVED3[2]; /* 0x48..0x4C */
    volatile uint32_t MACQ0TXFCR;   /* 0x50 */
    volatile uint32_t MACQ0RXFCR;   /* 0x54 */
    volatile uint32_t RESERVED4[6]; /* 0x58..0x6C */
    volatile uint32_t MACRXFCR;     /* 0x70 */
    volatile uint32_t MACTXFCR;     /* 0x74 */
    volatile uint32_t RESERVED5[2]; /* 0x78..0x7C */
    volatile uint32_t MACPCSR;      /* 0x80 */
    volatile uint32_t MACRWKPFR;    /* 0x84 */
    volatile uint32_t RESERVED6[2]; /* 0x88..0x8C */
    volatile uint32_t MACLCSR;      /* 0x90 */
    volatile uint32_t MACLTCR;      /* 0x94 */
    volatile uint32_t MACLETR;      /* 0x98 */
    volatile uint32_t MACA0HR;      /* 0xA0 */
    volatile uint32_t MACA0LR;      /* 0xA4 */
    volatile uint32_t MACA1HR;      /* 0xA8 */
    volatile uint32_t MACA1LR;      /* 0xAC */
} ETH_MacReg;

#define ETH_MAC ((ETH_MacReg *)ETH_BASE)

/* DMA */
#define DMA1_BASE           (0x40020000UL)
#define DMA2_BASE           (0x40020400UL)

typedef struct {
    volatile uint32_t LISR;   /* 0x00 low interrupt status */
    volatile uint32_t HISR;  /* 0x04 high interrupt status */
    uint32_t RESERVED0[4];
    volatile uint32_t S0CR;   /* 0x10 stream 0 config */
    volatile uint32_t S0NDTR;
    volatile uint32_t S0PAR;
    volatile uint32_t S0M0AR;
    volatile uint32_t S0M1AR;
    volatile uint32_t S0FCR;
} DMA_StreamRegs;

/* Ethernet DMA descriptor (enhanced descriptor format) */
typedef struct {
    volatile uint32_t TDES0;
    volatile uint32_t TDES1;
    volatile uint32_t TDES2;
    volatile uint32_t TDES3;
} ETH_DMADESC_T;

/* ETH DMA registers */
#define ETH_DMA_BASE        (ETH_BASE + 0x1000UL)
#define ETH_DMAMR           (*(volatile uint32_t *)(ETH_DMA_BASE + 0x00))
#define ETH_DMASBMR         (*(volatile uint32_t *)(ETH_DMA_BASE + 0x04))
#define ETH_DMAISR         (*(volatile uint32_t *)(ETH_DMA_BASE + 0x08))
#define ETH_DMADSR         (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0C))
#define ETH_DMACCR         (*(volatile uint32_t *)(ETH_DMA_BASE + 0x14))
#define ETH_DMACTXCR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x50))
#define ETH_DMACRXCR      (*(volatile uint32_t *)(ETH_DMA_BASE + 0x58))
#define ETH_DMACRXDLAR    (*(volatile uint32_t *)(ETH_DMA_BASE + 0x60))
#define ETH_DMACHTDLAR    (*(volatile uint32_t *)(ETH_DMA_BASE + 0x48))
#define ETH_DMACRXDTPR    (*(volatile uint32_t *)(ETH_DMA_BASE + 0x68))
#define ETH_DMACHTDTPR    (*(volatile uint32_t *)(ETH_DMA_BASE + 0x54))

/* TIM2 — gPTP fine timer */
#define TIM2_BASE            (0x40000000UL)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SMCR;    /* 0x08 */
    volatile uint32_t DIER;    /* 0x0C */
    volatile uint32_t SR;      /* 0x10 */
    volatile uint32_t EGR;     /* 0x14 */
    volatile uint32_t CCMR1;   /* 0x18 */
    volatile uint32_t CCMR2;   /* 0x1C */
    volatile uint32_t CCER;    /* 0x20 */
    volatile uint32_t CNT;     /* 0x24 */
    volatile uint32_t PSC;     /* 0x28 */
    volatile uint32_t ARR;     /* 0x2C */
    volatile uint32_t CCR1;    /* 0x30 */
    volatile uint32_t CCR2;    /* 0x34 */
    volatile uint32_t CCR3;    /* 0x38 */
    volatile uint32_t CCR4;    /* 0x3C */
    volatile uint32_t RESERVED0;
    volatile uint32_t OR;      /* 0x44 */
} TIM_TypeDef;
#define TIM2 ((TIM_TypeDef *)TIM2_BASE)

/* USART3 — debug console */
#define USART3_BASE          (0x40004800UL)
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t GTPR;
    volatile uint32_t RTOR;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} USART_TypeDef;
#define USART3 ((USART_TypeDef *)USART3_BASE)

/* SPI6 — nRF52820 BLE bridge */
#define SPI6_BASE            (0x58021000UL)
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CFG1;
    volatile uint32_t CFG2;
    volatile uint32_t IER;
    volatile uint32_t SR;
    volatile uint32_t IFCR;
    volatile uint32_t RESERVED0[4];
    volatile uint32_t TXDR;
    volatile uint32_t RXDR;
} SPI_TypeDef;
#define SPI6 ((SPI_TypeDef *)SPI6_BASE)

/* SDMMC1 — SD card capture */
#define SDMMC1_BASE          (0x40012800UL)

/* USB OTG-HS */
#define USB1_OTG_HS_BASE     (0x40080000UL)

/* SYSCFG */
#define SYSCFG_BASE          (0x58025400UL)
#define SYSCFG_PMCR         (*(volatile uint32_t *)(SYSCFG_BASE + 0x04))
#define SYSCFG_EXTICR1      (*(volatile uint32_t *)(SYSCFG_BASE + 0x08))

/* RNG */
#define RNG_BASE             (0x48029000UL)
#define RNG_CR              (*(volatile uint32_t *)(RNG_BASE + 0x00))
#define RNG_SR              (*(volatile uint32_t *)(RNG_BASE + 0x04))
#define RNG_DR              (*(volatile uint32_t *)(RNG_BASE + 0x08))

/* IWDG */
#define IWDG_BASE            (0x58004800UL)
#define IWDG_KR             (*(volatile uint32_t *)(IWDG_BASE + 0x00))
#define IWDG_PR             (*(volatile uint32_t *)(IWDG_BASE + 0x04))
#define IWDG_RLR            (*(volatile uint32_t *)(IWDG_BASE + 0x08))
#define IWDG_SR             (*(volatile uint32_t *)(IWDG_BASE + 0x0C))

/* FLASH controller */
#define FLASH_BASE           (0x58022000UL)
#define FLASH_ACR           (*(volatile uint32_t *)(FLASH_BASE + 0x00))

/* CRC unit */
#define CRC_BASE             (0x58024C00UL)
#define CRC_DR              (*(volatile uint32_t *)(CRC_BASE + 0x00))
#define CRC_CR              (*(volatile uint32_t *)(CRC_BASE + 0x08))

/* ------------------------------------------------------------------ */
/* NVIC / SCB / SysTick                                                 */
/* ------------------------------------------------------------------ */
#define SCB_BASE             (0xE000ED00UL)
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_AIRCR           (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define NVIC_ISER0          (*(volatile uint32_t *)(0xE000E100UL))
#define NVIC_ICER0          (*(volatile uint32_t *)(0xE000E180UL))
#define NVIC_IPR_BASE       (0xE000E400UL)
#define NVIC_STIR           (*(volatile uint32_t *)(0xE000EF00UL))

#define SysTick_BASE        (0xE000E010UL)
#define SysTick_CTRL        (*(volatile uint32_t *)(SysTick_BASE + 0x00))
#define SysTick_LOAD        (*(volatile uint32_t *)(SysTick_BASE + 0x04))
#define SysTick_VAL         (*(volatile uint32_t *)(SysTick_BASE + 0x08))

/* ------------------------------------------------------------------ */
/* IRQ numbers used by AxleTap                                          */
/* ------------------------------------------------------------------ */
#define ETH_IRQn            61   /* Ethernet global IRQ */
#define DMA1_Stream0_IRQn   11
#define DMA1_Stream1_IRQn   12
#define TIM2_IRQn           28
#define USART3_IRQn         39
#define SPI6_IRQn           58
#define OTG_HS_IRQn         77
#define EXTI15_10_IRQn      15

/* ------------------------------------------------------------------ */
/* Bit helpers                                                          */
/* ------------------------------------------------------------------ */
#define BIT(n)              (1UL << (n))
#define REG32(addr)        (*(volatile uint32_t *)(addr))

#endif /* AXLETAP_REGISTERS_H */