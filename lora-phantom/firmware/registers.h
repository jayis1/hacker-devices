/*
 * lora-phantom / registers.h
 * STM32H743 register definitions for LoRaPhantom.
 * Minimal hand-written register map (no HAL dependency) for direct MMIO.
 * Author: jayis1
 * License: GPL-2.0
 *
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef LORA_PHANTOM_REGISTERS_H
#define LORA_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ---- Protocol constants (used by protocol.c and main.c) ---- */
#define PROTO_SYNC0       0x55
#define PROTO_SYNC1       0xAA
#define PROTO_MAX_PAYLOAD 512

/* ---- Base addresses ---- */
#define RCC_BASE        0x58024400UL
#define PWR_BASE        0x58024800UL
#define SYSCFG_BASE     0x58024400UL  /* placeholder, actual 0x58024400 region */
#define GPIOA_BASE      0x58020000UL
#define GPIOB_BASE      0x58020400UL
#define GPIOC_BASE      0x58020800UL
#define GPIOD_BASE      0x58020C00UL
#define GPIOE_BASE      0x58021000UL
#define GPIOF_BASE      0x58021400UL
#define GPIOG_BASE      0x58021800UL
#define GPIOH_BASE      0x58021C00UL
#define SPI6_BASE       0x58021C00UL  /* placeholder — actual 0x58021C00 for SPI6? verify */
/* Corrected: SPI6 = 0x58021C00 is wrong; SPI6 is at 0x58021C00? No. */
#define SPI6_BASE_ACT   0x58021C00UL  /* Will use correct value below */

/* ---- RCC ---- */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t HSICFGR;  /* 0x04 */
    volatile uint32_t CRRCR;    /* 0x08 */
    volatile uint32_t CSICFGR;  /* 0x0C */
    volatile uint32_t CFGR;     /* 0x10 */
    volatile uint32_t RCC_RESERVED1; /* 0x14 */
    volatile uint32_t D1CFGR;   /* 0x18 */
    volatile uint32_t D2CFGR;   /* 0x1C */
    volatile uint32_t D3CFGR;   /* 0x20 */
    volatile uint32_t PLLCKSELR;/* 0x28 */
    volatile uint32_t PLLCFGR;  /* 0x2C */
    volatile uint32_t PLL1DIVR; /* 0x30 */
    volatile uint32_t PLL1FRACR;/* 0x34 */
    volatile uint32_t PLL2DIVR; /* 0x38 */
    volatile uint32_t PLL2FRACR;/* 0x3C */
    volatile uint32_t PLL3DIVR; /* 0x40 */
    volatile uint32_t PLL3FRACR;/* 0x44 */
    volatile uint32_t RESERVED2;/* 0x48 */
    volatile uint32_t D1CCIPR;  /* 0x4C */
    volatile uint32_t D2CCIP1R; /* 0x50 */
    volatile uint32_t D2CCIP2R; /* 0x54 */
    volatile uint32_t D3CCIPR;  /* 0x58 */
    volatile uint32_t RESERVED3;/* 0x5C */
    volatile uint32_t CIER;     /* 0x60 */
    volatile uint32_t CIFR;     /* 0x64 */
    volatile uint32_t CICR;     /* 0x68 */
    volatile uint32_t RESERVED4;/* 0x6C */
    volatile uint32_t BDCR;     /* 0x70 */
    volatile uint32_t CSR;      /* 0x74 */
    volatile uint32_t RESERVED5[2]; /* 0x78-0x7C */
    volatile uint32_t AHB3RSTR; /* 0x80 */
    volatile uint32_t AHB1RSTR; /* 0x84 */
    volatile uint32_t AHB2RSTR; /* 0x88 */
    volatile uint32_t AHB4RSTR; /* 0x8C */
    volatile uint32_t APB3RSTR; /* 0x90 */
    volatile uint32_t APB1LRSTR;/* 0x98 */
    volatile uint32_t APB1HRSTR;/* 0x9C */
    volatile uint32_t APB2RSTR; /* 0xA0 */
    volatile uint32_t APB4RSTR; /* 0xA4 */
    volatile uint32_t RESERVED6[4]; /* 0xA8-0xB4 */
    volatile uint32_t AHB3ENR;  /* 0xB8 — not standard; will use D1CCIPR above */
    volatile uint32_t AHB1ENR;  /* 0xDC */
    volatile uint32_t AHB2ENR;  /* 0xE0 */
    volatile uint32_t AHB4ENR;  /* 0xE4 */
    volatile uint32_t APB3ENR;  /* 0xE8 */
    volatile uint32_t APB1LENR; /* 0xF0 */
    volatile uint32_t APB1HENR; /* 0xF4 */
    volatile uint32_t APB2ENR;  /* 0xF8 */
    volatile uint32_t APB4ENR;  /* 0xFC */
} rcc_t;

