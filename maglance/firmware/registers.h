/*
 * registers.h — STM32G474 register definitions for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal register-level definitions for bare-metal programming.
 * Only the peripherals used by MagLance are defined here.
 * Reference: STM32G474 Reference Manual (RM0440)
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ---- Base Addresses ---- */
#define PERIPH_BASE         0x40000000UL
#define AHB1_BASE           0x48020000UL
#define AHB2_BASE           0x48021000UL
#define APB1_BASE           0x40000000UL
#define APB2_BASE           0x40010000UL
#define AHB_PERIPH_BASE     0x40018000UL

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
    volatile uint32_t BRR;      /* 0x28 */
} GPIO_TypeDef;

#define GPIOA    ((GPIO_TypeDef *) 0x48020000UL)
#define GPIOB    ((GPIO_TypeDef *) 0x48020400UL)
#define GPIOC    ((GPIO_TypeDef *) 0x48020800UL)
#define GPIOD    ((GPIO_TypeDef *) 0x48020C00UL)
#define GPIOE    ((GPIO_TypeDef *) 0x48021000UL)
#define GPIOF    ((GPIO_TypeDef *) 0x48021400UL)
#define GPIOG    ((GPIO_TypeDef *) 0x48021800UL)

/* GPIO mode values */
#define GPIO_MODE_INPUT    0x00
#define GPIO_MODE_OUTPUT   0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG   0x03

#define GPIO_OTYPE_PP      0x00
#define GPIO_OTYPE_OD      0x01

#define GPIO_SPEED_LOW     0x00
#define GPIO_SPEED_MED     0x01
#define GPIO_SPEED_HIGH    0x02
#define GPIO_SPEED_VHIGH   0x03

#define GPIO_PUPD_NONE     0x00
#define GPIO_PUPD_PU       0x01
#define GPIO_PUPD_PD       0x02

/* ---- RCC (Reset and Clock Control) ---- */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t ICSCR;   /* 0x04 */
    volatile uint32_t CFGR;    /* 0x08 */
    volatile uint32_t PLLCFGR; /* 0x0C */
    volatile uint32_t CIR;     /* 0x10 */
    volatile uint32_t AHB1RSTR; /* 0x28 */
    volatile uint32_t AHB2RSTR; /* 0x2C */
    volatile uint32_t AHB3RSTR; /* 0x30 */
    volatile uint32_t APB1RSTR1; /* 0x38 */
    volatile uint32_t APB1RSTR2; /* 0x3C */
    volatile uint32_t APB2RSTR;  /* 0x40 */
    volatile uint32_t AHB1ENR;  /* 0x48 */
    volatile uint32_t AHB2ENR;  /* 0x4C */
    volatile uint32_t AHB3ENR;  /* 0x50 */
    volatile uint32_t APB1ENR1; /* 0x58 */
    volatile uint32_t APB1ENR2; /* 0x5C */
    volatile uint32_t APB2ENR;  /* 0x60 */
    /* ... truncated for brevity ... */
} RCC_TypeDef;

#define RCC      ((RCC_TypeDef *) 0x40021000UL)

/* RCC enable bits */
#define RCC_AHB1ENR_GPIOAEN   (1U << 0)
#define RCC_AHB1ENR_GPIOBEN   (1U << 1)
#define RCC_AHB1ENR_GPIOCEN   (1U << 2)
#define RCC_AHB1ENR_GPIODEN   (1U << 3)
#define RCC_AHB1ENR_GPIOEEN   (1U << 4)
#define RCC_AHB1ENR_GPIOFEN   (1U << 5)
#define RCC_AHB1ENR_GPIOGEN   (1U << 6)

#define RCC_AHB2ENR_ADC12EN   (1U << 0)
#define RCC_AHB2ENR_ADC345EN  (1U << 13)

