/*
 * tesla-phantom — registers.h
 * STM32H743 register definitions for the
 * Electromagnetic Fault Injection & Magnetic Side-Channel Analysis Platform.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TESLA_PHANTOM_REGISTERS_H
#define TESLA_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ── Peripheral base addresses ────────────────────────────────────── */
#define PERIPH_BASE        0x40000000UL
#define APB1PERIPH_BASE    PERIPH_BASE
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE    (PERIPH_BASE + 0x08020000UL)
#define AHB3PERIPH_BASE    (PERIPH_BASE + 0x10000000UL)

/* GPIO ports */
#define GPIOA_BASE         (AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASE         (AHB1PERIPH_BASE + 0x0400)
#define GPIOC_BASE         (AHB1PERIPH_BASE + 0x0800)
#define GPIOD_BASE         (AHB1PERIPH_BASE + 0x0C00)
#define GPIOE_BASE         (AHB1PERIPH_BASE + 0x1000)
#define GPIOF_BASE         (AHB1PERIPH_BASE + 0x1400)
#define GPIOG_BASE         (AHB1PERIPH_BASE + 0x1800)
#define GPIOH_BASE         (AHB1PERIPH_BASE + 0x1C00)
#define GPIOI_BASE         (AHB1PERIPH_BASE + 0x2000)

/* RCC */
#define RCC_BASE           (AHB1PERIPH_BASE + 0x4400)

/* PWR */
#define PWR_BASE           (AHB1PERIPH_BASE + 0x4800)

/* SYSCFG */
#define SYSCFG_BASE        (APB2PERIPH_BASE + 0x0000)

/* DMA */
#define DMA1_BASE          (AHB1PERIPH_BASE + 0x6000)
#define DMA2_BASE          (AHB1PERIPH_BASE + 0x7000)

/* DMAMUX */
#define DMAMUX1_BASE       (AHB1PERIPH_BASE + 0x7800)

/* SPI */
#define SPI1_BASE          (APB2PERIPH_BASE + 0x3000)
#define SPI2_BASE          (APB1PERIPH_BASE + 0x3800)
#define SPI4_BASE          (APB2PERIPH_BASE + 0x3400)

/* USART */
#define USART2_BASE        (APB1PERIPH_BASE + 0x2400)
#define USART3_BASE        (APB1PERIPH_BASE + 0x2800)

/* I2C */
#define I2C1_BASE          (APB1PERIPH_BASE + 0x5400)

/* DAC */
#define DAC1_BASE          (AHB1PERIPH_BASE + 0x6800)

/* ADC */
#define ADC1_BASE          (AHB1PERIPH_BASE + 0x6400)
#define ADC12_COMMON       (AHB1PERIPH_BASE + 0x6600)

/* TIM */
#define TIM1_BASE          (APB2PERIPH_BASE + 0x0000)
#define TIM2_BASE          (APB1PERIPH_BASE + 0x0000)
#define TIM3_BASE          (APB1PERIPH_BASE + 0x0400)
#define TIM4_BASE          (APB1PERIPH_BASE + 0x0800)
#define TIM6_BASE          (APB1PERIPH_BASE + 0x1000)
#define TIM7_BASE          (APB1PERIPH_BASE + 0x1400)

/* FMC */
#define FMC_BASE           (AHB3PERIPH_BASE + 0x4000)

/* QSPI */
#define OCTOSPI1_BASE      (AHB3PERIPH_BASE + 0x0000)

/* EXTI */
#define EXTI_BASE          (APB2PERIPH_BASE + 0x0400)

/* SysTick */
#define SYSTICK_BASE       (0xE000E010UL)

/* ── GPIO register layout ─────────────────────────────────────────── */
typedef struct {
    volatile uint32_t MODER;     /* 0x00 mode register         */
    volatile uint32_t OTYPER;    /* 0x04 output type           */
    volatile uint32_t OSPEEDR;   /* 0x08 output speed          */
    volatile uint32_t PUPDR;     /* 0x0C pull-up/pull-down     */
    volatile uint32_t IDR;       /* 0x10 input data            */
    volatile uint32_t ODR;       /* 0x14 output data           */
    volatile uint32_t BSRR;      /* 0x18 bit set/reset         */
    volatile uint32_t LCKR;      /* 0x1C lock                  */
    volatile uint32_t AFR[2];    /* 0x20-0x24 alternate function */
    volatile uint32_t BRR;       /* 0x28 bit reset             */
} gpio_regs_t;

