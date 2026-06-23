/*
 * ACOUSTIC-PHANTOM — MCU register definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Minimal register-level definitions for the STM32H743VIT6.
 * Not a complete CMSIS replacement — only what the firmware uses.
 */
#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ---------------------------------------------------- */
#define PERIPH_BASE        0x40000000UL
#define D1_AHB1PERI_BASE   (PERIPH_BASE + 0x00020000UL)
#define D1_AHB2PERI_BASE   (PERIPH_BASE + 0x00010000UL)
#define D1_AHB3PERI_BASE   (PERIPH_BASE + 0x10000000UL)  /* FMC, QSPI */
#define D1_APB1L_BASE      (PERIPH_BASE + 0x00000000UL)
#define D1_APB1H_BASE      (PERIPH_BASE + 0x00020000UL)
#define D1_APB2_BASE       (PERIPH_BASE + 0x00010000UL)
#define D2_AHB1PERI_BASE   (PERIPH_BASE + 0x00020000UL)
#define D2_AHB2PERI_BASE   (PERIPH_BASE + 0x00030000UL)
#define D2_APB1L_BASE      (PERIPH_BASE + 0x00020000UL)
#define D2_APB1H_BASE      (PERIPH_BASE + 0x00020400UL)
#define D2_APB2_BASE       (PERIPH_BASE + 0x00010000UL)

/* ---- RCC --------------------------------------------------------------- */
#define RCC_BASE           (D1_AHB1PERI_BASE + 0x4400UL)
typedef struct {
    volatile uint32_t CR;          /* 0x00 */
    volatile uint32_t HSICFGR;     /* 0x04 */
    volatile uint32_t CSICFGR;     /* 0x08 */
    volatile uint32_t CRRCR;       /* 0x0C */
    volatile uint32_t RESERVED0;   /* 0x10 */
    volatile uint32_t CFGR;        /* 0x14 */
    volatile uint32_t RESERVED1;   /* 0x18 */
    volatile uint32_t RESERVED2;   /* 0x1C */
    volatile uint32_t D1CFGR;      /* 0x20 */
    volatile uint32_t D2CFGR;      /* 0x24 */
    volatile uint32_t D3CFGR;      /* 0x28 */
    volatile uint32_t RESERVED3;   /* 0x2C */
    volatile uint32_t PLLCKSELR;   /* 0x30 */
    volatile uint32_t PLLCFGR;     /* 0x34 */
    volatile uint32_t PLL1DIVR;    /* 0x38 */
    volatile uint32_t PLL1FRACR;   /* 0x3C */
    volatile uint32_t PLL2DIVR;    /* 0x40 */
    volatile uint32_t PLL2FRACR;   /* 0x44 */
    volatile uint32_t PLL3DIVR;    /* 0x48 */
    volatile uint32_t PLL3FRACR;   /* 0x4C */
    volatile uint32_t RESERVED4;   /* 0x50 */
    volatile uint32_t D1CCIPR;     /* 0x54 */
    volatile uint32_t D2CCIP1R;    /* 0x58 */
    volatile uint32_t D2CCIP2R;    /* 0x5C */
    volatile uint32_t D3CCIPR;     /* 0x60 */
    volatile uint32_t RESERVED5;   /* 0x64 */
    volatile uint32_t CIER;        /* 0x68 */
    volatile uint32_t CIFR;        /* 0x6C */
    volatile uint32_t RESERVED6;   /* 0x70 */
    volatile uint32_t BDCR;        /* 0x74 */
    volatile uint32_t CSR;         /* 0x78 */
    volatile uint32_t RESERVED7[2];/* 0x7C-0x80 */
    volatile uint32_t AHB3RSTR;    /* 0x84 */
    volatile uint32_t AHB1RSTR;    /* 0x88 */
    volatile uint32_t AHB2RSTR;    /* 0x8C */
    volatile uint32_t AHB4RSTR;    /* 0x90 */
    volatile uint32_t APB3RSTR;    /* 0x94 */
    volatile uint32_t APB1LRSTR;   /* 0x98 */
    volatile uint32_t APB1HRSTR;   /* 0x9C */
    volatile uint32_t APB2RSTR;    /* 0xA0 */
    volatile uint32_t RESERVED8[4];/* 0xA4-0xB0 */
    volatile uint32_t AHB3ENR;     /* 0xB4 */
    volatile uint32_t AHB1ENR;     /* 0xB8 */
    volatile uint32_t AHB2ENR;     /* 0xBC */
    volatile uint32_t AHB4ENR;     /* 0xC0 */
    volatile uint32_t APB3ENR;     /* 0xC4 */
    volatile uint32_t APB1LENR;    /* 0xC8 */
    volatile uint32_t APB1HENR;    /* 0xCC */
    volatile uint32_t APB2ENR;     /* 0xD0 */
} RCC_TypeDef;
#define RCC                ((RCC_TypeDef *)RCC_BASE)