#define RCC_APB1ENR1_TIM3EN   (1U << 1)
#define RCC_APB1ENR1_SPI2EN   (1U << 14)
#define RCC_APB1ENR1_I2C1EN   (1U << 21)
#define RCC_APB1ENR1_USART2EN (1U << 17)

#define RCC_APB1ENR2_USART1EN (1U << 14)

#define RCC_APB2ENR_TIM1EN    (1U << 0)
#define RCC_APB2ENR_USART1EN_APB2 (1U << 4)
#define RCC_APB2ENR_HRTIMEN   (1U << 28)

/* ---- HRTIM (High-Resolution Timer) ---- */
typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t ISR;        /* 0x08 */
    volatile uint32_t IER;        /* 0x0C */
    volatile uint32_t OENR;       /* 0x10 */
    uint32_t reserved1[2];
    volatile uint32_t CNTR;       /* 0x1C */
    volatile uint32_t FLTR;       /* 0x20 */
    volatile uint32_t BDMADR;     /* 0x24 */
    volatile uint32_t BDMUPDR;   /* 0x28 */
    /* Master Timer */
    volatile uint32_t MCR;        /* 0x2C */
    volatile uint32_t MISR;       /* 0x30 */
    volatile uint32_t MICR;      /* 0x34 */
    volatile uint32_t MDIER;     /* 0x38 */
    volatile uint32_t MCNT;      /* 0x3C */
    volatile uint32_t MPER;      /* 0x40 */
    volatile uint32_t MREP;      /* 0x44 */
    volatile uint32_t MCMP1;     /* 0x48 */
    volatile uint32_t MCMP2;     /* 0x4C */
    volatile uint32_t MCMP3;     /* 0x50 */
    volatile uint32_t MCMP4;     /* 0x54 */
    /* Timing Unit A (used for H-bridge left high-side) */
    volatile uint32_t TIMA_CR;   /* 0x58 */
    volatile uint32_t TIMA_ISR;  /* 0x5C */
    volatile uint32_t TIMA_ICR;  /* 0x60 */
    volatile uint32_t TIMA_DIER; /* 0x64 */
    volatile uint32_t TIMA_CNT;  /* 0x68 */
    volatile uint32_t TIMA_PER;  /* 0x6C */
    volatile uint32_t TIMA_CMP1; /* 0x70 */
    volatile uint32_t TIMA_CMP2; /* 0x74 */
    volatile uint32_t TIMA_CMP3; /* 0x78 */
    volatile uint32_t TIMA_CMP4; /* 0x7C */
    /* ... timing units B, C, D, E follow similar pattern ... */
    /* For MagLance we use units A-D for the H-bridge */
    volatile uint32_t TIMB_CR;   /* 0x80 */
    volatile uint32_t TIMB_ISR;  /* 0x84 */
    volatile uint32_t TIMB_ICR;  /* 0x88 */
    volatile uint32_t TIMB_DIER; /* 0x8C */
    volatile uint32_t TIMB_CNT;  /* 0x90 */
    volatile uint32_t TIMB_PER;  /* 0x94 */
    volatile uint32_t TIMB_CMP1; /* 0x98 */
    volatile uint32_t TIMB_CMP2; /* 0x9C */
    volatile uint32_t TIMB_CMP3; /* 0xA0 */
    volatile uint32_t TIMB_CMP4; /* 0xA4 */
    volatile uint32_t TIMC_CR;   /* 0xA8 */
    volatile uint32_t TIMC_ISR;  /* 0xAC */
    volatile uint32_t TIMC_ICR;  /* 0xB0 */
    volatile uint32_t TIMC_DIER; /* 0xB4 */
    volatile uint32_t TIMC_CNT;  /* 0xB8 */
    volatile uint32_t TIMC_PER;  /* 0xBC */
    volatile uint32_t TIMC_CMP1; /* 0xC0 */
    volatile uint32_t TIMC_CMP2; /* 0xC4 */
    volatile uint32_t TIMC_CMP3; /* 0xC8 */
    volatile uint32_t TIMC_CMP4; /* 0xCC */
    volatile uint32_t TIMD_CR;   /* 0xD0 */
    volatile uint32_t TIMD_ISR;  /* 0xD4 */
    volatile uint32_t TIMD_ICR;  /* 0xD8 */
    volatile uint32_t TIMD_DIER; /* 0xDC */
    volatile uint32_t TIMD_CNT;  /* 0xE0 */
    volatile uint32_t TIMD_PER;  /* 0xE4 */
    volatile uint32_t TIMD_CMP1; /* 0xE8 */
    volatile uint32_t TIMD_CMP2; /* 0xEC */
    volatile uint32_t TIMD_CMP3; /* 0xF0 */
    volatile uint32_t TIMD_CMP4; /* 0xF4 */
    /* Output set/reset registers for each timer */
    volatile uint32_t SETA1R;    /* 0xF8 */
    volatile uint32_t SETA2R;    /* 0xFC */
    volatile uint32_t RSTA1R;    /* 0x100 */
    volatile uint32_t RSTA2R;    /* 0x104 */
    volatile uint32_t SETB1R;    /* 0x108 */
    volatile uint32_t SETB2R;    /* 0x10C */
    volatile uint32_t RSTB1R;    /* 0x110 */
    volatile uint32_t RSTB2R;    /* 0x114 */
    volatile uint32_t SETC1R;    /* 0x118 */
    volatile uint32_t SETC2R;    /* 0x11C */
    volatile uint32_t RSTC1R;    /* 0x120 */
    volatile uint32_t RSTC2R;    /* 0x124 */
    volatile uint32_t SETD1R;    /* 0x128 */
    volatile uint32_t SETD2R;    /* 0x12C */
    volatile uint32_t RSTD1R;    /* 0x130 */
    volatile uint32_t RSTD2R;    /* 0x134 */
    /* ... EExR, EE6R, etc. for external events ... */
    volatile uint32_t EE1R;      /* 0x138 */
    volatile uint32_t EE2R;      /* 0x13C */
    volatile uint32_t EE3R;      /* 0x140 */
    volatile uint32_t EE4R;      /* 0x144 */
    volatile uint32_t EE5R;      /* 0x148 */
    volatile uint32_t EE6R;      /* 0x14C */
    /* Fault conditioning */
    volatile uint32_t FLT1R;     /* 0x150 */
    volatile uint32_t FLT2R;     /* 0x154 */
    volatile uint32_t FLT3R;     /* 0x158 */
    volatile uint32_t FLT4R;     /* 0x15C */
    volatile uint32_t FLT5R;     /* 0x160 */
    volatile uint32_t CH1R;      /* 0x164 */
    volatile uint32_t CH2R;      /* 0x168 */
    volatile uint32_t CH3R;      /* 0x16C */
    volatile uint32_t CH4R;      /* 0x170 */
    volatile uint32_t CH5R;      /* 0x174 */
    volatile uint32_t CH6R;      /* 0x178 */
    /* Compare registers for output generation */
    volatile uint32_t OCH1R;    /* 0x17C */
    volatile uint32_t OCH2R;    /* 0x180 */
    volatile uint32_t OCH3R;    /* 0x184 */
    volatile uint32_t OCH4R;    /* 0x188 */
    volatile uint32_t OCH5R;    /* 0x18C */
    volatile uint32_t OCH6R;    /* 0x190 */
    volatile uint32_t OCH7R;    /* 0x194 */
    volatile uint32_t OCH8R;    /* 0x198 */
    volatile uint32_t OCH9R;    /* 0x19C */
    volatile uint32_t OCH10R;   /* 0x1A0 */
    volatile uint32_t OCH11R;   /* 0x1A4 */
    volatile uint32_t OCH12R;   /* 0x1A8 */
    volatile uint32_t OCH13R;   /* 0x1AC */
    volatile uint32_t OCH14R;   /* 0x1B0 */
    volatile uint32_t OCH15R;   /* 0x1B4 */
    volatile uint32_t OCH16R;   /* 0x1B8 */
    /* Timer E (used for sweep mode frequency generation) */
    volatile uint32_t TIME_CR;   /* 0x1BC */
    volatile uint32_t TIME_ISR;  /* 0x1C0 */
    volatile uint32_t TIME_ICR;  /* 0x1C4 */
    volatile uint32_t TIME_DIER; /* 0x1C8 */
    volatile uint32_t TIME_CNT;  /* 0x1CC */
    volatile uint32_t TIME_PER;  /* 0x1D0 */
    volatile uint32_t TIME_CMP1; /* 0x1D4 */
    volatile uint32_t TIME_CMP2; /* 0x1D8 */
    volatile uint32_t TIME_CMP3; /* 0x1DC */
    volatile uint32_t TIME_CMP4; /* 0x1E0 */
    volatile uint32_t SETE1R;    /* 0x1E4 */
    volatile uint32_t SETE2R;    /* 0x1E8 */
    volatile uint32_t RSTE1R;    /* 0x1EC */
    volatile uint32_t RSTE2R;    /* 0x1F0 */
    /* Common output enable */
    volatile uint32_t OENR2;     /* 0x1F4 */
} HRTIM_TypeDef;