#define GPIO_MODE_INPUT    0
#define GPIO_MODE_OUTPUT   1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG   3

#define GPIO_PUPD_NONE     0
#define GPIO_PUPD_PULLUP   1
#define GPIO_PUPD_PULLDOWN 2

#define GPIO_SPEED_LOW     0
#define GPIO_SPEED_MED     1
#define GPIO_SPEED_HIGH    2
#define GPIO_SPEED_VHIGH   3

#define GPIO_OTYPE_PP      0
#define GPIO_OTYPE_OD      1

#define GPIO_AF0           0
#define GPIO_AF5           5   /* SPI1/SPI2   */
#define GPIO_AF6           6   /* SPI2/SPI4   */
#define GPIO_AF7           7   /* USART2/3    */
#define GPIO_AF9           9   /* QSPI        */
#define GPIO_AF10          10  /* QSPI        */
#define GPIO_AF12          12  /* FMC         */

#define GPIO(port)         ((gpio_regs_t *)(GPIO ## port ## _BASE))

static inline void gpio_set(volatile gpio_regs_t *g, uint32_t pin) {
    g->BSRR = (1U << pin);
}
static inline void gpio_clr(volatile gpio_regs_t *g, uint32_t pin) {
    g->BSRR = (1U << (pin + 16));
}
static inline uint32_t gpio_get(volatile gpio_regs_t *g, uint32_t pin) {
    return (g->IDR >> pin) & 1U;
}
static inline void gpio_cfg_output(volatile gpio_regs_t *g, uint32_t pin,
                                    uint32_t speed, uint32_t otype) {
    g->MODER = (g->MODER & ~(3U << (pin*2))) | (GPIO_MODE_OUTPUT << (pin*2));
    g->OSPEEDR = (g->OSPEEDR & ~(3U << (pin*2))) | (speed << (pin*2));
    g->OTYPER = (g->OTYPER & ~(1U << pin)) | (otype << pin);
}
static inline void gpio_cfg_input(volatile gpio_regs_t *g, uint32_t pin,
                                   uint32_t pupd) {
    g->MODER = (g->MODER & ~(3U << (pin*2))) | (GPIO_MODE_INPUT << (pin*2));
    g->PUPDR = (g->PUPDR & ~(3U << (pin*2))) | (pupd << (pin*2));
}
static inline void gpio_cfg_af(volatile gpio_regs_t *g, uint32_t pin,
                                uint32_t af, uint32_t speed) {
    g->MODER = (g->MODER & ~(3U << (pin*2))) | (GPIO_MODE_AF << (pin*2));
    g->OSPEEDR = (g->OSPEEDR & ~(3U << (pin*2))) | (speed << (pin*2));
    g->PUPDR = (g->PUPDR & ~(3U << (pin*2))) | (GPIO_PUPD_NONE << (pin*2));
    int idx = (pin < 8) ? 0 : 1;
    int shift = (pin % 8) * 4;
    g->AFR[idx] = (g->AFR[idx] & ~(0xFU << shift)) | ((uint32_t)af << shift);
}
static inline void gpio_cfg_analog(volatile gpio_regs_t *g, uint32_t pin) {
    g->MODER = (g->MODER & ~(3U << (pin*2))) | (GPIO_MODE_ANALOG << (pin*2));
    g->PUPDR = (g->PUPDR & ~(3U << (pin*2))) | (GPIO_PUPD_NONE << (pin*2));
}

