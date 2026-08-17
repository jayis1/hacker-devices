/*
 * registers.h — STM32H743 register definitions (subset used by Pulse-Reaper)
 *
 * This is a minimal hand-maintained register map for the peripherals
 * actually used by the firmware. It avoids pulling in the full ST HAL
 * and keeps the code self-contained and auditable.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_REGISTERS_H
#define PULSEREAPER_REGISTERS_H

#include <stdint.h>

/* ----------------------------------------------------------------------- */
/*  Base addresses                                                         */
/* ----------------------------------------------------------------------- */

#define PERIPH_BASE         0x40000000u
#define AHB1_BASE           (PERIPH_BASE + 0x00020000u)
#define AHB2_BASE           (PERIPH_BASE + 0x00030000u)
#define AHB3_BASE           (PERIPH_BASE + 0x00040000u)
#define AHB4_BASE           (PERIPH_BASE + 0x58020000u)
#define APB4_BASE           (PERIPH_BASE + 0x58000000u)
#define APB1_BASE           (PERIPH_BASE + 0x00000000u)
#define APB1LR_BASE         APB1_BASE
#define APB1HR_BASE         (PERIPH_BASE + 0x00010000u)
#define APB2_BASE           (PERIPH_BASE + 0x00010000u)

/* RCC */
#define RCC_BASE            (AHB1_BASE + 0x4400u)

/* GPIO ports */
#define GPIOA_BASE          (AHB4_BASE + 0x0000u)
#define GPIOB_BASE          (AHB4_BASE + 0x0400u)
#define GPIOC_BASE          (AHB4_BASE + 0x0800u)
#define GPIOD_BASE          (AHB4_BASE + 0x0C00u)
#define GPIOE_BASE          (AHB4_BASE + 0x1000u)

/* SPI1 (APB2), SPI2 (APB1) */
#define SPI1_BASE           (APB2_BASE + 0x3000u)
#define SPI2_BASE           (APB1LR_BASE + 0x3800u)

/* USART3 (APB1L), LPUART1 (APB1H) */
#define USART3_BASE         (APB1LR_BASE + 0x4800u)
#define LPUART1_BASE        (APB1HR_BASE + 0x0C00u)

/* I2C1 (APB1L), I2C4 (APB4) */
#define I2C1_BASE           (APB1LR_BASE + 0x5400u)
#define I2C4_BASE           (APB4_BASE + 0x1C00u)

/* SDMMC1 (AHB2) */
#define SDMMC1_BASE         (AHB2_BASE + 0x2800u)

/* USB1 OTG */
#define USB1_BASE           (AHB1_BASE + 0x8000u)

/* DMA1, DMA2 stream registers (AHB1) */
#define DMA1_BASE           (AHB1_BASE + 0x6000u)
#define DMA2_BASE           (AHB1_BASE + 0x7000u)

/* DWT / ITM / SysTick */
#define DWT_BASE            0xE0001000u
#define SCB_BASE            0xE000ED00u
#define SYSTICK_BASE        0xE000E010u
#define NVIC_BASE           0xE000E100u

/* Flash controller (for option bytes / bank layout) */
#define FLASH_BASE          (AHB3_BASE + 0x2000u)

/* ----------------------------------------------------------------------- */
/*  Register access macros                                                 */
/* ----------------------------------------------------------------------- */

#define REG32(addr)        (*(volatile uint32_t *)(addr))
#define REG16(addr)        (*(volatile uint16_t *)(addr))
#define REG8(addr)        (*(volatile uint8_t  *)(addr))

#define SET_BIT(reg, mask)    ((reg) |= (mask))
#define CLR_BIT(reg, mask)    ((reg) &= ~(mask))
#define GET_BIT(reg, mask)    (((reg) & (mask)) != 0u)

/* ----------------------------------------------------------------------- */
/*  GPIO                                                                   */
/* ----------------------------------------------------------------------- */