#define HRTIM1   ((HRTIM_TypeDef *) 0x40016800UL)

/* HRTIM bit definitions */
#define HRTIM_CR1_HRTIMEN    (1U << 0)  /* Enable HRTIM */
#define HRTIM_CR1_ADUSYNC    (1U << 1)  /* ADC update sync */

/* Timer CR bits */
#define HRTIM_TIM_CR_CONT    (1U << 3)  /* Continuous mode */
#define HRTIM_TIM_CR_RETRIG  (1U << 1)  /* Retriggerable mode */
#define HRTIM_TIM_CR_SYNC    (1U << 20) /* Synchronous start */

/* Set/Reset register source values */
#define HRTIM_RST_PER        0x01   /* Reset on period */
#define HRTIM_SET_CMP2       0x02   /* Set on CMP2 */
#define HRTIM_RST_CMP4       0x04   /* Reset on CMP4 */
#define HRTIM_SET_NONE       0x00
#define HRTIM_RST_NONE       0x00

/* ---- SPI (for magnetometers) ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
} SPI_TypeDef;

#define SPI2     ((SPI_TypeDef *) 0x40003800UL)

/* SPI CR1 bits */
#define SPI_CR1_CPHA       (1U << 0)
#define SPI_CR1_CPOL       (1U << 1)
#define SPI_CR1_MSTR       (1U << 2)
#define SPI_CR1_BR_MASK    (0x07U << 3)
#define SPI_CR1_BR_DIV2   (0x00U << 3)
#define SPI_CR1_BR_DIV4   (0x01U << 3)
#define SPI_CR1_BR_DIV8   (0x02U << 3)
#define SPI_CR1_BR_DIV16  (0x03U << 3)
#define SPI_CR1_BR_DIV32  (0x04U << 3)
#define SPI_CR1_BR_DIV64  (0x05U << 3)
#define SPI_CR1_BR_DIV128 (0x06U << 3)
#define SPI_CR1_BR_DIV256 (0x07U << 3)
#define SPI_CR1_LSBFIRST   (1U << 7)
#define SPI_CR1_SSI        (1U << 8)
#define SPI_CR1_SSM        (1U << 9)
#define SPI_CR1_RXONLY     (1U << 10)
#define SPI_CR1_DFF        (1U << 11)
#define SPI_CR1_CRCNEXT    (1U << 12)
#define SPI_CR1_CRCEN      (1U << 13)
#define SPI_CR1_BIDIOE     (1U << 14)
#define SPI_CR1_BIDIMODE   (1U << 15)
#define SPI_CR1_SPE        (1U << 6)