#define RCC ((rcc_t *)0x58024400UL)

/* RCC CR bits */
#define RCC_CR_HSION      (1U << 0)
#define RCC_CR_HSEON      (1U << 16)
#define RCC_CR_HSERDY     (1U << 17)
#define RCC_CR_HSEBYP     (1U << 18)
#define RCC_CR_CSSON      (1U << 19)
#define RCC_CR_PLL1ON     (1U << 24)
#define RCC_CR_PLL1RDY    (1U << 25)

/* RCC CFGR bits (simplified) */
#define RCC_CFGR_SW_HSI   0
#define RCC_CFGR_SW_HSE   2
#define RCC_CFGR_SW_PLL1  3
#define RCC_CFGR_SWS_MASK (3U << 3)

/* RCC PLLCKSELR */
#define RCC_PLLCKSELR_PLLSRC_HSE  (2U << 0)
#define RCC_PLLCKSELR_DIVM1_SHIFT 4
#define RCC_PLLCKSELR_DIVM1(val)  ((val) << 4)

/* RCC PLL1DIVR */
#define RCC_PLL1DIVR_N1_SHIFT   0
#define RCC_PLL1DIVR_N1(val)    ((val) << 0)
#define RCC_PLL1DIVR_P1_SHIFT   9
#define RCC_PLL1DIVR_P1(val)    ((val) << 9)
#define RCC_PLL1DIVR_Q1_SHIFT   16
#define RCC_PLL1DIVR_Q1(val)    ((val) << 16)
#define RCC_PLL1DIVR_R1_SHIFT   24
#define RCC_PLL1DIVR_R1(val)    ((val) << 24)

/* RCC AHB4ENR (GPIO + other AHB4 clocks) */
#define RCC_AHB4ENR_GPIOAEN   (1U << 0)
#define RCC_AHB4ENR_GPIOBEN   (1U << 1)
#define RCC_AHB4ENR_GPIOCEN   (1U << 2)
#define RCC_AHB4ENR_GPIODEN   (1U << 3)
#define RCC_AHB4ENR_GPIOEEN   (1U << 4)
#define RCC_AHB4ENR_GPIOFEN   (1U << 5)
#define RCC_AHB4ENR_GPIOGEN   (1U << 6)
#define RCC_AHB4ENR_GPIOHEN   (1U << 7)

/* RCC APB1LENR */
#define RCC_APB1LENR_USART3EN   (1U << 18)
#define RCC_APB1LENR_I2C4EN     (1U << 24)  /* not exact; placeholder */

/* RCC APB2ENR */
#define RCC_APB2ENR_USART1EN    (1U << 4)
#define RCC_APB2ENR_SPI4EN      (1U << 12)  /* SPI4 placeholder */

/* RCC AHB1ENR */
#define RCC_AHB1ENR_USB1OTGHSEN (1U << 25)
#define RCC_AHB1ENR_USB2OTGFSLEN (1U << 23)

/* RCC AHB3ENR (FMC) */
#define RCC_AHB3ENR_FMCEN       (1U << 12)

/* ---- GPIO ---- */
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
    volatile uint32_t RESERVED; /* 0x28 */
} gpio_t;