typedef struct {
    volatile uint32_t MODER;     /* 0x00 */
    volatile uint32_t OTYPER;    /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;     /* 0x0C */
    volatile uint32_t IDR;       /* 0x10 */
    volatile uint32_t ODR;       /* 0x14 */
    volatile uint32_t BSRR;      /* 0x18 */
    volatile uint32_t LCKR;      /* 0x1C */
    volatile uint32_t AFRL;      /* 0x20 */
    volatile uint32_t AFRH;      /* 0x24 */
    volatile uint32_t BRR;       /* 0x28 */
    volatile uint32_t rsvd[243]; /* pad to 0x400 */
} gpio_reg_t;

#define GPIOA ((gpio_reg_t *)GPIOA_BASE)
#define GPIOB ((gpio_reg_t *)GPIOB_BASE)
#define GPIOC ((gpio_reg_t *)GPIOC_BASE)
#define GPIOD ((gpio_reg_t *)GPIOD_BASE)
#define GPIOE ((gpio_reg_t *)GPIOE_BASE)

#define GPIO_MODE_INPUT   0u
#define GPIO_MODE_OUTPUT  1u
#define GPIO_MODE_AF      2u
#define GPIO_MODE_ANALOG  3u

#define GPIO_OTYPE_PP     0u
#define GPIO_OTYPE_OD     1u

#define GPIO_OSPEED_LOW   0u
#define GPIO_OSPEED_MED   1u
#define GPIO_OSPEED_HIGH  2u
#define GPIO_OSPEED_VHIGH 3u

#define GPIO_PUPD_NONE    0u
#define GPIO_PUPD_UP      1u
#define GPIO_PUPD_DOWN    2u

/* ----------------------------------------------------------------------- */
/*  RCC — clock control (subset)                                           */
/* ----------------------------------------------------------------------- */

typedef struct {
    volatile uint32_t CR;        /* 0x00 */
    volatile uint32_t HSICFGR;   /* 0x04 */
    volatile uint32_t CRRCR;     /* 0x08 */
    volatile uint32_t CSICFGR;   /* 0x0C */
    volatile uint32_t CFGR;     /* 0x10 */
    volatile uint32_t rsvd1;     /* 0x14 */
    volatile uint32_t D1CFGR;    /* 0x18 */
    volatile uint32_t D2CFGR;    /* 0x1C */
    volatile uint32_t D3CFGR;    /* 0x20 */
    volatile uint32_t rsvd2;     /* 0x24 */
    volatile uint32_t CKCFGR;    /* 0x28 */
    volatile uint32_t D2CCIP1R;  /* 0x2C */
    volatile uint32_t D2CCIP2R;  /* 0x30 */
    volatile uint32_t D3CCIPR;   /* 0x34 */
    volatile uint32_t rsvd3[3];  /* 0x38..0x40 */
    volatile uint32_t PLLCKSELR; /* 0x44 */
    volatile uint32_t PLLCFGR;   /* 0x48 */
    volatile uint32_t PLL1DIVR;  /* 0x4C */
    volatile uint32_t PLL1FRACR; /* 0x50 */
    volatile uint32_t PLL2DIVR;  /* 0x54 */
    volatile uint32_t rsvd4[10]; /* pad */
    volatile uint32_t D1CCIPR;    /* offset varies — see RM0433 */
    /* ... truncated for brevity in this hand-maintained map ...           */
} rcc_reg_t;

#define RCC ((rcc_reg_t *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION    (1u << 0)
#define RCC_CR_HSIRDY   (1u << 1)
#define RCC_CR_HSEON    (1u << 16)
#define RCC_CR_HSERDY   (1u << 17)
#define RCC_CR_HSEBYP   (1u << 18)
#define RCC_CR_PLL1ON   (1u << 24)
#define RCC_CR_PLL1RDY  (1u << 25)