/* RCC bit positions */
#define RCC_CR_HSION       (1U << 0)
#define RCC_CR_HSIRDY      (1U << 1)
#define RCC_CR_HSEON       (1U << 16)
#define RCC_CR_HSERDY      (1U << 17)
#define RCC_CR_PLL1ON      (1U << 24)
#define RCC_CR_PLL1RDY     (1U << 25)
#define RCC_CFGR_SW_MASK   0x07U
#define RCC_CFGR_SW_HSI    0x00U
#define RCC_CFGR_SW_PLL1   0x03U

/* RCC enable bits */
#define RCC_AHB4ENR_GPIOA  (1U << 0)
#define RCC_AHB4ENR_GPIOB  (1U << 1)
#define RCC_AHB4ENR_GPIOC  (1U << 2)
#define RCC_AHB4ENR_GPIOD  (1U << 3)
#define RCC_AHB4ENR_GPIOE  (1U << 4)
#define RCC_AHB4ENR_CRC    (1U << 19)
#define RCC_AHB3ENR_QSPI   (1U << 14)
#define RCC_AHB3ENR_FMC    (1U << 12)
#define RCC_APB1LENR_I2C1  (1U << 21)
#define RCC_APB1LENR_SPI2  (1U << 14)
#define RCC_APB1LENR_USART2 (1U<< 17)
#define RCC_APB2ENR_SPI1   (1U << 12)
#define RCC_APB2ENR_SPI3   (1U << 15)
#define RCC_APB2ENR_USART1 (1U << 4)
#define RCC_APB2ENR_TIM1   (1U << 0)
#define RCC_APB2ENR_TIM2   (1U << 0)
#define RCC_AHB1ENR_SDMMC1 (1U << 16)
#define RCC_AHB1ENR_USB1   (1U << 25)
#define RCC_AHB1ENR_DMA1   (1U << 0)
#define RCC_AHB1ENR_DMA2   (1U << 1)
#define RCC_AHB2ENR_GPIOA  (1U << 0)

/* ---- GPIO -------------------------------------------------------------- */
#define GPIOA_BASE         (D1_AHB4PERI_BASE + 0x0000UL)
#define GPIOB_BASE         (D1_AHB4PERI_BASE + 0x0400UL)
#define GPIOC_BASE         (D1_AHB4PERI_BASE + 0x0800UL)
#define GPIOD_BASE         (D1_AHB4PERI_BASE + 0x0C00UL)
#define GPIOE_BASE         (D1_AHB4PERI_BASE + 0x1000UL)

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

#define GPIO_MODE_INPUT    0x00
#define GPIO_MODE_OUTPUT   0x01
#define GPIO_MODE_AF       0x02
#define GPIO_MODE_ANALOG   0x03

#define GPIO_PUPD_NONE     0x00
#define GPIO_PUPD_PULLUP   0x01
#define GPIO_PUPD_PULLDN   0x02

/* ---- SysTick ----------------------------------------------------------- */
#define SYSTICK_BASE       (0xE000E010UL)
typedef struct {
    volatile uint32_t CSR;   /* 0x00 */
    volatile uint32_t LOAD;  /* 0x04 */
    volatile uint32_t VAL;   /* 0x08 */
    volatile uint32_t CALIB; /* 0x0C */
} SysTick_Type;
#define SysTick ((SysTick_Type *)SYSTICK_BASE)
#define SysTick_CTRL_ENABLE   (1U << 0)
#define SysTick_CTRL_TICKINT  (1U << 1)
#define SysTick_CTRL_CLKSRC   (1U << 2)