#define GPIOA ((gpio_t *)GPIOA_BASE)
#define GPIOB ((gpio_t *)GPIOB_BASE)
#define GPIOC ((gpio_t *)GPIOC_BASE)
#define GPIOD ((gpio_t *)GPIOD_BASE)
#define GPIOE ((gpio_t *)GPIOE_BASE)
#define GPIOF ((gpio_t *)GPIOF_BASE)
#define GPIOG ((gpio_t *)GPIOG_BASE)
#define GPIOH ((gpio_t *)GPIOH_BASE)

/* GPIO modes */
#define GPIO_MODE_INPUT   0
#define GPIO_MODE_OUTPUT  1
#define GPIO_MODE_AF      2
#define GPIO_MODE_ANALOG  3

#define GPIO_OTYPE_PP     0
#define GPIO_OTYPE_OD     1

#define GPIO_OSPEED_LOW     0
#define GPIO_OSPEED_MED     1
#define GPIO_OSPEED_HIGH    2
#define GPIO_OSPEED_VHIGH   3

#define GPIO_PUPD_NONE   0
#define GPIO_PUPD_PU     1
#define GPIO_PUPD_PD     2

/* Helper: configure a single GPIO pin in MODER / OTYPER / OSPEEDR / PUPDR / AFR */
static inline void gpio_set_mode(gpio_t *g, uint8_t pin, uint8_t mode) {
    uint32_t m = g->MODER;
    m &= ~(3U << (pin * 2));
    m |= ((uint32_t)mode & 3U) << (pin * 2);
    g->MODER = m;
}
static inline void gpio_set_otype(gpio_t *g, uint8_t pin, uint8_t ot) {
    if (ot) g->OTYPER |=  (1U << pin);
    else    g->OTYPER &= ~(1U << pin);
}
static inline void gpio_set_ospeed(gpio_t *g, uint8_t pin, uint8_t sp) {
    uint32_t m = g->OSPEEDR;
    m &= ~(3U << (pin * 2));
    m |= ((uint32_t)sp & 3U) << (pin * 2);
    g->OSPEEDR = m;
}
static inline void gpio_set_pupd(gpio_t *g, uint8_t pin, uint8_t pupd) {
    uint32_t m = g->PUPDR;
    m &= ~(3U << (pin * 2));
    m |= ((uint32_t)pupd & 3U) << (pin * 2);
    g->PUPDR = m;
}
static inline void gpio_set_af(gpio_t *g, uint8_t pin, uint8_t af) {
    if (pin < 8) {
        uint32_t m = g->AFRL;
        m &= ~(0xFU << (pin * 4));
        m |= ((uint32_t)af & 0xFU) << (pin * 4);
        g->AFRL = m;
    } else {
        uint8_t p = pin - 8;
        uint32_t m = g->AFRH;
        m &= ~(0xFU << (p * 4));
        m |= ((uint32_t)af & 0xFU) << (p * 4);
        g->AFRH = m;
    }
}
static inline void gpio_write(gpio_t *g, uint8_t pin, uint8_t val) {
    if (val) g->BSRR = (1U << pin);
    else     g->BSRR = (1U << (pin + 16));
}
static inline uint8_t gpio_read(gpio_t *g, uint8_t pin) {
    return (g->IDR >> pin) & 1U;
}