/* RCC CFGR bits (SW field) */
#define RCC_CFGR_SW_HSISYS  0u
#define RCC_CFGR_SW_HSE     2u
#define RCC_CFGR_SW_PLL1    3u
#define RCC_CFGR_SWS_MASK   (3u << 3)

/* PLLCKSELR */
#define RCC_PLLCKSELR_DIVM1_SHIFT 4u
#define RCC_PLLCKSELR_DIVM1_MASK  (0x3Fu << 4u)

/* PLL1DIVR */
#define RCC_PLL1DIVR_N1_SHIFT 8u
#define RCC_PLL1DIVR_N1_MASK  (0xFFu << 8u)

/* ----------------------------------------------------------------------- */
/*  SPI (subset)                                                           */
/* ----------------------------------------------------------------------- */

typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CFG1;      /* 0x08 */
    volatile uint32_t CFG2;      /* 0x0C */
    volatile uint32_t IER;       /* 0x10 */
    volatile uint32_t SR;        /* 0x14 */
    volatile uint32_t IFCR;      /* 0x18 */
    volatile uint32_t TXDR;     /* 0x1C — write */
    volatile uint32_t RXDR;     /* 0x20 — read  */
    volatile uint32_t rsvd[6];  /* pad */
} spi_reg_t;

#define SPI1 ((spi_reg_t *)SPI1_BASE)
#define SPI2 ((spi_reg_t *)SPI2_BASE)

#define SPI_CR1_SPE        (1u << 0)
#define SPI_CR1_CSTART     (1u << 7)
#define SPI_CR1_CSUSP      (1u << 8)
#define SPI_CR1_HDDIR      (1u << 9)
#define SPI_CR1_MASRX      (1u << 10)

#define SPI_CFG1_MBR_SHIFT 28u
#define SPI_CFG1_MBR_MASK  (7u << 28u)
#define SPI_CFG1_DSIZE_SHIFT 0u
#define SPI_CFG1_DSIZE_MASK  (0x3Fu)
#define SPI_CFG1_FTHLV_SHIFT 5u
#define SPI_CFG1_TXDMAEN  (1u << 15)
#define SPI_CFG1_RXDMAEN  (1u << 14)

#define SPI_CFG2_MASTER    (1u << 22)
#define SPI_CFG2_SSM       (1u << 0)
#define SPI_CFG2_SSI       (1u << 1)
#define SPI_CFG2_CPOL      (1u << 24)
#define SPI_CFG2_CPHA      (1u << 25)
#define SPI_CFG2_LSBFIRST  (1u << 26)

#define SPI_SR_EOT         (1u << 3)
#define SPI_SR_RXP         (1u << 0)
#define SPI_SR_TXP         (1u << 1)
#define SPI_SR_BUSY        (1u << 22)
#define SPI_SR_CRCERR      (1u << 4)
#define SPI_SR_MODF        (1u << 8)
#define SPI_SR_OVR         (1u << 6)

/* ----------------------------------------------------------------------- */
/*  USART (subset — used for BLE C2)                                        */
/* ----------------------------------------------------------------------- */

typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CR3;       /* 0x08 */
    volatile uint32_t BRR;       /* 0x0C */
    volatile uint32_t GTPR;      /* 0x10 */
    volatile uint32_t RTOR;      /* 0x14 */
    volatile uint32_t RQR;       /* 0x18 */
    volatile uint32_t ISR;       /* 0x1C */
    volatile uint32_t ICR;       /* 0x20 */
    volatile uint32_t RDR;       /* 0x24 */
    volatile uint32_t TDR;       /* 0x28 */
    volatile uint32_t rsvd[4];   /* pad */
} usart_reg_t;

#define USART3 ((usart_reg_t *)USART3_BASE)
#define LPUART1 ((usart_reg_t *)LPUART1_BASE)

