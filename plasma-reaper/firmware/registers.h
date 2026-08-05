/*
 * registers.h — MCU register definitions for PlasmaReaper
 * Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This file provides register-level definitions for the STM32H723
 * peripherals used by PlasmaReaper. In a real build these would be
 * provided by the STM32 HAL / CMSIS headers; here we define the
 * minimal set needed to compile the driver code standalone.
 */

#ifndef PLASMA_REAPER_REGISTERS_H
#define PLASMA_REAPER_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ------------------------------------------------ */

#define PERIPH_BASE         0x40000000UL
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)
#define AHB2_BASE           (PERIPH_BASE + 0x00000000UL)
#define AHB4_BASE           (PERIPH_BASE + 0x00040000UL)
#define APB1_BASE           (PERIPH_BASE + 0x00010000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00020000UL) /* alias of AHB1 */

/* ---- RCC ----------------------------------------------------------- */

#define RCC_BASE            (AHB1_BASE + 0x1000UL)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD0))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD4))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_AHB1RSTR        (*(volatile uint32_t *)(RCC_BASE + 0x80))
#define RCC_AHB4RSTR        (*(volatile uint32_t *)(RCC_BASE + 0x88))
#define RCC_APB1RSTR        (*(volatile uint32_t *)(RCC_BASE + 0x90))
#define RCC_APB2RSTR        (*(volatile uint32_t *)(RCC_BASE + 0x94))

/* RCC enable bits */
#define RCC_AHB1ENR_GPIOA   (1U << 0)
#define RCC_AHB1ENR_GPIOB   (1U << 1)
#define RCC_AHB1ENR_GPIOC   (1U << 2)
#define RCC_AHB1ENR_GPIOD   (1U << 3)
#define RCC_AHB1ENR_GPIOE   (1U << 4)
#define RCC_AHB2ENR_USB     (1U << 0)
#define RCC_AHB4ENR_GPIOA_EN (1U << 0) /* AHB4 GPIOA clock */
#define RCC_AHB4ENR_GPIOB_EN (1U << 1)
#define RCC_AHB4ENR_GPIOC_EN (1U << 2)
#define RCC_AHB4ENR_GPIOD_EN (1U << 3)
#define RCC_AHB4ENR_GPIOE_EN (1U << 4)
#define RCC_APB1ENR_USART2  (1U << 17)
#define RCC_APB1ENR_USART3  (1U << 18)
#define RCC_APB1ENR_I2C1    (1U << 21)
#define RCC_APB2ENR_USART1  (1U << 4)
#define RCC_APB2ENR_SPI1    (1U << 12)
#define RCC_APB2ENR_SPI2    (1U << 14)
#define RCC_APB2ENR_ADC1    (1U << 8)

/* HRTIM clock enable — RCC_APB2ENR bit 23 (HRTIM1) */
#define RCC_APB2ENR_HRTIM1  (1U << 23)

/* ---- GPIO ---------------------------------------------------------- */

#define GPIOA_BASE          (AHB4_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB4_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB4_BASE + 0x0800UL)
#define GPIOD_BASE          (AHB4_BASE + 0x0C00UL)
#define GPIOE_BASE          (AHB4_BASE + 0x1000UL)

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
} gpio_regs_t;

#define GPIOA               ((gpio_regs_t *)GPIOA_BASE)
#define GPIOB               ((gpio_regs_t *)GPIOB_BASE)
#define GPIOC               ((gpio_regs_t *)GPIOC_BASE)
#define GPIOD               ((gpio_regs_t *)GPIOD_BASE)
#define GPIOE               ((gpio_regs_t *)GPIOE_BASE)

/* GPIO mode values */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT    0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG   0x03

#define GPIO_OSPEED_LOW     0x00
#define GPIO_OSPEED_MED     0x01
#define GPIO_OSPEED_HIGH    0x02
#define GPIO_OSPEED_VHIGH   0x03

#define GPIO_PUPD_NONE      0x00
#define GPIO_PUPD_PULLUP    0x01
#define GPIO_PUPD_PULLDOWN  0x02

/* ---- USART --------------------------------------------------------- */

#define USART1_BASE         (APB2_BASE + 0x1000UL)  /* 0x40011000 alias */
#define USART2_BASE         (APB1_BASE + 0x4400UL)
#define USART3_BASE         (APB1_BASE + 0x4800UL)

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
} usart_regs_t;

#define USART1              ((usart_regs_t *)USART1_BASE)
#define USART2              ((usart_regs_t *)USART2_BASE)
#define USART3              ((usart_regs_t *)USART3_BASE)

#define USART_CR1_UE        (1U << 0)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_RXNEIE    (1U << 5)
#define USART_CR1_TCIE      (1U << 6)
#define USART_CR1_IDLEIE    (1U << 4)
#define USART_CR3_DMAR      (1U << 6)
#define USART_CR3_DMAT      (1U << 7)
#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TC        (1U << 6)
#define USART_ISR_IDLE      (1U << 4)
#define USART_ISR_TXE       (1U << 7)

/* ---- SPI ----------------------------------------------------------- */