/* SPI CR2 bits */
#define SPI_CR2_DS_8BIT    (0x07U << 8)
#define SPI_CR2_DS_16BIT   (0x0FU << 8)
#define SPI_CR2_FRXTH      (1U << 12)
#define SPI_CR2_NSSP       (1U << 3)
#define SPI_CR2_SSOE       (1U << 2)
#define SPI_CR2_TXEIE      (1U << 1)
#define SPI_CR2_RXNEIE     (1U << 0)

/* SPI SR bits */
#define SPI_SR_RXNE        (1U << 0)
#define SPI_SR_TXE         (1U << 1)
#define SPI_SR_BSY        (1U << 7)
#define SPI_SR_MODF       (1U << 9)
#define SPI_SR_OVR        (1U << 6)
#define SPI_SR_CRCERR     (1U << 4)

/* ---- I2C (for OLED, EEPROM, ADS1115) ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t OAR1;     /* 0x08 */
    volatile uint32_t OAR2;     /* 0x0C */
    volatile uint32_t TIMINGR;  /* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;      /* 0x18 */
    volatile uint32_t ICR;      /* 0x1C */
    volatile uint32_t PECR;     /* 0x20 */
    volatile uint32_t RXDR;     /* 0x24 */
    volatile uint32_t TXDR;     /* 0x28 */
} I2C_TypeDef;