/* ---- NVIC -------------------------------------------------------------- */
#define NVIC_BASE          (0xE000E100UL)
#define NVIC_ISER0         (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ICER0         (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ISPR0         (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ICPR0         (*(volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_IPR_BASE      (NVIC_BASE + 0x300UL)

/* IRQ numbers */
#define SPI2_IRQn          36
#define SPI3_IRQn          51
#define DMA1_Stream0_IRQn  11
#define DMA1_Stream1_IRQn  12
#define DMA1_Stream2_IRQn  13
#define DMA1_Stream3_IRQn  14
#define DMA2_Stream0_IRQn  56
#define DMA2_Stream1_IRQn  57
#define USART1_IRQn        37
#define EXTI15_10_IRQn     40

/* ---- EXTI -------------------------------------------------------------- */
#define EXTI_BASE          (D1_AHB2PERI_BASE + 0x0EC00UL)
typedef struct {
    volatile uint32_t IMR1;    /* 0x00 */
    volatile uint32_t EMR1;    /* 0x04 */
    volatile uint32_t RTSR1;   /* 0x08 */
    volatile uint32_t FTSR1;   /* 0x0C */
    volatile uint32_t SWIER1;  /* 0x10 */
    volatile uint32_t PR1;     /* 0x14 */
} EXTI_TypeDef;
#define EXTI ((EXTI_TypeDef *)EXTI_BASE)

/* ---- SPI/I2S ----------------------------------------------------------- */
#define SPI2_BASE          (D1_APB1L_BASE + 0x3800UL)
#define SPI3_BASE          (D1_APB1L_BASE + 0x3C00UL)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SR;      /* 0x08 */
    volatile uint32_t DR;      /* 0x0C */
    volatile uint32_t CRCPR;   /* 0x10 */
    volatile uint32_t RXCRCR;  /* 0x14 */
    volatile uint32_t TXCRCR;  /* 0x18 */
    volatile uint32_t I2SCFGR; /* 0x1C */
    volatile uint32_t I2SPR;   /* 0x20 */
} SPI_TypeDef;
#define SPI2 ((SPI_TypeDef *)SPI2_BASE)
#define SPI3 ((SPI_TypeDef *)SPI3_BASE)

#define SPI_CR1_SPE        (1U << 6)
#define SPI_SR_RXNE        (1U << 0)
#define SPI_SR_TXE         (1U << 1)
#define SPI_SR_OVR         (1U << 6)
#define I2SCFGR_I2SMOD     (1U << 11)
#define I2SCFGR_I2SCFG_MASK (3U << 8)
#define I2SCFGR_I2SCFG_RX  (2U << 8)

/* ---- USART1 (BLE) ------------------------------------------------------ */
#define USART1_BASE        (D1_APB2_BASE + 0x3800UL)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t CR3;     /* 0x08 */
    volatile uint32_t BRR;     /* 0x0C */
    volatile uint32_t GTPR;    /* 0x10 */
    volatile uint32_t RTOR;    /* 0x14 */
    volatile uint32_t RQR;     /* 0x18 */
    volatile uint32_t ISR;     /* 0x1C */
    volatile uint32_t ICR;     /* 0x20 */
    volatile uint32_t RDR;     /* 0x24 */
    volatile uint32_t TDR;     /* 0x28 */
} USART_TypeDef;
#define USART1 ((USART_TypeDef *)USART1_BASE)

#define USART_CR1_UE       (1U << 0)
#define USART_CR1_RE       (1U << 2)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_RXNEIE   (1U << 5)
#define USART_CR1_TXEIE    (1U << 7)
#define USART_ISR_RXNE     (1U << 5)
#define USART_ISR_TXE      (1U << 7)
#define USART_ISR_TC       (1U << 6)

/* ---- I2C1 -------------------------------------------------------------- */
#define I2C1_BASE          (D1_APB1L_BASE + 0x5400UL)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t OAR1;    /* 0x08 */
    volatile uint32_t OAR2;    /* 0x0C */
    volatile uint32_t TIMINGR; /* 0x10 */
    volatile uint32_t TIMEOUTR;/* 0x14 */
    volatile uint32_t ISR;     /* 0x18 */
    volatile uint32_t ICR;     /* 0x1C */
    volatile uint32_t PECR;    /* 0x20 */
    volatile uint32_t RXDR;    /* 0x24 */
    volatile uint32_t TXDR;    /* 0x28 */
} I2C_TypeDef;
#define I2C1 ((I2C_TypeDef *)I2C1_BASE)