#define SPI1_BASE           (APB2_BASE + 0x3000UL)
#define SPI2_BASE           (APB1_BASE + 0x3800UL)

typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
} spi_regs_t;

#define SPI1                ((spi_regs_t *)SPI1_BASE)
#define SPI2                ((spi_regs_t *)SPI2_BASE)

#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_BR_DIV2     (0U << 3)
#define SPI_CR1_BR_DIV4     (1U << 3)
#define SPI_CR1_BR_DIV8     (2U << 3)
#define SPI_CR1_BR_DIV16    (3U << 3)
#define SPI_CR1_BR_DIV32    (4U << 3)
#define SPI_CR1_BR_DIV64    (5U << 3)
#define SPI_CR1_BR_DIV128   (6U << 3)
#define SPI_CR1_BR_DIV256   (7U << 3)
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_LSBFIRST    (1U << 7)
#define SPI_CR2_DS_8BIT     (7U << 8)
#define SPI_CR2_FRXTH       (1U << 2)
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_BSY          (1U << 7)

/* ---- HRTIM (High-Resolution Timer) --------------------------------- */

#define HRTIM1_BASE         (APB2_BASE + 0x6800UL)

/* HRTIM master timer */
#define HRTIM_MCR           (*(volatile uint32_t *)(HRTIM1_BASE + 0x00))
#define HRTIM_MISR          (*(volatile uint32_t *)(HRTIM1_BASE + 0x0C))
#define HRTIM_MICR          (*(volatile uint32_t *)(HRTIM1_BASE + 0x10))
#define HRTIM_MCNT          (*(volatile uint32_t *)(HRTIM1_BASE + 0x18))
#define HRTIM_MPER          (*(volatile uint32_t *)(HRTIM1_BASE + 0x14))
#define HRTIM_MREP          (*(volatile uint32_t *)(HRTIM1_BASE + 0x1C))
#define HRTIM_MCMP1R        (*(volatile uint32_t *)(HRTIM1_BASE + 0x20))
#define HRTIM_MCMP2R        (*(volatile uint32_t *)(HRTIM1_BASE + 0x24))
#define HRTIM_MCMP3R        (*(volatile uint32_t *)(HRTIM1_BASE + 0x28))
#define HRTIM_MCMP4R        (*(volatile uint32_t *)(HRTIM1_BASE + 0x2C))

/* Timer F (power glitch) register offsets from HRTIM1_BASE + 0x100 */
#define HRTIM_TIMF_BASE     (HRTIM1_BASE + 0x280UL)
#define HRTIM_TIMF_CR       (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x00))
#define HRTIM_TIMF_ISR      (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x08))
#define HRTIM_TIMF_ICR      (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x0C))
#define HRTIM_TIMF_CNT      (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x10))
#define HRTIM_TIMF_PER      (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x14))
#define HRTIM_TIMF_CMP1     (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x18))
#define HRTIM_TIMF_CMP2     (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x1C))
#define HRTIM_TIMF_OUT      (*(volatile uint32_t *)(HRTIM_TIMF_BASE + 0x24))

/* HRTIM enable bit in MCR */
#define HRTIM_MCR_HRTIM1EN  (1U << 0)

/* ---- DMA ----------------------------------------------------------- */

#define DMA1_BASE           (AHB1_BASE + 0x0000UL)
#define DMA1_STREAM0_BASE   (DMA1_BASE + 0x10UL)
#define DMA1_STREAM1_BASE   (DMA1_BASE + 0x24UL)
#define DMA1_STREAM2_BASE   (DMA1_BASE + 0x38UL)

typedef struct {
    volatile uint32_t PAR;      /* 0x00 — peripheral address */
    volatile uint32_t M0AR;     /* 0x04 — memory address 0    */
    volatile uint32_t M1AR;     /* 0x08 — memory address 1    */
    volatile uint32_t LCKCR;    /* 0x0C */
    volatile uint32_t LCKISR;   /* 0x10 */
} dma_chan_t; /* Note: This is simplified; real layout uses separate
               * LISR/HISR/LIFCR/HIFCR at stream base. */

/* DMA stream register block (STM32H7 layout) */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t NDTR;     /* 0x04 */
    volatile uint32_t PAR;      /* 0x08 */
    volatile uint32_t M0AR;     /* 0x0C */
    volatile uint32_t M1AR;     /* 0x10 */
    volatile uint32_t RESERVED; /* 0x14 */
    volatile uint32_t RESERVED2;/* 0x18 */
} dma_stream_t;

#define DMA1_Stream0       ((dma_stream_t *)(DMA1_BASE + 0x10UL))
#define DMA1_Stream1       ((dma_stream_t *)(DMA1_BASE + 0x24UL))
#define DMA1_Stream2       ((dma_stream_t *)(DMA1_BASE + 0x38UL))

/* DMA interrupt registers */
#define DMA1_LISR          (*(volatile uint32_t *)(DMA1_BASE + 0x00))
#define DMA1_HISR          (*(volatile uint32_t *)(DMA1_BASE + 0x04))
#define DMA1_LIFCR         (*(volatile uint32_t *)(DMA1_BASE + 0x08))
#define DMA1_HIFCR         (*(volatile uint32_t *)(DMA1_BASE + 0x0C))

