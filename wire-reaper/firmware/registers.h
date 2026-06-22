/*
 * registers.h — STM32H743 register definitions for WireReaper
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Minimal set of MMIO register structures for the peripherals used
 * by WireReaper. Intentionally avoids pulling in the full ST HAL.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ---- Generic register access helpers ---- */
#define REG32(addr)     (*(volatile uint32_t *)(addr))
#define REG16(addr)     (*(volatile uint16_t *)(addr))
#define REG8(addr)      (*(volatile uint8_t  *)(addr))

#define SET_BIT(reg, b)     ((reg) |= (b))
#define CLR_BIT(reg, b)     ((reg) &= ~(b))
#define RD_BIT(reg, b)      ((reg) & (b))
#define WR_REG(reg, v)      ((reg) = (uint32_t)(v))

/* ---- RCC (Reset and Clock Control) ---- */
#define RCC_BASE        0x58024400UL
#define RCC_CR          REG32(RCC_BASE + 0x00)
#define RCC_PLLCFGR     REG32(RCC_BASE + 0x08)
#define RCC_CFGR        REG32(RCC_BASE + 0x10)
#define RCC_AHB1ENR     REG32(RCC_BASE + 0xD0)  /* GPIO ports A-K */
#define RCC_AHB2ENR     REG32(RCC_BASE + 0xD4)  /* USB, RNG, etc. */
#define RCC_AHB3ENR     REG32(RCC_BASE + 0xD8)  /* FMC */
#define RCC_AHB4ENR     REG32(RCC_BASE + 0xDC)  /* GPIO H-K, BDMA */
#define RCC_APB1ENR1    REG32(RCC_BASE + 0xE0)  /* USART2/3, UART4/5, I2C1/2/3 */
#define RCC_APB1ENR2    REG32(RCC_BASE + 0xE4)  /* UART8/9, SDMMC1? */
#define RCC_APB2ENR     REG32(RCC_BASE + 0xE8)  /* USART1, SPI1, SPI4, TIM1-17 */
#define RCC_APB3ENR     REG32(RCC_BASE + 0xEC)  /* LTDC, DSI */
#define RCC_AHB1RSTR    REG32(RCC_BASE + 0x20)
#define RCC_AHB3RSTR    REG32(RCC_BASE + 0x28)
#define RCC_APB1RSTR1   REG32(RCC_BASE + 0x40)
#define RCC_APB2RSTR    REG32(RCC_BASE + 0x48)

/* RCC AHB1ENR bit masks (GPIO clocks) */
#define RCC_AHB1ENR_GPIOA    (1U << 0)
#define RCC_AHB1ENR_GPIOB    (1U << 1)
#define RCC_AHB1ENR_GPIOC    (1U << 2)
#define RCC_AHB1ENR_GPIOD    (1U << 3)
#define RCC_AHB1ENR_GPIOE    (1U << 4)
#define RCC_AHB1ENR_GPIOF    (1U << 5)
#define RCC_AHB1ENR_GPIOG    (1U << 6)
#define RCC_AHB1ENR_GPIOH    (1U << 7)

/* RCC AHB2ENR bit masks */
#define RCC_AHB2ENR_USB      (1U << 0)

/* RCC AHB3ENR bit masks */
#define RCC_AHB3ENR_FMC      (1U << 0)

/* RCC APB1ENR1 bit masks */
#define RCC_APB1ENR_USART2   (1U << 17)
#define RCC_APB1ENR_USART3   (1U << 18)
#define RCC_APB1ENR_UART4    (1U << 19)
#define RCC_APB1ENR_UART5    (1U << 20)
#define RCC_APB1ENR_I2C1     (1U << 21)
#define RCC_APB1ENR_I2C2     (1U << 22)
#define RCC_APB1ENR_I2C3     (1U << 23)
#define RCC_APB1ENR_TIM2     (1U << 0)
#define RCC_APB1ENR_TIM6     (1U << 4)
#define RCC_APB1ENR_TIM7     (1U << 5)

/* RCC APB2ENR bit masks */
#define RCC_APB2ENR_USART1   (1U << 4)
#define RCC_APB2ENR_SPI1     (1U << 12)
#define RCC_APB2ENR_SPI4     (1U << 13)
#define RCC_APB2ENR_TIM1     (1U << 0)
#define RCC_APB2ENR_TIM8     (1U << 1)

/* RCC AHB4ENR for GPIO H-K + SDMMC */
#define RCC_AHB4ENR_SDMMC1   (1U << 16)

/* ---- PWR (Power Control) ---- */
#define PWR_BASE        0x58024800UL
#define PWR_CR1         REG32(PWR_BASE + 0x00)
#define PWR_CR3         REG32(PWR_BASE + 0x08)
#define PWR_D3CR        REG32(PWR_BASE + 0x0C)