/* ── RCC register layout ──────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR;       /* 0x00 clock control register    */
    volatile uint32_t HSICFGR;  /* 0x04 HSI config                */
    volatile uint32_t CRRCR;    /* 0x08 CRS clock rc              */
    volatile uint32_t CSICFGR;  /* 0x0C CSI config                */
    volatile uint32_t CFGR;     /* 0x10 clock config              */
    volatile uint32_t RCC_RESERVED1;
    volatile uint32_t D1CFGR;   /* 0x18 domain 1 config           */
    volatile uint32_t D2CFGR;   /* 0x1C domain 2 config           */
    volatile uint32_t D3CFGR;   /* 0x20 domain 3 config           */
    volatile uint32_t PLLCKSELR;/* 0x24 PLL clock sel             */
    volatile uint32_t PLLCFGR;  /* 0x28 PLL config                */
    volatile uint32_t PLL1DIVR; /* 0x2C PLL1 div                  */
    volatile uint32_t PLL1FRACR;/* 0x30 PLL1 frac                 */
    volatile uint32_t PLL2DIVR; /* 0x34 PLL2 div                  */
    volatile uint32_t PLL2FRACR;/* 0x38 PLL2 frac                 */
    volatile uint32_t PLL3DIVR; /* 0x3C PLL3 div                  */
    volatile uint32_t PLL3FRACR;/* 0x40 PLL3 frac                 */
    volatile uint32_t RESERVED1;
    volatile uint32_t CDCCIPR;  /* 0x48 clocks domain clk sel     */
    volatile uint32_t RESERVED2;
    volatile uint32_t CDCCIP1R; /* 0x50                           */
    volatile uint32_t CDCCIP2R; /* 0x54                           */
    volatile uint32_t RESERVED3;
    volatile uint32_t SrdCCIPR;  /* 0x5C                          */
    volatile uint32_t RESERVED4[4];
    volatile uint32_t CIER;     /* 0x7C interrupt enable          */
    volatile uint32_t CIFR;     /* 0x80 interrupt flags           */
    volatile uint32_t CICR;     /* 0x84 interrupt clear           */
    volatile uint32_t RESERVED5;
    volatile uint32_t BDCR;     /* 0x8C backup domain             */
    volatile uint32_t CSR;      /* 0x90 control/status            */
} rcc_regs_t;

#define RCC ((rcc_regs_t *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION    (1U << 0)
#define RCC_CR_HSIRDY   (1U << 2)
#define RCC_CR_HSEON    (1U << 16)
#define RCC_CR_HSERDY   (1U << 17)
#define RCC_CR_HSEBYP   (1U << 18)
#define RCC_CR_CSSON    (1U << 19)
#define RCC_CR_PLL1ON   (1U << 24)
#define RCC_CR_PLL1RDY  (1U << 25)

/* RCC CFGR bits */
#define RCC_CFGR_SW_HSI     0
#define RCC_CFGR_SW_HSE     2
#define RCC_CFGR_SW_PLL1    3

/* PLLCKSELR */
#define RCC_PLLSRC_HSE      (2U << 0)
#define RCC_PLLSRC_CSI      (1U << 0)
#define RCC_PLLSRC_HSI      (0U << 0)

/* ── SPI register layout ──────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 control register 1     */
    volatile uint32_t CR2;       /* 0x04 control register 2     */
    volatile uint32_t SR;        /* 0x08 status register        */
    volatile uint32_t DR;        /* 0x0C data register          */
    volatile uint32_t CRCPR;     /* 0x10 CRC polynomial         */
    volatile uint32_t RXCRCR;    /* 0x14 RX CRC                 */
    volatile uint32_t TXCRCR;    /* 0x18 TX CRC                 */
    volatile uint32_t I2SCFGR;   /* 0x1C I2S config             */
    volatile uint32_t I2SPR;     /* 0x20 I2S prescaler          */
} spi_regs_t;

#define SPI_CR1_CPHA       (1U << 0)
#define SPI_CR1_CPOL       (1U << 1)
#define SPI_CR1_MSTR       (1U << 2)
#define SPI_CR1_BR_DIV2    (0U << 3)
#define SPI_CR1_BR_DIV4    (1U << 3)
#define SPI_CR1_BR_DIV8    (2U << 3)
#define SPI_CR1_BR_DIV16   (3U << 3)
#define SPI_CR1_BR_DIV32   (4U << 3)
#define SPI_CR1_BR_DIV64   (5U << 3)
#define SPI_CR1_BR_DIV128  (6U << 3)
#define SPI_CR1_BR_DIV256  (7U << 3)
#define SPI_CR1_SPE        (1U << 6)
#define SPI_CR1_LSBFIRST   (1U << 7)
#define SPI_CR1_SSM        (1U << 9)
#define SPI_CR1_SSI        (1U << 10)