#define DMA_SxCR_EN        (1U << 0)
#define DMA_SxCR_TCIE      (1U << 4)
#define DMA_SxCR_HTIE      (1U << 3)
#define DMA_SxCR_TEIE      (1U << 2)
#define DMA_SxCR_DMEIE     (1U << 1)
#define DMA_SxCR_DIR_P2M   (0x02 << 6) /* Peripheral-to-memory */
#define DMA_SxCR_MINC      (1U << 10)  /* Memory increment mode  */
#define DMA_SxCR_PSIZE_16  (1U << 9)   /* 16-bit peripheral      */
#define DMA_SxCR_MSIZE_16  (1U << 13)  /* 16-bit memory          */
#define DMA_SxCR_PRIO_HIGH (0x02 << 16)
#define DMA_SxCR_CIRC      (1U << 8)

/* ---- ADC1 ---------------------------------------------------------- */

#define ADC1_BASE           (AHB2_BASE + 0x1000UL)  /* simplified alias */
#define ADC1_CR             (*(volatile uint32_t *)(ADC1_BASE + 0x08))
#define ADC1_ISR            (*(volatile uint32_t *)(ADC1_BASE + 0x00))
#define ADC1_IER            (*(volatile uint32_t *)(ADC1_BASE + 0x04))
#define ADC1_SQR1           (*(volatile uint32_t *)(ADC1_BASE + 0x30))
#define ADC1_SQR3           (*(volatile uint32_t *)(ADC1_BASE + 0x38))
#define ADC1_DR             (*(volatile uint32_t *)(ADC1_BASE + 0x40))
#define ADC1_CFGR           (*(volatile uint32_t *)(ADC1_BASE + 0x0C))
#define ADC1_SMPR1          (*(volatile uint32_t *)(ADC1_BASE + 0x14))

#define ADC_CR_ADEN         (1U << 0)
#define ADC_CR_ADSTART      (1U << 2)
#define ADC_ISR_ADRDY       (1U << 0)
#define ADC_ISR_EOC         (1U << 2)
#define ADC_IER_EOCIE       (1U << 2)
#define ADC_CFGR_DMAEN      (1U << 0)
#define ADC_CFGR_CONT       (1U << 13)
#define ADC_CFGR_OVRMOD     (1U << 12)

/* ---- I2C1 ---------------------------------------------------------- */

#define I2C1_BASE           (APB1_BASE + 0x5400UL)

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
} i2c_regs_t;

#define I2C1                ((i2c_regs_t *)I2C1_BASE)

#define I2C_CR1_PE          (1U << 0)
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_RD_WRN      (1U << 10)
#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_NACKF       (1U << 4)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_NBYTES_MAX  0xFF

/* ---- USB ----------------------------------------------------------- */

#define USB_BASE            (AHB2_BASE + 0x0800UL)
/* USB registers are handled by the USB CDC driver; minimal defines here. */
#define USB_OTG_GOTGCTL     (*(volatile uint32_t *)(USB_BASE + 0x000))
#define USB_OTG_GAHBCFG     (*(volatile uint32_t *)(USB_BASE + 0x008))
#define USB_OTG_GUSBCFG     (*(volatile uint32_t *)(USB_BASE + 0x00C))
#define USB_OTG_GRSTCTL     (*(volatile uint32_t *)(USB_BASE + 0x010))
#define USB_OTG_GINTSTS     (*(volatile uint32_t *)(USB_BASE + 0x014))
#define USB_OTG_GINTMSK     (*(volatile uint32_t *)(USB_BASE + 0x018))

/* ---- NVIC ---------------------------------------------------------- */

#define NVIC_BASE           (0xE000E100UL)
#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ICER0          (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ISPR0          (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ICPR0          (*(volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_IPR0           (*(volatile uint32_t *)(NVIC_BASE + 0x300))

/* IRQ numbers (STM32H723) */
#define USART1_IRQn         37
#define USART2_IRQn         38
#define USART3_IRQn         39
#define DMA1_Stream0_IRQn   11
#define DMA1_Stream1_IRQn   12
#define DMA1_Stream2_IRQn   13
#define ADC1_IRQn           18
#define SPI1_IRQn           35

/* ---- SysTick ------------------------------------------------------- */

#define SYSTICK_BASE        0xE000E010UL
#define SYSTICK_CSR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_RVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_CVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CALIB       (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

#define SYSTICK_CSR_ENABLE  (1U << 0)
#define SYSTICK_CSR_TICKINT (1U << 1)
#define SYSTICK_CSR_CLKSRC  (1U << 2)

/* ---- Flash (for option bytes / RDP — reference only) --------------- */

#define FLASH_BASE          (AHB1_BASE + 0x2000UL)
#define FLASH_KEYR          (*(volatile uint32_t *)(FLASH_BASE + 0x08))
#define FLASH_SR            (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_CR            (*(volatile uint32_t *)(FLASH_BASE + 0x14))

/* ---- Helper macros ------------------------------------------------- */

#define BIT(n)              (1U << (n))
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#endif /* PLASMA_REAPER_REGISTERS_H */