#define I2C1     ((I2C_TypeDef *) 0x40005400UL)

/* I2C CR1 bits */
#define I2C_CR1_PE         (1U << 0)
#define I2C_CR1_TXIE       (1U << 1)
#define I2C_CR1_RXIE       (1U << 2)
#define I2C_CR1_ADDRIE     (1U << 3)
#define I2C_CR1_NACKIE     (1U << 4)
#define I2C_CR1_STOPIE     (1U << 5)
#define I2C_CR1_TCIE       (1U << 6)
#define I2C_CR1_ERRIE      (1U << 8)
#define I2C_CR1_DNF_MASK   (0x0FU << 8)
#define I2C_CR1_ANFOFF     (1U << 12)
#define I2C_CR1_TXDMAEN    (1U << 14)
#define I2C_CR1_RXDMAEN    (1U << 15)

/* I2C CR2 bits */
#define I2C_CR2_START      (1U << 13)
#define I2C_CR2_STOP       (1U << 14)
#define I2C_CR2_NACK       (1U << 15)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_RELOAD     (1U << 24)
#define I2C_CR2_AUTOEND    (1U << 25)
#define I2C_CR2_RD_WRN     (1U << 10)
#define I2C_CR2_10BIT_ADDR (1U << 11)
#define I2C_CR2_SADD_MASK  0x3FF

/* I2C ISR bits */
#define I2C_ISR_TXE        (1U << 0)
#define I2C_ISR_TXIS       (1U << 1)
#define I2C_ISR_RXNE       (1U << 2)
#define I2C_ISR_ADDR       (1U << 3)
#define I2C_ISR_NACKF      (1U << 4)
#define I2C_ISR_STOPF      (1U << 5)
#define I2C_ISR_TC         (1U << 6)
#define I2C_ISR_TCR        (1U << 7)
#define I2C_ISR_BERR       (1U << 8)
#define I2C_ISR_ARLO       (1U << 9)
#define I2C_ISR_OVR        (1U << 10)
#define I2C_ISR_PECERR     (1U << 11)
#define I2C_ISR_TIMEOUT    (1U << 12)
#define I2C_ISR_ALERT      (1U << 13)
#define I2C_ISR_BUSY       (1U << 15)

/* I2C ICR bits */
#define I2C_ICR_ADDRCF     (1U << 3)
#define I2C_ICR_NACKCF     (1U << 4)
#define I2C_ICR_STOPCF     (1U << 5)
#define I2C_ICR_BERRCF     (1U << 8)
#define I2C_ICR_ARLOCF     (1U << 9)
#define I2C_ICR_OVRCF      (1U << 10)
#define I2C_ICR_PECCF      (1U << 11)
#define I2C_ICR_TIMOUTCF   (1U << 12)
#define I2C_ICR_ALERTCF    (1U << 13)