#define SPI_CR2_DS_8BIT    (7U << 8)
#define SPI_CR2_DS_16BIT   (15U << 8)
#define SPI_CR2_RXNEIE     (1U << 0)
#define SPI_CR2_TXEIE      (1U << 1)
#define SPI_CR2_RXDMAEN    (1U << 0)
#define SPI_CR2_TXDMAEN    (1U << 1)

#define SPI_SR_RXNE        (1U << 0)
#define SPI_SR_TXE         (1U << 1)
#define SPI_SR_BSY         (1U << 7)
#define SPI_SR_MODF        (1U << 4)
#define SPI_SR_OVR         (1U << 6)

#define SPI1 ((spi_regs_t *)SPI1_BASE)
#define SPI2 ((spi_regs_t *)SPI2_BASE)
#define SPI4 ((spi_regs_t *)SPI4_BASE)

static inline void spi_wait_tx(volatile spi_regs_t *s) {
    while (!(s->SR & SPI_SR_TXE)) { }
}
static inline void spi_wait_rx(volatile spi_regs_t *s) {
    while (!(s->SR & SPI_SR_RXNE)) { }
}
static inline void spi_wait_busy(volatile spi_regs_t *s) {
    while (s->SR & SPI_SR_BSY) { }
}
static inline uint16_t spi_xfer(volatile spi_regs_t *s, uint16_t data) {
    spi_wait_tx(s);
    s->DR = data;
    spi_wait_rx(s);
    return (uint16_t)s->DR;
}

/* ── USART register layout ────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 control 1     */
    volatile uint32_t CR2;       /* 0x04 control 2     */
    volatile uint32_t CR3;       /* 0x08 control 3     */
    volatile uint32_t BRR;       /* 0x0C baudrate      */
    volatile uint32_t GTPR;      /* 0x10 guard / presc */
    volatile uint32_t RTOR;      /* 0x14 rx timeout    */
    volatile uint32_t RQR;       /* 0x18 request       */
    volatile uint32_t ISR;       /* 0x1C int status    */
    volatile uint32_t ICR;       /* 0x20 int clear     */
    volatile uint32_t RDR;       /* 0x24 rx data       */
    volatile uint32_t TDR;       /* 0x28 tx data       */
} usart_regs_t;

#define USART_CR1_UE       (1U << 0)
#define USART_CR1_RE       (1U << 2)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_RXNEIE   (1U << 5)
#define USART_CR1_TCIE     (1U << 6)
#define USART_CR1_PCE      (1U << 10)
#define USART_CR1_PS       (1U << 9)
#define USART_CR1_M0       (1U << 12)
#define USART_CR1_OVER8    (1U << 15)

#define USART_ISR_RXNE     (1U << 5)
#define USART_ISR_TXE      (1U << 7)
#define USART_ISR_TC       (1U << 6)
#define USART_ISR_BUSY     (1U << 16)
#define USART_ISR_FE       (1U << 1)
#define USART_ISR_NE       (1U << 2)
#define USART_ISR_ORE      (1U << 3)

#define USART2 ((usart_regs_t *)USART2_BASE)
#define USART3 ((usart_regs_t *)USART3_BASE)

static inline void usart_wait_tx(volatile usart_regs_t *u) {
    while (!(u->ISR & USART_ISR_TXE)) { }
}
static inline void usart_wait_rx(volatile usart_regs_t *u) {
    while (!(u->ISR & USART_ISR_RXNE)) { }
}
static inline void usart_write(volatile usart_regs_t *u, uint8_t b) {
    usart_wait_tx(u);
    u->TDR = b;
}
static inline uint8_t usart_read(volatile usart_regs_t *u) {
    usart_wait_rx(u);
    return (uint8_t)u->RDR;
}

/* ── I2C register layout (simplified) ─────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 control 1   */
    volatile uint32_t CR2;       /* 0x04 control 2   */
    volatile uint32_t OAR1;      /* 0x08 own addr 1  */
    volatile uint32_t OAR2;      /* 0x0C own addr 2  */
    volatile uint32_t TIMINGR;   /* 0x10 timing      */
    volatile uint32_t TIMEOUTR;  /* 0x14 timeout     */
    volatile uint32_t ISR;       /* 0x18 int status  */
    volatile uint32_t ICR;       /* 0x1C int clear   */
    volatile uint32_t PECR;      /* 0x20 PEC          */
    volatile uint32_t RXDR;      /* 0x24 rx data      */
    volatile uint32_t TXDR;      /* 0x28 tx data      */
} i2c_regs_t;