/* ---- GPIO register layout ---- */
typedef struct {
    volatile uint32_t MODER;    /* 0x00: Mode register */
    volatile uint32_t OTYPER;   /* 0x04: Output type register */
    volatile uint32_t OSPEEDR;  /* 0x08: Output speed register */
    volatile uint32_t PUPDR;    /* 0x0C: Pull-up/pull-down register */
    volatile uint32_t IDR;      /* 0x10: Input data register */
    volatile uint32_t ODR;      /* 0x14: Output data register */
    volatile uint32_t BSRR;     /* 0x18: Bit set/reset register */
    volatile uint32_t LCKR;     /* 0x1C: Lock register */
    volatile uint32_t AFR[2];   /* 0x20-0x24: Alternate function (low/high) */
    volatile uint32_t BRR;      /* 0x28: Bit reset register */
    volatile uint32_t SECCFGR;  /* 0x2C: Secure config */
} gpio_t;

#define GPIO(port) ((gpio_t *)(port))

/* GPIO mode values */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT    0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG    0x03

#define GPIO_OTYPE_PP       0x00
#define GPIO_OTYPE_OD       0x01

#define GPIO_OSPEED_LOW     0x00
#define GPIO_OSPEED_MID     0x01
#define GPIO_OSPEED_HIGH    0x02
#define GPIO_OSPEED_VHIGH   0x03

#define GPIO_PUPD_NONE      0x00
#define GPIO_PUPD_PULLUP    0x01
#define GPIO_PUPD_PULLDOWN  0x02

/* ---- USART register layout ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00: Control register 1 */
    volatile uint32_t CR2;      /* 0x04: Control register 2 */
    volatile uint32_t CR3;      /* 0x08: Control register 3 */
    volatile uint32_t BRR;      /* 0x0C: Baud rate */
    volatile uint32_t GTPR;     /* 0x10: Guard time / prescaler */
    volatile uint32_t RTOR;     /* 0x14: Receiver timeout */
    volatile uint32_t RQR;      /* 0x18: Request register */
    volatile uint32_t ISR;      /* 0x1C: Interrupt & status */
    volatile uint32_t ICR;      /* 0x20: Interrupt flag clear */
    volatile uint32_t RDR;      /* 0x24: Receive data */
    volatile uint32_t TDR;      /* 0x28: Transmit data */
} usart_t;

#define USART(port) ((usart_t *)(port))

/* USART CR1 bits */
#define USART_CR1_UE        (1U << 0)   /* USART enable */
#define USART_CR1_RE        (1U << 2)   /* Receiver enable */
#define USART_CR1_TE        (1U << 3)   /* Transmitter enable */
#define USART_CR1_RXNEIE    (1U << 5)   /* RXNE interrupt enable */
#define USART_CR1_TCIE      (1U << 6)   /* Transmission complete IE */
#define USART_CR1_IDLEIE    (1U << 4)   /* IDLE interrupt enable */

/* USART ISR bits */
#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TC        (1U << 6)
#define USART_ISR_TXE       (1U << 7)
#define USART_ISR_IDLE      (1U << 4)
#define USART_ISR_ORE       (1U << 3)
#define USART_ISR_FE        (1U << 2)
#define USART_ISR_NE        (1U << 1)

/* USART CR3 bits */
#define USART_CR3_DMAT      (1U << 7)
#define USART_CR3_DMAR      (1U << 6)
#define USART_CR3_CTSE      (1U << 9)
#define USART_CR3_RTSE      (1U << 8)

/* ---- I2C register layout ---- */
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
} i2c_t;

#define I2C(port) ((i2c_t *)(port))

/* I2C CR1 bits */
#define I2C_CR1_PE          (1U << 0)
#define I2C_CR1_TXIE        (1U << 1)
#define I2C_CR1_RXIE        (1U << 2)
#define I2C_CR1_ADDRIE      (1U << 3)
#define I2C_CR1_NACKIE      (1U << 4)
#define I2C_CR1_STOPIE      (1U << 5)
#define I2C_CR1_TXDMAEN     (1U << 23)
#define I2C_CR1_RXDMAEN     (1U << 15)
#define I2C_CR1_ANFOFF      (1U << 12)
#define I2C_CR1_DNF         (0xFU << 8)

/* I2C CR2 bits */
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_NACK        (1U << 15)
#define I2C_CR2_RELOAD      (1U << 24)
#define I2C_CR2_AUTOEND     (1U << 25)
#define I2C_CR2_NBYTES_MASK (0xFFU << 16)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_RD_WRN      (1U << 10)