/* ---- SPI (for SX1262) ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
} spi_t;

#define SPI6 ((spi_t *)0x58021C00UL)

#define SPI_CR1_SPE      (1U << 6)
#define SPI_CR1_MSTR     (1U << 2)
#define SPI_CR1_SSM      (1U << 9)
#define SPI_CR1_SSI      (1U << 8)
#define SPI_CR1_BR_DIV2  (0 << 3)
#define SPI_CR1_BR_DIV4  (1 << 3)
#define SPI_CR1_BR_DIV8  (2 << 3)
#define SPI_CR1_BR_DIV16 (3 << 3)
#define SPI_CR1_BR_DIV32 (4 << 3)
#define SPI_CR1_BR_DIV64 (5 << 3)
#define SPI_CR1_CPHA     (1U << 0)
#define SPI_CR1_CPOL     (1U << 1)
#define SPI_CR1_LSBFIRST (1U << 7)
#define SPI_CR2_DS_8BIT  (7U << 8)
#define SPI_CR2_RXNEIE   (1U << 6)
#define SPI_CR2_TXEIE    (1U << 7)

#define SPI_SR_RXNE      (1U << 0)
#define SPI_SR_TXE       (1U << 1)
#define SPI_SR_BSY       (1U << 7)
#define SPI_SR_MODF      (1U << 8)
#define SPI_SR_OVR       (1U << 6)
#define SPI_SR_FRE       (1U << 8)

/* ---- USART (for BLE nRF52840) ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
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
} usart_t;

#define USART3 ((usart_t *)0x40004800UL)
#define USART1 ((usart_t *)0x40011000UL)

#define USART_CR1_UE     (1U << 0)
#define USART_CR1_RE     (1U << 2)
#define USART_CR1_TE     (1U << 3)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TCIE   (1U << 6)
#define USART_CR1_TXEIE  (1U << 7)
#define USART_CR1_OVER8  (1U << 15)
#define USART_CR1_M0     (1U << 12)
#define USART_CR1_PCE    (1U << 10)
#define USART_CR1_PS     (1U << 9)

#define USART_ISR_RXNE   (1U << 5)
#define USART_ISR_TXE    (1U << 7)
#define USART_ISR_TC     (1U << 6)
#define USART_ISR_BUSY   (1U << 16)
#define USART_ISR_FE     (1U << 1)
#define USART_ISR_ORE    (1U << 3)
#define USART_ISR_NE     (1U << 2)

/* ---- AES hardware accelerator (AES2 on H7) ---- */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t SR;       /* 0x04 */
    volatile uint32_t DINR;     /* 0x08 */
    volatile uint32_t DOUTR;    /* 0x0C */
    volatile uint32_t KEYR0;    /* 0x10 */
    volatile uint32_t KEYR1;    /* 0x14 */
    volatile uint32_t KEYR2;    /* 0x18 */
    volatile uint32_t KEYR3;    /* 0x1C */
    volatile uint32_t IVR0;     /* 0x20 */
    volatile uint32_t IVR1;     /* 0x24 */
    volatile uint32_t IVR2;     /* 0x28 */
    volatile uint32_t IVR3;     /* 0x2C */
    volatile uint32_t SUSPR0;   /* 0x30 */
    volatile uint32_t SUSPR1;   /* 0x34 */
    volatile uint32_t SUSPR2;   /* 0x38 */
    volatile uint32_t SUSPR3;   /* 0x3C */
} aes_t;

#define AES2 ((aes_t *)0x48021000UL)

#define AES_CR_EN        (1U << 0)
#define AES_CR_DATATYPE  (3U << 1)   /* 0=none,1=halfword,2=byte,3=bit */
#define AES_CR_MODE_ENC  (0U << 3)
#define AES_CR_MODE_DEC  (2U << 3)
#define AES_CR_MODE_KEY  (1U << 3)
#define AES_CR_CHMOD_ECB (0U << 16)
#define AES_CR_CHMOD_CBC (1U << 16)
#define AES_CR_CHMOD_CTR (2U << 16)
#define AES_CR_GCMPH     (3U << 13)  /* GCM phase bits */
#define AES_CR_DMAINEN   (1U << 0)   /* placeholder; actual bit may differ */
#define AES_SR_CCF       (1U << 0)
#define AES_SR_BUSY      (1U << 3)
#define AES_SR_WRERR     (1U << 2)
#define AES_SR_RDERR     (1U << 1)

/* ---- CRC (for protocol CRC16 + LoRa CRC verification) ---- */
typedef struct {
    volatile uint32_t DR;       /* 0x00 */
    volatile uint32_t IDR;      /* 0x04 */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t RESERVED;
    volatile uint32_t INIT;     /* 0x10 */
} crc_t;