#define I2C_CR1_PE         (1U << 0)
#define I2C_CR2_START      (1U << 13)
#define I2C_CR2_STOP       (1U << 14)
#define I2C_CR2_ACK        (1U << 11)
#define I2C_CR2_NBYTES_MASK (0xFFU << 16)
#define I2C_ISR_RXNE       (1U << 2)
#define I2C_ISR_TXIS       (1U << 1)
#define I2C_ISR_TC         (1U << 6)
#define I2C_ISR_BUSY       (1U << 15)
#define I2C_ISR_NACKF      (1U << 4)

#define I2C1 ((i2c_regs_t *)I2C1_BASE)

/* ── DAC register layout ──────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR;        /* 0x00 control       */
    volatile uint32_t SWTRIGR;   /* 0x04 sw trigger    */
    volatile uint32_t DHR12R1;   /* 0x08 12-bit right ch1 */
    volatile uint32_t DHR12L1;   /* 0x0C 12-bit left  ch1 */
    volatile uint32_t DHR8R1;    /* 0x10 8-bit         ch1 */
    volatile uint32_t DHR12R2;   /* 0x14 12-bit right ch2 */
    volatile uint32_t DHR12L2;   /* 0x18 12-bit left  ch2 */
    volatile uint32_t DHR8R2;    /* 0x1C 8-bit         ch2 */
    volatile uint32_t DOR1;      /* 0x20 output        ch1 */
    volatile uint32_t DOR2;      /* 0x24 output        ch2 */
} dac_regs_t;

#define DAC_CR_EN1         (1U << 0)
#define DAC_CR_BOFF1       (1U << 13) /* buffer output enable    */
#define DAC_CR_TSEL1_SW    (0U << 3)  /* software trigger         */

#define DAC1 ((dac_regs_t *)DAC1_BASE)

/* ── SysTick ──────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CSR;       /* 0x00 control & status  */
    volatile uint32_t LOAD;      /* 0x04 reload value      */
    volatile uint32_t VAL;       /* 0x08 current value     */
    volatile uint32_t CALIB;     /* 0x0C calibration       */
} systick_regs_t;

#define SYSTICK ((systick_regs_t *)SYSTICK_BASE)

#define SYSTICK_CSR_ENABLE   (1U << 0)
#define SYSTICK_CSR_TICKINT  (1U << 1)
#define SYSTICK_CSR_CLKSRC   (1U << 2)

/* ── EXTI ─────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t SEC1;      /* 0x00 rising edge     */
    volatile uint32_t SEC2;      /* 0x04 falling edge    */
    volatile uint32_t EXTICR1;   /* 0x08-0x0B            */
    volatile uint32_t EXTICR2;
    volatile uint32_t EXTICR3;
    volatile uint32_t EXTICR4;
    volatile uint32_t IMR1;      /* 0x20 interrupt mask  */
    volatile uint32_t EMR1;      /* 0x24 event mask      */
    volatile uint32_t IMR2;
    volatile uint32_t EMR2;
    volatile uint32_t PR1;       /* 0x38 pending (clear on write) */
    volatile uint32_t PR2;
} exti_regs_t;

#define EXTI ((exti_regs_t *)EXTI_BASE)

/* ── DMA (simplified — stream layout) ─────────────────────────────── */
typedef struct {
    volatile uint32_t CR;        /* 0x00 config      */
    volatile uint32_t NDTR;      /* 0x04 count       */
    volatile uint32_t PAR;       /* 0x08 periph addr */
    volatile uint32_t M0AR;      /* 0x0C mem addr 0  */
    volatile uint32_t M1AR;      /* 0x10 mem addr 1  */
    volatile uint32_t FCR;       /* 0x14 FIFO ctrl   */
} dma_stream_t;