#define USART_CR1_UE     (1u << 0)
#define USART_CR1_RE     (1u << 2)
#define USART_CR1_TE     (1u << 3)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_CR1_TCIE   (1u << 6)
#define USART_CR1_PCE     (1u << 8)
#define USART_CR1_PS      (1u << 9)
#define USART_CR1_OVER8   (1u << 15)

#define USART_CR2_STOP_SHIFT 12u
#define USART_CR2_STOP_1     0u

#define USART_CR3_RTSE   (1u << 8)
#define USART_CR3_CTSE   (1u << 9)
#define USART_CR3_DMAT   (1u << 7)
#define USART_CR3_DMAR   (1u << 6)
#define USART_CR3_HDSEL  (1u << 3)

#define USART_ISR_RXNE   (1u << 5)
#define USART_ISR_TXE    (1u << 7)
#define USART_ISR_TC     (1u << 6)
#define USART_ISR_BUSY   (1u << 16)
#define USART_ISR_FE     (1u << 1)
#define USART_ISR_ORE    (1u << 3)
#define USART_ISR_NE     (1u << 2)

/* ----------------------------------------------------------------------- */
/*  I2C (subset)                                                           */
/* ----------------------------------------------------------------------- */

typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t OAR1;      /* 0x08 */
    volatile uint32_t OAR2;      /* 0x0C */
    volatile uint32_t TIMINGR;   /* 0x10 */
    volatile uint32_t TIMEOUtr;  /* 0x14 */
    volatile uint32_t ISR;       /* 0x18 */
    volatile uint32_t ICR;       /* 0x1C */
    volatile uint32_t PECR;      /* 0x20 */
    volatile uint32_t RXDR;      /* 0x24 */
    volatile uint32_t TXDR;      /* 0x28 */
    volatile uint32_t rsvd[4];   /* pad */
} i2c_reg_t;

#define I2C1 ((i2c_reg_t *)I2C1_BASE)
#define I2C4 ((i2c_reg_t *)I2C4_BASE)

#define I2C_CR1_PE        (1u << 0)
#define I2C_CR1_TXIE      (1u << 1)
#define I2C_CR1_RXIE      (1u << 2)
#define I2C_CR1_STOPIE    (1u << 5)
#define I2C_CR1_NACKIE    (1u << 5)
#define I2C_CR2_NBYTES_SHIFT 16u
#define I2C_CR2_START     (1u << 13)
#define I2C_CR2_STOP      (1u << 14)
#define I2C_CR2_AUTOEND   (1u << 20)
#define I2C_CR2_RELOAD    (1u << 24)
#define I2C_ISR_TXIS      (1u << 1)
#define I2C_ISR_RXNE      (1u << 2)
#define I2C_ISR_STOPF     (1u << 5)
#define I2C_ISR_NACKF     (1u << 4)
#define I2C_ISR_BUSY      (1u << 15)

/* ----------------------------------------------------------------------- */
/*  SDMMC (subset)                                                          */
/* ----------------------------------------------------------------------- */

typedef struct {
    volatile uint32_t POWER;     /* 0x00 */
    volatile uint32_t CLKCR;     /* 0x04 */
    volatile uint32_t ARG;       /* 0x08 */
    volatile uint32_t CMD;        /* 0x0C */
    volatile uint32_t RESPCMD;    /* 0x10 — resp cmd  */
    volatile uint32_t RESP1;      /* 0x14 */
    volatile uint32_t RESP2;      /* 0x18 */
    volatile uint32_t RESP3;      /* 0x1C */
    volatile uint32_t RESP4;      /* 0x20 */
    volatile uint32_t DTIMER;     /* 0x24 */
    volatile uint32_t DLEN;       /* 0x28 */
    volatile uint32_t DCTRL;      /* 0x2C */
    volatile uint32_t DCOUNT;     /* 0x30 */
    volatile uint32_t STA;        /* 0x34 */
    volatile uint32_t ICR;        /* 0x38 */
    volatile uint32_t rsvd[2];    /* 0x3C..0x40 */
    volatile uint32_t FIFO;       /* 0x80 */
    volatile uint32_t rsvd2[31];  /* pad */
} sdmmc_reg_t;