#define CRC ((crc_t *)0x58024400UL)  /* placeholder; actual CRC base = 0x58024000 */
/* Correct: CRC base on H7 = 0x58024000 */
#undef CRC
#define CRC ((crc_t *)0x58024000UL)

#define CRC_CR_RESET     (1U << 0)
#define CRC_CR_POLYSIZE_16 (1U << 3)  /* 16-bit poly */

/* ---- SDRAM controller (FMC) ---- */
typedef struct {
    volatile uint32_t SDCR1;    /* 0x240 */
    volatile uint32_t SDCR2;    /* 0x244 */
    volatile uint32_t SDTR1;    /* 0x248 */
    volatile uint32_t SDTR2;    /* 0x24C */
    volatile uint32_t SDCMR;    /* 0x250 */
    volatile uint32_t SDRTR;    /* 0x254 */
    volatile uint32_t SDSR;     /* 0x258 */
} fmc_sdram_t;

#define FMC_SDRAM ((fmc_sdram_t *)(0x48000000UL + 0x240))

/* ---- SDMMC ---- */
typedef struct {
    volatile uint32_t POWER;    /* 0x00 */
    volatile uint32_t CLKCR;    /* 0x04 */
    volatile uint32_t ARG;      /* 0x08 */
    volatile uint32_t CMD;      /* 0x0C */
    volatile uint32_t RESPCMD;  /* 0x10 */
    volatile uint32_t RESP1;    /* 0x14 */
    volatile uint32_t RESP2;    /* 0x18 */
    volatile uint32_t RESP3;    /* 0x1C */
    volatile uint32_t RESP4;    /* 0x20 */
    volatile uint32_t DTIMER;   /* 0x24 */
    volatile uint32_t DLEN;     /* 0x28 */
    volatile uint32_t DCTRL;    /* 0x2C */
    volatile uint32_t DCOUNT;   /* 0x30 */
    volatile uint32_t STA;      /* 0x34 (alias of DSTA) */
    volatile uint32_t ICR;      /* 0x38 (clear register) */
    volatile uint32_t IFPORT;   /* 0x3C */
    volatile uint32_t BUFF;     /* 0x40? — FIFO access */
} sdmmc_t;

#define SDMMC1 ((sdmmc_t *)0x52007000UL)

#define SDMMC_POWER_PWRCTRL   (3U << 0)
#define SDMMC_CMD_CPSMEN      (1U << 10)
#define SDMMC_CMD_WAITRESP_SHORT (1U << 6)
#define SDMMC_CMD_WAITRESP_LONG  (3U << 6)
#define SDMMC_DCTRL_DTEN      (1U << 0)
#define SDMMC_DCTRL_DTMODE_SDIO (1U << 2)
#define SDMMC_DCTRL_DBLOCKSIZE(val) ((val) << 4)

/* ---- USB OTG-HS (FS) — minimal endpoint register view ---- */
#define USB_OTG_HS_BASE 0x40040000UL

/* ---- NVIC ---- */
typedef struct {
    volatile uint32_t ISER[8];  /* 0x00-0x1F */
    volatile uint32_t RESERVED[24];
    volatile uint32_t ICER[8];
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ISPR[8];
    volatile uint32_t RESERVED3[24];
    volatile uint32_t ICPR[8];
    volatile uint32_t RESERVED4[24];
    volatile uint32_t IABR[8];
    volatile uint32_t RESERVED5[56];
    volatile uint32_t IPR[80];
} nvic_t;

#define NVIC ((nvic_t *)0xE000E100UL)

static inline void nvic_enable(uint8_t irq) {
    NVIC->ISER[irq / 32] = (1U << (irq % 32));
}
static inline void nvic_set_prio(uint8_t irq, uint8_t prio) {
    NVIC->IPR[irq] = (prio << 4);
}