typedef struct {
    dma_stream_t S[8];           /* 8 streams per DMA controller */
    volatile uint32_t LISR;      /* 0x00 low int status  */
    volatile uint32_t HISR;      /* 0x04 high int status */
    volatile uint32_t LIFCR;     /* 0x08 low int clear   */
    volatile uint32_t HIFCR;     /* 0x0C high int clear  */
    /* Note: LISR offset from base is 0x000, stream regs at 0x010+ */
} dma_regs_t;

/* The actual layout has LISR at base+0x00 then streams at 0x010+ */
#define DMA2 ((volatile uint8_t *)DMA2_BASE)

#define DMA_SxCR_EN     (1U << 0)
#define DMA_SxCR_TCIE   (1U << 4)
#define DMA_SxCR_HTIE   (1U << 3)
#define DMA_SxCR_DIR_P2M (2U << 6)   /* peripheral-to-memory   */
#define DMA_SxCR_MINC   (1U << 10)   /* memory increment       */
#define DMA_SxCR_PINC   (1U << 9)    /* periph increment       */
#define DMA_SxCR_PSIZE_16 (1U << 11)
#define DMA_SxCR_MSIZE_16 (1U << 13)
#define DMA_SxCR_CHSEL(s) ((s) << 25)

/* ── FMC (simplified — bank1 NOR/PSRAM) ──────────────────────────── */
typedef struct {
    volatile uint32_t BCR1;      /* 0x00 bank 1 ctrl    */
    volatile uint32_t BTR1;      /* 0x04 bank 1 timing  */
    volatile uint32_t BCR2;
    volatile uint32_t BTR2;
    volatile uint32_t BCR3;
    volatile uint32_t BTR3;
    volatile uint32_t BCR4;
    volatile uint32_t BTR4;
} fmc_bank1_regs_t;

#define FMC_BANK1 ((fmc_bank1_regs_t *)(FMC_BASE))

#define FMC_BCR1_MBKEN    (1U << 0)
#define FMC_BCR1_MWID_16  (1U << 4)
#define FMC_BCR1_MTYP_SR  (0U << 2)
#define FMC_BCR1_WREN     (1U << 12)

/* ── Timer registers (simplified) ─────────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 control 1     */
    volatile uint32_t CR2;       /* 0x04 control 2     */
    volatile uint32_t SMCR;      /* 0x08 slave mode    */
    volatile uint32_t DIER;      /* 0x0C DMA/int en    */
    volatile uint32_t SR;        /* 0x10 status        */
    volatile uint32_t EGR;       /* 0x14 event gen     */
    volatile uint32_t CCMR1;     /* 0x18               */
    volatile uint32_t CCMR2;     /* 0x1C               */
    volatile uint32_t CCER;      /* 0x20               */
    volatile uint32_t CNT;       /* 0x24 counter       */
    volatile uint32_t PSC;       /* 0x28 prescaler     */
    volatile uint32_t ARR;       /* 0x2C auto-reload   */
    volatile uint32_t CCR1;      /* 0x30               */
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
} tim_regs_t;

#define TIM_CR1_CEN      (1U << 0)
#define TIM_DIER_UIE     (1U << 0)
#define TIM_SR_UIF       (1U << 0)

#define TIM2 ((tim_regs_t *)TIM2_BASE)
#define TIM6 ((tim_regs_t *)TIM6_BASE)

/* ── NVIC (simplified) ────────────────────────────────────────────── */
#define NVIC_BASE_ADDR   0xE000E100UL
#define NVIC_ISER(n)     (*(volatile uint32_t *)(NVIC_BASE_ADDR + (n)*4))
#define NVIC_ICPR(n)     (*(volatile uint32_t *)(NVIC_BASE_ADDR + 0x80 + (n)*4))

/* ── IRQ numbers ──────────────────────────────────────────────────── */
#define IRQ_EXTI4        12      /* EXTI line 4 (external trigger)     */
#define IRQ_DMA2_STR0    56      /* DMA2 stream 0 (ADC capture)        */
#define IRQ_USART3       39      /* USART3 (BLE)                       */
#define IRQ_TIM2         28      /* TIM2 (scan timing)                 */
#define IRQ_SPI1         35      /* SPI1 (FPGA)                        */

#endif /* TESLA_PHANTOM_REGISTERS_H */