#define SDMMC1 ((sdmmc_reg_t *)SDMMC1_BASE)

#define SDMMC_CLKCR_CLKEN   (1u << 8)
#define SDMMC_CLKCR_PWRSAV  (1u << 12)
#define SDMMC_DCTRL_DTEN    (1u << 0)
#define SDMMC_DCTRL_DTDIR   (1u << 1)
#define SDMMC_DCTRL_DMAEN   (1u << 3)
#define SDMMC_STA_CMDACT    (1u << 11)
#define SDMMC_STA_RXACT     (1u << 21)
#define SDMMC_STA_TXACT     (1u << 22)
#define SDMMC_STA_DATAEND   (1u << 8)
#define SDMMC_STA_RXFIFOE   (1u << 17)
#define SDMMC_STA_RXFIFOF   (1u << 19)
#define SDMMC_STA_TXFIFOE   (1u << 18)
#define SDMMC_STA_TXFIFOF  (1u << 20)

/* ----------------------------------------------------------------------- */
/*  DMA (subset — stream + channel config)                                  */
/* ----------------------------------------------------------------------- */

typedef struct {
    volatile uint32_t LPAR;     /* 0x00 — peripheral addr  */
    volatile uint32_t LAR;      /* 0x04 — memory addr      */
    volatile uint32_t NDTR;     /* 0x08 — count            */
    volatile uint32_t LFCR;     /* 0x0C — config           */
    volatile uint32_t LISR;      /* 0x10 — stream IRQ status (per stream) */
    volatile uint32_t rsvd[2];  /* pad */
} dma_stream_reg_t;

/* The actual DMA layout is more complex; this stub keeps the code readable
 * without pulling in the full ST HAL. See drivers/board_init.c for the
 * actual DMA stream configuration used for SPI1/FPGA and USART3/BLE.        */

/* ----------------------------------------------------------------------- */
/*  NVIC / SysTick                                                         */
/* ----------------------------------------------------------------------- */

#define NVIC_ISER0  REG32(NVIC_BASE + 0x000u)
#define NVIC_ICER0  REG32(NVIC_BASE + 0x080u)
#define NVIC_IPR0   REG8 (NVIC_BASE + 0x300u)

#define SCB_CPACR   REG32(SCB_BASE + 0x88u)
#define SCB_AIRCR   REG32(SCB_BASE + 0x0Cu)
#define SCB_SCR     REG32(SCB_BASE + 0x10u)

#define SYST_CSR    REG32(SYSTICK_BASE + 0x00u)
#define SYST_RVR    REG32(SYSTICK_BASE + 0x04u)
#define SYST_CVR    REG32(SYSTICK_BASE + 0x08u)

/* ----------------------------------------------------------------------- */
/*  DWT (data watchpoint & trace) — cycle counter                          */
/* ----------------------------------------------------------------------- */

#define DWT_CTRL    REG32(DWT_BASE + 0x000u)
#define DWT_CYCCNT  REG32(DWT_BASE + 0x004u)

#define DWT_CTRL_CYCCNTENA (1u << 0)

/* ----------------------------------------------------------------------- */
/*  IRQ numbers (subset)                                                   */
/* ----------------------------------------------------------------------- */

#define IRQ_DMA1_STREAM0   11u
#define IRQ_DMA1_STREAM1   12u
#define IRQ_SPI1           35u
#define IRQ_USART3         39u
#define IRQ_SDMMC1         88u
#define IRQ_I2C1_EV        31u
#define IRQ_EXTI0          6u
#define IRQ_EXTI1          7u
#define IRQ_EXTI4          10u
#define IRQ_EXTI9_5        23u

#endif /* PULSEREAPER_REGISTERS_H */