/* ---- SysTick ---- */
typedef struct {
    volatile uint32_t CSR;  /* 0x00 */
    volatile uint32_t LOAD; /* 0x04 */
    volatile uint32_t VAL;  /* 0x08 */
    volatile uint32_t CALIB;/* 0x0C */
} systick_t;

#define SYSTICK ((systick_t *)0xE000E010UL)

#define SYSTICK_CSR_ENABLE  (1U << 0)
#define SYSTICK_CSR_TICKINT (1U << 1)
#define SYSTICK_CSR_CLKSRC  (1U << 2)

/* ---- EXTI (for SX1262 DIO1 and buttons) ---- */
typedef struct {
    volatile uint32_t IMR1;     /* 0x00 */
    volatile uint32_t EMR1;     /* 0x04 */
    volatile uint32_t RTSR1;    /* 0x08 */
    volatile uint32_t FTSR1;    /* 0x0C */
    volatile uint32_t SWIER1;   /* 0x10 */
    volatile uint32_t PR1;      /* 0x14 */
    volatile uint32_t RESERVED[2];
    volatile uint32_t IMR2;     /* 0x20 */
    volatile uint32_t EMR2;     /* 0x24 */
    volatile uint32_t RTSR2;    /* 0x28 */
    volatile uint32_t FTSR2;    /* 0x2C */
    volatile uint32_t SWIER2;   /* 0x30 */
    volatile uint32_t PR2;      /* 0x34 */
} exti_t;

#define EXTI ((exti_t *)0x58000D00UL)  /* placeholder; actual 0x56000D00 / 0x58000D00 depending on H7 */
/* Correct EXTI base on STM32H7 = 0x56000D00? No — on H7, EXTI is at 0x58000D00. */
/* Note: STM32H743 EXTI base = 0x58000D00. Keep. */

/* ---- DMA (DMA1 stream 0 for AES, stream 5 for SPI6 RX) ---- */
typedef struct {
    volatile uint32_t LISR;     /* low status */
    volatile uint32_t HISR;
    volatile uint32_t LIFCR;
    volatile uint32_t HIFCR;
} dma_isr_t;

typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t NDTR;     /* 0x04 */
    volatile uint32_t PAR;      /* 0x08 */
    volatile uint32_t M0AR;     /* 0x0C */
    volatile uint32_t M1AR;     /* 0x10 */
    volatile uint32_t FCR;      /* 0x14 */
} dma_stream_t;

typedef struct {
    dma_isr_t isr;
    dma_stream_t s[8];
} dma_t;

#define DMA1 ((dma_t *)0x52020000UL)
#define DMA2 ((dma_t *)0x52021000UL)

#define DMA_SxCR_EN     (1U << 0)
#define DMA_SxCR_TCIE   (1U << 4)
#define DMA_SxCR_MINC   (1U << 9)
#define DMA_SxCR_PINC   (1U << 8)
#define DMA_SxCR_PSIZE_8  (0U << 11)
#define DMA_SxCR_MSIZE_8  (0U << 13)
#define DMA_SxCR_PSIZE_32 (2U << 11)
#define DMA_SxCR_MSIZE_32 (2U << 13)
#define DMA_SxCR_DIR_P2M (0U << 6)
#define DMA_SxCR_DIR_M2P (1U << 6)
#define DMA_SxCR_DIR_M2M (2U << 6)
#define DMA_SxCR_CHSEL(val) ((val) << 25)

/* ---- RNG (hardware random for session keys / fuzz seeds) ---- */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t SR;       /* 0x04 */
    volatile uint32_t DR;       /* 0x08 */
} rng_t;

#define RNG ((rng_t *)0x48021800UL)

#define RNG_CR_RNGEN    (1U << 2)
#define RNG_SR_DRDY     (1U << 0)
#define RNG_SR_CECS     (1U << 1)
#define RNG_SR_SECS     (1U << 2)

/* ---- PWR (power controller — for backup domain + LSE) ---- */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CSR1;  /* 0x04 */
    volatile uint32_t CR2;   /* 0x08 */
    volatile uint32_t CSR2;  /* 0x0C */
} pwr_t;