/* I2C ISR bits */
#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_TXIS        (1U << 1)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_ADDR        (1U << 3)
#define I2C_ISR_NACKF       (1U << 4)
#define I2C_ISR_STOPF       (1U << 5)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_TCR         (1U << 7)
#define I2C_ISR_BUSY        (1U << 15)

/* I2C ICR bits */
#define I2C_ICR_ADDRCF      (1U << 3)
#define I2C_ICR_NACKCF      (1U << 4)
#define I2C_ICR_STOPCF      (1U << 5)

/* ---- SPI register layout ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C (actually at 0x0C, scaled) */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
    volatile uint32_t I2SCFGR;  /* 0x1C */
    volatile uint32_t I2SPR;    /* 0x20 */
    volatile uint32_t CFG1;     /* 0x04 (H7 has extended config) */
} spi_t;

/* For STM32H7, the SPI register map differs from F1/F4. Simplified: */
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_BR_SHIFT    3
#define SPI_CR1_BR_MASK     (0x7U << 3)
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_LSBFIRST    (1U << 7)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR1_RXONLY      (1U << 10)
#define SPI_CR1_CRCL        (1U << 11)

#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_BSY          (1U << 7)
#define SPI_SR_MODF         (1U << 4)
#define SPI_SR_OVR          (1U << 6)

/* ---- DMA (DMA1, DMA2) ---- */
#define DMA1_BASE       0x40020000UL
#define DMA2_BASE       0x40020400UL

typedef struct {
    volatile uint32_t LISR;     /* 0x00: Low interrupt status */
    volatile uint32_t HISR;     /* 0x04: High interrupt status */
    volatile uint32_t LIFCR;    /* 0x08: Low flag clear */
    volatile uint32_t HIFCR;    /* 0x0C: High flag clear */
} dma_common_t;

typedef struct {
    volatile uint32_t CR;       /* 0x00: Stream config */
    volatile uint32_t NDTR;     /* 0x04: Number of data */
    volatile uint32_t PAR;      /* 0x08: Peripheral address */
    volatile uint32_t M0AR;     /* 0x0C: Memory address 0 */
    volatile uint32_t M1AR;     /* 0x10: Memory address 1 */
    volatile uint32_t FCR;      /* 0x14: FIFO control */
} dma_stream_t;

/* DMA stream CR bits */
#define DMA_CR_EN          (1U << 0)
#define DMA_CR_DMEIE       (1U << 2)
#define DMA_CR_TEIE        (1U << 3)
#define DMA_CR_HTIE        (1U << 4)
#define DMA_CR_TCIE        (1U << 5)
#define DMA_CR_PFCTRL      (1U << 5)
#define DMA_CR_DIR_SHIFT   6
#define DMA_CR_DIR_MASK    (0x3U << 6)
#define DMA_CR_DIR_P2M     (0x0U << 6)
#define DMA_CR_DIR_M2P     (0x1U << 6)
#define DMA_CR_DIR_M2M     (0x2U << 6)
#define DMA_CR_CIRC        (1U << 8)
#define DMA_CR_MINC        (1U << 10)
#define DMA_CR_PINC        (1U << 9)
#define DMA_CR_PSIZE_SHIFT 11
#define DMA_CR_MSIZE_SHIFT 13
#define DMA_CR_PSIZE_8BIT  (0x0U << 11)
#define DMA_CR_PSIZE_16BIT (0x1U << 11)
#define DMA_CR_PSIZE_32BIT (0x2U << 11)
#define DMA_CR_PL_SHIFT    16
#define DMA_CR_PL_LOW      (0x0U << 16)
#define DMA_CR_PL_MED      (0x1U << 16)
#define DMA_CR_PL_HIGH     (0x2U << 16)
#define DMA_CR_PL_VHIGH    (0x3U << 16)

/* ---- FMC (Flexible Memory Controller) for SDRAM ---- */
#define FMC_Bank1_BASE      (FMC_BASE + 0x0000)  /* NOR/PSRAM */
#define FMC_Bank5_6_BASE    (FMC_BASE + 0x0140)  /* SDRAM */

typedef struct {
    volatile uint32_t SDCR1;    /* 0x40: SDRAM control reg 1 */
    volatile uint32_t SDCR2;    /* 0x44: SDRAM control reg 2 */
    volatile uint32_t SDTR1;    /* 0x48: SDRAM timing reg 1 */
    volatile uint32_t SDTR2;    /* 0x4C: SDRAM timing reg 2 */
    volatile uint32_t SDCMR;    /* 0x50: SDRAM command mode */
    volatile uint32_t SDRTR;    /* 0x54: SDRAM refresh timer */
    volatile uint32_t SDSR;     /* 0x58: SDRAM status */
} fmc_sdram_t;