/* ---- ADC (for current and voltage sensing) ---- */
typedef struct {
    volatile uint32_t ISR;      /* 0x00 */
    volatile uint32_t IER;      /* 0x04 */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t CFGR;     /* 0x0C */
    volatile uint32_t CFGR2;    /* 0x10 */
    volatile uint32_t SMPR1;    /* 0x14 */
    volatile uint32_t SMPR2;    /* 0x18 */
    volatile uint32_t TR1;      /* 0x20 */
    volatile uint32_t TR2;      /* 0x24 */
    volatile uint32_t TR3;      /* 0x28 */
    volatile uint32_t SQR1;     /* 0x30 */
    volatile uint32_t SQR2;     /* 0x34 */
    volatile uint32_t SQR3;     /* 0x38 */
    volatile uint32_t SQR4;     /* 0x3C */
    volatile uint32_t DR;       /* 0x40 */
    /* ... */
} ADC_TypeDef;

#define ADC1     ((ADC_TypeDef *) 0x50000000UL)
#define ADC2     ((ADC_TypeDef *) 0x50000100UL)

/* ADC ISR bits */
#define ADC_ISR_ADRDY      (1U << 0)
#define ADC_ISR_EOC        (1U << 2)
#define ADC_ISR_EOS        (1U << 3)
#define ADC_ISR_OVR        (1U << 4)
#define ADC_ISR_JEOC       (1U << 7)
#define ADC_ISR_AWD1       (1U << 7)
#define ADC_ISR_AWD2       (1U << 8)
#define ADC_ISR_AWD3       (1U << 9)

/* ADC CR bits */
#define ADC_CR_ADEN        (1U << 0)
#define ADC_CR_ADDIS       (1U << 1)
#define ADC_CR_ADSTART     (1U << 2)
#define ADC_CR_ADSTOP      (1U << 4)
#define ADC_CR_ADVREGEN    (1U << 28)
#define ADC_CR_DEEPPWD     (1U << 29)
#define ADC_CR_ADCAL       (1U << 31)

/* ---- USART (for BLE module) ---- */
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
} USART_TypeDef;

#define USART1   ((USART_TypeDef *) 0x40013800UL)
#define USART2   ((USART_TypeDef *) 0x40004400UL)

/* USART CR1 bits */
#define USART_CR1_UE       (1U << 0)
#define USART_CR1_RE       (1U << 2)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_IDLEIE   (1U << 4)
#define USART_CR1_RXNEIE   (1U << 5)
#define USART_CR1_TCIE     (1U << 6)
#define USART_CR1_TXEIE    (1U << 7)
#define USART_CR1_PEIE     (1U << 8)
#define USART_CR1_PS       (1U << 9)
#define USART_CR1_PCE      (1U << 10)
#define USART_CR1_WAKE     (1U << 11)
#define USART_CR1_M0       (1U << 12)
#define USART_CR1_M1       (1U << 28)
#define USART_CR1_OVER8    (1U << 15)

/* USART ISR bits */
#define USART_ISR_PE       (1U << 0)
#define USART_ISR_FE       (1U << 1)
#define USART_ISR_NE       (1U << 2)
#define USART_ISR_ORE      (1U << 3)
#define USART_ISR_IDLE     (1U << 4)
#define USART_ISR_RXNE     (1U << 5)
#define USART_ISR_TC       (1U << 6)
#define USART_ISR_TXE      (1U << 7)
#define USART_ISR_BUSY     (1U << 16)
#define USART_ISR_CMF      (1U << 17)