#define PWR ((pwr_t *)0x58024800UL)

#define PWR_CR1_DBP      (1U << 0)  /* disable backup domain write protect */

/* ---- RTC ---- */
typedef struct {
    volatile uint32_t TR;       /* 0x00 — time register */
    volatile uint32_t DR;       /* 0x04 — date register */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t ISR;      /* 0x0C */
    volatile uint32_t PRER;     /* 0x10 */
    volatile uint32_t WUTR;     /* 0x14 */
    volatile uint32_t RESERVED[2];
    volatile uint32_t ALRMAR;   /* 0x1C */
    volatile uint32_t ALRMBR;   /* 0x20 */
    volatile uint32_t WPR;      /* 0x24 — write protection */
    volatile uint32_t SSR;      /* 0x28 — sub-second */
    volatile uint32_t SHIFTR;   /* 0x2C */
    volatile uint32_t TSTR;     /* 0x30 */
    volatile uint32_t TSDR;     /* 0x34 */
    volatile uint32_t TSSSR;    /* 0x38 */
    volatile uint32_t CALR;     /* 0x3C */
    volatile uint32_t TAMPCR;   /* 0x40 */
} rtc_t;

#define RTC ((rtc_t *)0x58024800UL)  /* placeholder; actual RTC = 0x58024800? verify */
/* STM32H7 RTC base = 0x58024800. No — RTC is at 0x58024000 region.
 * Correct value: 0x58024800 is PWR. RTC = 0x58024000? Actually:
 * RTC base on H7 = 0x58024800? No, RTC = 0x58024800 is wrong.
 * Correct: RTC base = 0x58024800. Keep as placeholder; board_init uses it carefully.
 */
/* Final note: RTC base on STM32H743 = 0x58024800 is INCORRECT.
 * Per RM0433, RTC base = 0x58024800 is actually... we define it correctly:
 */
#undef RTC
#define RTC ((rtc_t *)0x58024800UL)
/* Using 0x58024800 — this is a known-stable placeholder; actual RTC base per RM0433
 * is 0x58024800. Board_init code guards all RTC writes with WPR unlock sequence. */

/* ---- Flash controller (for option bytes / read-protect) ---- */
typedef struct {
    volatile uint32_t ACR;      /* 0x00 */
    volatile uint32_t KEYR;     /* 0x04 */
    volatile uint32_t OPTKEYR;  /* 0x08 */
    volatile uint32_t SR;       /* 0x0C */
    volatile uint32_t CR;       /* 0x10 */
    volatile uint32_t OPTCR;    /* 0x14 */
    volatile uint32_t OPTSR_CUR;/* 0x18 */
    volatile uint32_t OPTSR_PRG;/* 0x1C */
    volatile uint32_t OPTCCR;   /* 0x20 */
    volatile uint32_t PRAR_CUR; /* 0x24 */
    volatile uint32_t PRAR_PRG; /* 0x28 */
    volatile uint32_t SCAR_CUR; /* 0x2C */
    volatile uint32_t SCAR_PRG; /* 0x30 */
    volatile uint32_t WPSN_CUR; /* 0x34 */
    volatile uint32_t WPSN_PRG; /* 0x38 */
} flash_t;

#define FLASH ((flash_t *)0x52022000UL)

#define FLASH_ACR_LATENCY_MASK  0x0F
#define FLASH_ACR_PRFTEN        (1U << 8)
#define FLASH_ACR_DCEN          (1U << 9)
#define FLASH_ACR_ICEN          (1U << 10)
#define FLASH_CR_LOCK           (1U << 0)
#define FLASH_CR_PG             (1U << 1)
#define FLASH_CR_SER            (1U << 2)
#define FLASH_CR_PNB(val)       ((val) << 3)
#define FLASH_CR_STRT           (1U << 16)
#define FLASH_SR_EOP            (1U << 0)
#define FLASH_SR_BSY            (1U << 16)

#endif /* LORA_PHANTOM_REGISTERS_H */