#define FMC_SDRAM ((fmc_sdram_t *)FMC_Bank5_6_BASE)

/* SDCMR command values */
#define FMC_SDCMR_MODE_NORMAL    (0U << 0)
#define FMC_SDCMR_MODE_CLK       (1U << 0)
#define FMC_SDCMR_MODE_PALL      (2U << 0)
#define FMC_SDCMR_MODE_AUTO      (3U << 0)
#define FMC_SDCMR_MODE_WRITE     (4U << 0)
#define FMC_SDCMR_MODE_SELFREF   (5U << 0)
#define FMC_SDCMR_MODE_MASK      (7U << 0)

/* ---- USB OTG (USB3300 ULPI) ---- */
#define USB_OTG_BASE     0x40080000UL
#define USB_OTG_GUSBCFG  REG32(USB_OTG_BASE + 0x00)
#define USB_OTG_GINTSTS  REG32(USB_OTG_BASE + 0x0C)
#define USB_OTG_GINTMSK  REG32(USB_OTG_BASE + 0x10)
#define USB_OTG_GRSTCTL  REG32(USB_OTG_BASE + 0x14)
#define USB_OTG_GCCFG    REG32(USB_OTG_BASE + 0x38)

/* ---- TIM (basic timer for 1-Wire and timeouts) ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
} tim_t;

#define TIM1_BASE   0x40010000UL
#define TIM2_BASE   0x40000000UL
#define TIM6_BASE   0x40001000UL
#define TIM7_BASE   0x40001400UL
#define TIM(port) ((tim_t *)(port))

#define TIM_CR1_CEN     (1U << 0)
#define TIM_CR1_ARPE    (1U << 7)
#define TIM_DIER_UIE    (1U << 0)
#define TIM_SR_UIF      (1U << 0)

/* ---- NVIC (Nested Vectored Interrupt Controller) ---- */
#define NVIC_BASE       0xE000E100UL
#define NVIC_ISER0      REG32(NVIC_BASE + 0x000)
#define NVIC_ISER1      REG32(NVIC_BASE + 0x004)
#define NVIC_ICER0      REG32(NVIC_BASE + 0x080)
#define NVIC_ICPR0      REG32(NVIC_BASE + 0x180)

/* IRQ numbers (STM32H743) */
#define IRQ_USART1      37
#define IRQ_USART2      38
#define IRQ_USART3      39
#define IRQ_UART4       52
#define IRQ_UART5       53
#define IRQ_I2C1_EV     31
#define IRQ_I2C1_ER     32
#define IRQ_I2C2_EV     33
#define IRQ_I2C2_ER     34
#define IRQ_I2C3_EV     72
#define IRQ_I2C3_ER     73
#define IRQ_DMA1_STR0   11
#define IRQ_DMA1_STR1   12
#define IRQ_DMA1_STR2   13
#define IRQ_DMA1_STR3   14
#define IRQ_DMA1_STR4   15
#define IRQ_DMA1_STR5   16
#define IRQ_DMA1_STR6   17
#define IRQ_DMA1_STR7   47
#define IRQ_DMA2_STR0   56
#define IRQ_DMA2_STR1   57
#define IRQ_DMA2_STR2   58
#define IRQ_DMA2_STR3   59
#define IRQ_DMA2_STR4   60
#define IRQ_DMA2_STR5   68
#define IRQ_DMA2_STR6   69
#define IRQ_DMA2_STR7   70
#define IRQ_TIM6_DAC    54
#define IRQ_TIM7        55

static inline void nvic_enable(int irq) {
    if (irq < 32)
        NVIC_ISER0 = (1U << irq);
    else
        NVIC_ISER1 = (1U << (irq - 32));
}

static inline void nvic_clear(int irq) {
    if (irq < 32)
        NVIC_ICPR0 = (1U << irq);
}

/* ---- SysTick ---- */
#define SYSTICK_BASE    0xE000E010UL
#define SYST_CSR        REG32(SYSTICK_BASE + 0x00)
#define SYST_RVR        REG32(SYSTICK_BASE + 0x04)
#define SYST_CVR        REG32(SYSTICK_BASE + 0x08)
#define SYSTICK_ENABLE  (1U << 0)
#define SYSTICK_CLKSRC  (1U << 2)
#define SYSTICK_TICKINT (1U << 1)

/* ---- Flash controller (for latency) ---- */
#define FLASH_BASE      0x52002000UL
#define FLASH_ACR       REG32(FLASH_BASE + 0x00)

/* ---- IRQ prototype helper ---- */
#define IRQ_HANDLER(name) void name(void)

#endif /* REGISTERS_H */