/* ---- TIM (for rotary encoder) ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SMCR;     /* 0x08 */
    volatile uint32_t DIER;     /* 0x0C */
    volatile uint32_t SR;       /* 0x10 */
    volatile uint32_t EGR;      /* 0x14 */
    volatile uint32_t CCMR1;    /* 0x18 */
    volatile uint32_t CCMR2;    /* 0x1C */
    volatile uint32_t CCER;     /* 0x20 */
    volatile uint32_t CNT;      /* 0x24 */
    volatile uint32_t PSC;      /* 0x28 */
    volatile uint32_t ARR;      /* 0x2C */
    volatile uint32_t CCR1;     /* 0x30 */
    volatile uint32_t CCR2;     /* 0x34 */
    volatile uint32_t CCR3;     /* 0x38 */
    volatile uint32_t CCR4;     /* 0x3C */
} TIM_TypeDef;

#define TIM3     ((TIM_TypeDef *) 0x40000400UL)

/* TIM SMCR bits (encoder mode) */
#define TIM_SMCR_SMS_ENCODER1  0x01  /* TI1 edge counts, direction on TI2 */
#define TIM_SMCR_SMS_ENCODER2  0x02
#define TIM_SMCR_SMS_ENCODER3  0x03  /* Both edges, both channels */
#define TIM_SMCR_CEN           (1U << 0)

/* TIM CCMR bits */
#define TIM_CCMR1_CC1S_INPUT_TI2  (0x01U << 0)
#define TIM_CCMR1_CC2S_INPUT_TI1  (0x01U << 8)

/* ---- NVIC (Nested Vectored Interrupt Controller) ---- */
#define NVIC_BASE           0xE000E100UL
#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x00))
#define NVIC_ICER0          (*(volatile uint32_t *)(NVIC_BASE + 0x80))
#define NVIC_IPR_BASE       ((volatile uint32_t *)(NVIC_BASE + 0x300))

/* IRQ numbers for STM32G474 */
#define HRTIM1_Master_IRQn  112
#define HRTIM1_TIMA_IRQn    113
#define HRTIM1_TIMB_IRQn    114
#define HRTIM1_TIMC_IRQn    115
#define HRTIM1_TIMD_IRQn    116
#define HRTIM1_TIME_IRQn    117
#define HRTIM1_FLT_IRQn     118
#define ADC1_2_IRQn         18
#define SPI2_IRQn           36
#define USART1_IRQn         37
#define TIM3_IRQn           29

/* ---- SysTick ---- */
#define SysTick_BASE        0xE000E010UL
typedef struct {
    volatile uint32_t CTRL;     /* 0x00 */
    volatile uint32_t LOAD;     /* 0x04 */
    volatile uint32_t VAL;       /* 0x08 */
    volatile uint32_t CALIB;    /* 0x0C */
} SysTick_TypeDef;

#define SysTick   ((SysTick_TypeDef *) SysTick_BASE)
#define SysTick_CTRL_ENABLE  (1U << 0)
#define SysTick_CTRL_TICKINT (1U << 1)
#define SysTick_CTRL_CLKSOURCE (1U << 2)

/* ---- Flash (for firmware) ---- */
#define FLASH_BASE          0x08000000UL
#define FLASH_SIZE          (512 * 1024)  /* 512 KB on STM32G474VET6 */

/* ---- Helper Macros ---- */
#define REG32(addr)         (*(volatile uint32_t *)(addr))
#define READ_REG(reg)       ((uint32_t)(reg))
#define WRITE_REG(reg, val) ((reg) = (uint32_t)(val))
#define SET_BIT(reg, bit)   ((reg) |= (uint32_t)(bit))
#define CLEAR_BIT(reg, bit) ((reg) &= ~(uint32_t)(bit))
#define MODIFY_REG(reg, clr, set) ((reg) = (((reg) & ~(clr)) | (set)))

/* ---- Vector Table ---- */
typedef void (*isr_handler_t)(void);
extern isr_handler_t __vector_table[];

#endif /* REGISTERS_H */