#define I2C_CR1_PE         (1U << 0)
#define I2C_CR2_START      (1U << 13)
#define I2C_CR2_STOP       (1U << 14)
#define I2C_CR2_RD_WRN     (1U << 10)
#define I2C_CR2_NBYTES_MASK 0xFF00
#define I2C_ISR_TXE        (1U << 0)
#define I2C_ISR_RXNE       (1U << 2)
#define I2C_ISR_TC         (1U << 6)
#define I2C_ISR_BUSY       (1U << 15)

/* ---- DMA (Stream) ------------------------------------------------------ */
#define DMA1_BASE          (D1_AHB1PERI_BASE + 0x0000UL)
#define DMA2_BASE          (D1_AHB1PERI_BASE + 0x0400UL)
typedef struct {
    volatile uint32_t LAR;     /* 0x00 — not used in H7 */
    volatile uint32_t RESERVED0;
} DMA_Stream_t;

/* H7 DMA stream registers are more complex; we model the key fields. */
typedef struct {
    volatile uint32_t CR;      /* 0x00 + stream*0x20 */
} DMA_StreamRegs;

#define DMA1_Stream0       ((volatile uint32_t *)(DMA1_BASE + 0x010))
#define DMA1_Stream1       ((volatile uint32_t *)(DMA1_BASE + 0x030))
#define DMA2_Stream0       ((volatile uint32_t *)(DMA2_BASE + 0x010))

/* DMA stream offsets: CR, NDTR, PAR, M0AR, M1AR, FCR */
#define DMA_STREAM_SIZE    0x20
#define DMA_CR_EN          (1U << 0)
#define DMA_CR_TCIE        (1U << 4)
#define DMA_CR_HTIE        (1U << 3)
#define DMA_CR_MINC        (1U << 10)
#define DMA_CR_PINC        (1U << 9)
#define DMA_CR_CIRC        (1U << 8)
#define DMA_CR_PSIZE_16    (1U << 9)
#define DMA_CR_MSIZE_16    (1U << 11)
#define DMA_CR_DIR_P2M     (0U << 6)
#define DMA_CR_DIR_M2P     (1U << 6)
#define DMA_CR_DBM         (1U << 18)

/* ---- Flash controller (for firmware update signature check) ------------ */
#define FLASH_BASE         (D1_AHB1PERI_BASE + 0x2000UL)
typedef struct {
    volatile uint32_t ACR;      /* 0x00 */
    volatile uint32_t KEYR;     /* 0x04 */
    volatile uint32_t OPTKEYR;  /* 0x08 */
    volatile uint32_t SR;       /* 0x0C */
    volatile uint32_t CR;       /* 0x10 */
} FLASH_TypeDef;
#define FLASH ((FLASH_TypeDef *)FLASH_BASE)

/* ---- Unique device ID -------------------------------------------------- */
#define UID_BASE           0x1FF1E800UL

/* ---- TIM2 (for WS2812B LED timing) ------------------------------------- */
#define TIM2_BASE          (D1_APB1L_BASE + 0x0000UL)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SMCR;    /* 0x08 */
    volatile uint32_t DIER;    /* 0x0C */
    volatile uint32_t SR;      /* 0x10 */
    volatile uint32_t EGR;     /* 0x14 */
    volatile uint32_t RESERVED0;
    volatile uint32_t CCMR1;   /* 0x18 */
    volatile uint32_t CCMR2;   /* 0x1C */
    volatile uint32_t CCER;    /* 0x20 */
    volatile uint32_t CNT;     /* 0x24 */
    volatile uint32_t PSC;     /* 0x28 */
    volatile uint32_t ARR;     /* 0x2C */
    volatile uint32_t CCR1;    /* 0x30 */
    volatile uint32_t CCR2;    /* 0x34 */
} TIM_TypeDef;
#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM_CR1_CEN       (1U << 0)
#define TIM_CR1_ARPE      (1U << 7)

#endif /* REGISTERS_H */