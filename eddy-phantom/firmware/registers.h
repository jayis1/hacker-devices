/*
 * eddy-phantom — registers.h
 * STM32H743 register definitions for the
 * Electromagnetic Emanation Keyboard Reconnaissance Device.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EDDY_PHANTOM_REGISTERS_H
#define EDDY_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ── GPIO register layout ─────────────────────────────────────── */
typedef struct {
    volatile uint32_t MODER;     /* 0x00 mode register         */
    volatile uint32_t OTYPER;    /* 0x04 output type            */
    volatile uint32_t OSPEEDR;   /* 0x08 output speed           */
    volatile uint32_t PUPDR;     /* 0x0C pull-up/pull-down      */
    volatile uint32_t IDR;       /* 0x10 input data             */
    volatile uint32_t ODR;       /* 0x14 output data            */
    volatile uint32_t BSRR;      /* 0x18 bit set/reset          */
    volatile uint32_t LCKR;      /* 0x1C lock                   */
    volatile uint32_t AFR[2];    /* 0x20-0x24 alternate function */
    volatile uint32_t BRR;       /* 0x28 bit reset              */
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

#define GPIO_OTYPE_PP      0  /* push-pull   */
#define GPIO_OTYPE_OD      1  /* open-drain  */

#define GPIO_AF0           0
#define GPIO_AF5           5   /* SPI1/SPI2    */
#define GPIO_AF6           6   /* SPI2/SPI4    */
#define GPIO_AF7           7   /* USART3       */
#define GPIO_AF9           9   /* QSPI         */
#define GPIO_AF10          10  /* QSPI         */
#define GPIO_AF12          12  /* FMC          */

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

/* ── SPI register layout ──────────────────────────────────────── */
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
#define SPI_CR1_RXDMAEN    (1U << 0)  /* in CR2 actually */
#define SPI_CR1_TXDMAEN    (1U << 1)

#define SPI_CR2_DS_8BIT    (7U << 8)
#define SPI_CR2_DS_16BIT   (15U << 8)
#define SPI_CR2_RXNEIE     (1U << 0)
#define SPI_CR2_TXEIE      (1U << 1)
#define SPI_CR2_FRXTH      (1U << 2)

#define SPI_SR_RXNE        (1U << 0)
#define SPI_SR_TXE         (1U << 1)
#define SPI_SR_BSY         (1U << 7)
#define SPI_SR_OVR         (1U << 6)
#define SPI_SR_MODF        (1U << 5)

#define SPI(port)          ((spi_regs_t *)(SPI ## port ## _BASE))

/* ── USART register layout ────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CR3;       /* 0x08 */
    volatile uint32_t BRR;       /* 0x0C baudrate              */
    volatile uint32_t GTPR;      /* 0x10 guard time / prescaler */
    volatile uint32_t RTOR;      /* 0x14 receiver timeout      */
    volatile uint32_t RQR;       /* 0x18 request               */
    volatile uint32_t ISR;       /* 0x1C interrupt status      */
    volatile uint32_t ICR;       /* 0x20 interrupt clear       */
    volatile uint32_t RDR;       /* 0x24 receive data          */
    volatile uint32_t TDR;       /* 0x28 transmit data         */
} usart_regs_t;

#define USART_CR1_UE       (1U << 0)
#define USART_CR1_RE       (1U << 2)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_RXNEIE   (1U << 5)
#define USART_CR1_TCIE     (1U << 6)
#define USART_CR1_TXEIE    (1U << 7)
#define USART_CR1_M0       (1U << 12)
#define USART_CR1_PCE      (1U << 10)
#define USART_CR1_OVER8    (1U << 15)

#define USART_ISR_RXNE     (1U << 5)
#define USART_ISR_TXE      (1U << 7)
#define USART_ISR_TC       (1U << 6)
#define USART_ISR_BUSY     (1U << 16)

#define USART(port)        ((usart_regs_t *)(USART ## port ## _BASE))

/* ── RCC register layout (simplified) ─────────────────────────── */
typedef struct {
    volatile uint32_t CR;        /* 0x00 clock control          */
    volatile uint32_t HSICFGR;   /* 0x04 HSI config             */
    volatile uint32_t CRRCR;     /* 0x08 CSI/HSI48              */
    volatile uint32_t CSICFGR;   /* 0x0C CSI config             */
    volatile uint32_t CFGR;      /* 0x10 clock config           */
    volatile uint32_t RCC_RESERVED1;
    volatile uint32_t D1CFGR;    /* 0x18 domain 1 prescalers    */
    volatile uint32_t D2CFGR;    /* 0x1C domain 2 prescalers    */
    volatile uint32_t D3CFGR;    /* 0x20 domain 3 prescalers    */
    volatile uint32_t RCC_RESERVED2;
    volatile uint32_t PLLCKSELR; /* 0x28 PLL clock source       */
    volatile uint32_t PLLCFGR;   /* 0x2C PLL config             */
    volatile uint32_t PLL1DIVR;  /* 0x30 PLL1 dividers          */
    volatile uint32_t PLL1FRACR; /* 0x34 PLL1 fractional        */
    volatile uint32_t PLL2DIVR;  /* 0x38 PLL2 dividers          */
    volatile uint32_t PLL2FRACR; /* 0x3C PLL2 fractional        */
    volatile uint32_t PLL3DIVR;  /* 0x40 PLL3 dividers          */
    volatile uint32_t PLL3FRACR; /* 0x44 PLL3 fractional        */
    volatile uint32_t RCC_RESERVED3[3];
    volatile uint32_t D1CCIPR;   /* 0x54 domain 1 clock sel     */
    volatile uint32_t D2CCIP1R;  /* 0x58 domain 2 clock sel 1   */
    volatile uint32_t D2CCIP2R;  /* 0x5C domain 2 clock sel 2   */
    volatile uint32_t D3CCIPR;   /* 0x60 domain 3 clock sel     */
    volatile uint32_t RCC_RESERVED4[2];
    volatile uint32_t CIER;      /* 0x6C clock interrupt enable */
    volatile uint32_t CICR;      /* 0x70 clock interrupt clear  */
    volatile uint32_t RCC_RESERVED5[4];
    volatile uint32_t BDCR;      /* 0x80 backup domain          */
    volatile uint32_t CSR;       /* 0x84 control/status         */
    volatile uint32_t RCC_RESERVED6[2];
    volatile uint32_t AHB3RSTR;  /* 0x8C AHB3 reset             */
    volatile uint32_t AHB1RSTR;  /* 0x90 AHB1 reset             */
    volatile uint32_t AHB2RSTR;  /* 0x94 AHB2 reset             */
    volatile uint32_t AHB4RSTR;  /* 0x98 AHB4 reset             */
    volatile uint32_t APB3RSTR;  /* 0x9C APB3 reset             */
    volatile uint32_t APB1LRSTR; /* 0xA0 APB1L reset            */
    volatile uint32_t APB1HRSTR; /* 0xA4 APB1H reset            */
    volatile uint32_t APB2RSTR;  /* 0xA8 APB2 reset             */
    volatile uint32_t APB4RSTR;  /* 0xAC APB4 reset             */
    volatile uint32_t RCC_RESERVED7[4];
    volatile uint32_t AHB3ENR;   /* 0xC4 AHB3 enable            */
    volatile uint32_t AHB1ENR;   /* 0xC8 AHB1 enable            */
    volatile uint32_t AHB2ENR;   /* 0xCC AHB2 enable            */
    volatile uint32_t AHB4ENR;   /* 0xD0 AHB4 enable            */
    volatile uint32_t APB3ENR;   /* 0xD4 APB3 enable            */
    volatile uint32_t APB1LENR;  /* 0xD8 APB1L enable           */
    volatile uint32_t APB1HENR;  /* 0xDC APB1H enable           */
    volatile uint32_t APB2ENR;   /* 0xE0 APB2 enable            */
    volatile uint32_t APB4ENR;   /* 0xE4 APB4 enable            */
} rcc_regs_t;

#define RCC                ((rcc_regs_t *)(RCC_BASE))

/* RCC bit definitions */
#define RCC_CR_HSION       (1U << 0)
#define RCC_CR_HSEON       (1U << 16)
#define RCC_CR_HSERDY      (1U << 17)
#define RCC_CR_PLL1ON      (1U << 24)
#define RCC_CR_PLL1RDY     (1U << 25)
#define RCC_CR_CSION       (1U << 7)
#define RCC_CR_CSIRDY      (1U << 8)

#define RCC_CFGR_SW_HSI    0
#define RCC_CFGR_SW_HSE    2
#define RCC_CFGR_SW_PLL1   3
#define RCC_CFGR_SWS_SHIFT 3
#define RCC_CFGR_SWS_PLL1  (3U << 3)

/* PLL configuration for 480 MHz from 25 MHz HSE:
 *   PLLM = 5  (25/5 = 5 MHz input)
 *   PLLN = 96 (5 * 96 = 480 MHz VCO)
 *   PLLP = 2  (480/2 = 240 MHz ... but we need DIVP=1 for 480)
 *   Actually: PLLN=192, PLLP=2 → VCO=960, /2=480 MHz
 *   PLLM=5, PLLN=192, PLLP=2, PLLQ=4 (120 MHz for USB),
 *   PLLR=2 (not used for sysclock)
 */
#define RCC_PLLCKSELR_DIVM1_5  (5U << 0)
#define RCC_PLLCFGR_PLL1SRC_HSE (2U << 0)
#define RCC_PLL1DIVR_N192      (191U << 0)  /* N-1 = 191 */
#define RCC_PLL1DIVR_P2        (1U << 9)    /* P-1 = 1   */
#define RCC_PLL1DIVR_Q4        (3U << 16)   /* Q-1 = 3   */
#define RCC_PLL1DIVR_R2        (1U << 24)   /* R-1 = 1   */

/* RCC enable bits */
#define RCC_AHB1ENR_DMA1EN     (1U << 0)
#define RCC_AHB1ENR_DMA2EN     (1U << 1)
#define RCC_AHB3ENR_FMCEN      (1U << 0)
#define RCC_AHB3ENR_QSPIEN     (1U << 1)
#define RCC_AHB4ENR_GPIOAEN    (1U << 0)
#define RCC_AHB4ENR_GPIOBEN    (1U << 1)
#define RCC_AHB4ENR_GPIOCEN    (1U << 2)
#define RCC_AHB4ENR_GPIODEN    (1U << 3)
#define RCC_AHB4ENR_GPIOEEN    (1U << 4)
#define RCC_AHB4ENR_GPIOFEN    (1U << 5)
#define RCC_AHB4ENR_GPIOGEN    (1U << 6)
#define RCC_AHB4ENR_GPIOHEN    (1U << 7)
#define RCC_APB1LENR_USART2EN  (1U << 17)
#define RCC_APB1LENR_USART3EN  (1U << 18)
#define RCC_APB1LENR_SPI2EN    (1U << 14)
#define RCC_APB2ENR_SPI1EN     (1U << 12)
#define RCC_APB2ENR_SPI4EN     (1U << 15)
#define RCC_APB2ENR_SYSCFGEN   (1U << 0)

/* ── PWR registers ────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CSR1;  /* 0x04 */
    volatile uint32_t CR2;   /* 0x08 */
    volatile uint32_t CR3;   /* 0x0C */
    volatile uint32_t CPUCR; /* 0x10 */
    volatile uint32_t RESERVED[3];
    volatile uint32_t SR1;   /* 0x20 */
    volatile uint32_t SR2;   /* 0x24 */
    volatile uint32_t SCR;   /* 0x28 */
    volatile uint32_t CR4;   /* 0x2C */
    /* ... more registers */
} pwr_regs_t;

#define PWR               ((pwr_regs_t *)(PWR_BASE))
#define PWR_CR3_SCUEN     (1U << 2)
#define PWR_CR3_LDOEN     (1U << 1)
#define PWR_CR1_VOS_SCALE0 (3U << 3)  /* VOS0 = highest performance */

/* ── DMA register layout (simplified) ─────────────────────────── */
typedef struct {
    volatile uint32_t CR;     /* 0x00 configuration           */
    volatile uint32_t NDTR;   /* 0x04 number of data          */
    volatile uint32_t PAR;    /* 0x08 peripheral address      */
    volatile uint32_t M0AR;   /* 0x0C memory address 0        */
    volatile uint32_t M1AR;   /* 0x10 memory address 1        */
    volatile uint32_t FCR;    /* 0x14 FIFO control            */
} dma_stream_regs_t;

typedef struct {
    dma_stream_regs_t stream[8]; /* 8 streams per DMA controller */
    volatile uint32_t RESERVED[8];
    volatile uint32_t LISR;   /* 0x00 low interrupt status (offset within) */
    volatile uint32_t HISR;   /* 0x04 high interrupt status */
    volatile uint32_t LIFCR;  /* low interrupt flag clear */
    volatile uint32_t HIFCR;  /* high interrupt flag clear */
} dma_regs_t;

/* DMA CR bit fields */
#define DMA_CR_EN          (1U << 0)
#define DMA_CR_DMEIE       (1U << 1)
#define DMA_CR_TEIE        (1U << 2)
#define DMA_CR_HTIE        (1U << 3)
#define DMA_CR_TCIE        (1U << 4)
#define DMA_CR_PFCTRL      (1U << 5)
#define DMA_CR_DIR_P2M     (0U << 6)   /* peripheral-to-memory */
#define DMA_CR_DIR_M2P     (1U << 6)
#define DMA_CR_DIR_M2M     (2U << 6)
#define DMA_CR_CIRC        (1U << 8)
#define DMA_CR_MINC        (1U << 10)
#define DMA_CR_PINC        (1U << 9)
#define DMA_CR_PSIZE_16    (1U << 11)
#define DMA_CR_MSIZE_16    (1U << 13)
#define DMA_CR_PL_LOW      (0U << 16)
#define DMA_CR_PL_MED      (1U << 16)
#define DMA_CR_PL_HIGH     (2U << 16)
#define DMA_CR_PL_VHIGH    (3U << 16)
#define DMA_CR_DBM         (1U << 18)  /* double buffer mode */
#define DMA_CR_CT          (1U << 19)  /* current target */

#define DMA_FCR_DMDIS      (1U << 2)
#define DMA_FCR_FTH_FULL   (3U << 0)

/* DMA interrupt flags (within LISR/HISR) */
#define DMA_TCIF0          (1U << 5)
#define DMA_TCIF1          (1U << 11)
#define DMA_TCIF2          (1U << 21)
#define DMA_TCIF3          (1U << 27)

/* ── SysTick ──────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} systick_regs_t;

#define SYSTICK            ((systick_regs_t *)(STK_BASE))
#define SYSTICK_CTRL_ENABLE  (1U << 0)
#define SYSTICK_CTRL_TICKINT (1U << 1)
#define SYSTICK_CTRL_CLKSRC  (1U << 2)

/* ── NVIC ─────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t ISER[8];   /* 0x000 interrupt set-enable  */
    volatile uint32_t RESERVED[24];
    volatile uint32_t ICER[8];   /* 0x080 interrupt clear-enable */
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ISPR[8];   /* 0x100 interrupt set-pending  */
    volatile uint32_t RESERVED3[24];
    volatile uint32_t ICPR[8];   /* 0x180 interrupt clear-pending */
    volatile uint32_t RESERVED4[24];
    volatile uint32_t IABR[8];   /* 0x200 interrupt active bit   */
    volatile uint32_t RESERVED5[56];
    volatile uint8_t  IPR[240];  /* 0x400 interrupt priority     */
} nvic_regs_t;

#define NVIC               ((nvic_regs_t *)(NVIC_BASE))

/* NVIC IRQ numbers for STM32H743 (relevant subset) */
#define IRQ_DMA1_STREAM0   11
#define IRQ_DMA1_STREAM1   12
#define IRQ_DMA1_STREAM2   13
#define IRQ_DMA1_STREAM3   14
#define IRQ_SPI1           35
#define IRQ_SPI2           36
#define IRQ_USART3         39
#define IRQ_SPI4           84
#define IRQ_EXTI4          11  /* shared with DMA1_STREAM0 on H7 */
#define IRQ_EXTI9_5        23
#define IRQ_EXTI15_10      40

static inline void nvic_enable(uint32_t irq) {
    NVIC->ISER[irq / 32] = (1U << (irq % 32));
}
static inline void nvic_set_priority(uint32_t irq, uint32_t prio) {
    NVIC->IPR[irq] = (uint8_t)((prio << 4) & 0xFF);
}

/* ── SCB ──────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t RESERVED[2];
    volatile uint32_t VTOR;     /* 0x08 vector table offset */
    volatile uint32_t RESERVED2;
    volatile uint32_t AIRCR;    /* 0x0C application interrupt/reset */
    volatile uint32_t SCR;      /* 0x10 system control */
    volatile uint32_t CCR;      /* 0x14 configuration & control */
    volatile uint8_t  SHPR[12]; /* 0x18 system handlers priority */
    volatile uint32_t RESERVED3[4];
    volatile uint32_t CPACR;    /* 0x88 coprocessor access control */
} scb_regs_t;

#define SCB                ((scb_regs_t *)(SCB_BASE))
#define SCB_CPACR_CP10_FULL (0xFU << 20)
#define SCB_CPACR_CP11_FULL (0xFU << 22)

/* ── FMC SDRAM controller registers ───────────────────────────── */
typedef struct {
    volatile uint32_t SDCR1;   /* 0x000 SDRAM control reg 1 */
    volatile uint32_t SDCR2;   /* 0x004 SDRAM control reg 2 */
    volatile uint32_t SDTR1;   /* 0x008 SDRAM timing reg 1  */
    volatile uint32_t SDTR2;   /* 0x00C SDRAM timing reg 2  */
    volatile uint32_t SDCMR;   /* 0x010 SDRAM command mode  */
    volatile uint32_t SDRTR;   /* 0x014 SDRAM refresh rate  */
    volatile uint32_t SDSR;    /* 0x018 SDRAM status        */
} fmc_sdram_regs_t;

#define FMC_SDRAM          ((fmc_sdram_regs_t *)(0xA0000140UL))

/* FMC SDCR1 fields */
#define FMC_SDCR_BANK_SDCLK_2  (2U << 0)   /* SDCLK = 2x HCLK */
#define FMC_SDCR_BANK_CAS_3    (3U << 4)    /* CAS latency = 3  */
#define FMC_SDCR_BANK_NB_4     (0U << 6)    /* 4 internal banks */
#define FMC_SDCR_BANK_MWID_32  (2U << 4)    /* 32-bit data      */

/* ── QSPI registers (simplified) ──────────────────────────────── */
typedef struct {
    volatile uint32_t CR;       /* 0x00 control              */
    volatile uint32_t DCR;      /* 0x04 device config         */
    volatile uint32_t SR;       /* 0x08 status                */
    volatile uint32_t FCR;      /* 0x0C flag clear            */
    volatile uint32_t DLR;      /* 0x10 data length           */
    volatile uint32_t CCR;      /* 0x14 comm config           */
    volatile uint32_t AR;       /* 0x18 address               */
    volatile uint32_t ABR;      /* 0x1C alternate bytes       */
    volatile uint32_t DR;       /* 0x20 data                  */
    volatile uint32_t PSMKR;    /* 0x24 poll status mask      */
    volatile uint32_t PSMAR;    /* 0x28 poll status match     */
    volatile uint32_t PIR;      /* 0x2C poll interval         */
    volatile uint32_t LPTR;     /* 0x30 low-power timeout     */
} qspi_regs_t;

#define QSPI               ((qspi_regs_t *)(QSPI_BASE))
#define QSPI_CR_EN         (1U << 0)
#define QSPI_CR_DMAEN      (1U << 2)
#define QSPI_CR_TCEN       (1U << 11)
#define QSPI_CR_FSEL       (1U << 1)   /* flash select */
#define QSPI_CR_APMS       (1U << 3)   /* auto poll    */
#define QSPI_SR_TCF        (1U << 1)   /* transfer complete */
#define QSPI_SR_BUSY       (1U << 5)
#define QSPI_CCR_FMODE_IND (0U << 26)
#define QSPI_CCR_FMODE_MEM (3U << 26)
#define QSPI_CCR_DMODE_4L  (3U << 12)
#define QSPI_CCR_ADMODE_4L (3U << 8)
#define QSPI_CCR_IMODE_4L  (3U << 4)

/* ── Flash controller (for firmware update) ───────────────────── */
typedef struct {
    volatile uint32_t ACR;      /* 0x00 access control        */
    volatile uint32_t KEYR[2];  /* 0x04-0x08 key registers    */
    volatile uint32_t OPTKEYR[2]; /* 0x0C-0x10 option key     */
    volatile uint32_t SR;       /* 0x14 status                */
    volatile uint32_t CR;       /* 0x18 control               */
    volatile uint32_t OPTCR;    /* 0x1C option control        */
    volatile uint32_t OPTSR;    /* 0x20 option status         */
    volatile uint32_t OPTCCR;   /* 0x24 option clear status   */
    volatile uint32_t PRAR_CUR; /* 0x28 prop address          */
    volatile uint32_t PRAR_PRG;
    volatile uint32_t SCAR_CUR;
    volatile uint32_t SCAR_PRG;
    volatile uint32_t WPSN_CUR;
    volatile uint32_t WPSN_PRG;
} flash_regs_t;

#define FLASH_BASE_ADDR   0x52002000UL
#define FLASH             ((flash_regs_t *)(FLASH_BASE_ADDR))
#define FLASH_ACR_LATENCY_2 (2U << 0)  /* 2 wait states (WS2) */
#define FLASH_SR_BSY      (1U << 0)
#define FLASH_CR_LOCK     (1U << 0)
#define FLASH_CR_PG       (1U << 1)
#define FLASH_CR_SER      (1U << 2)
#define FLASH_CR_PNB_SHIFT 3
#define FLASH_CR_STRT     (1U << 7)
#define FLASH_CR_BKER     (1U << 11)

/* ── EXTI (external interrupts) ───────────────────────────────── */
typedef struct {
    volatile uint32_t RTSR1;    /* 0x00 rising trigger sel   */
    volatile uint32_t FTSR1;    /* 0x04 falling trigger sel  */
    volatile uint32_t SWIER1;   /* 0x08 software interrupt   */
    volatile uint32_t RESERVED1;
    volatile uint32_t PR1;      /* 0x0C pending register     */
    volatile uint32_t RESERVED2[3];
    volatile uint32_t RTSR2;    /* 0x20 */
    volatile uint32_t FTSR2;    /* 0x24 */
    volatile uint32_t SWIER2;   /* 0x28 */
    volatile uint32_t RESERVED3;
    volatile uint32_t PR2;      /* 0x2C */
} exti_regs_t;

#define EXTI_BASE_ADDR    0x58002800UL
#define EXTI              ((exti_regs_t *)(EXTI_BASE_ADDR))

/* ── SYSCFG ───────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t RESERVED[1];
    volatile uint32_t PMR[16];  /* 0x04-0x40 pad mode (exti mux) */
    volatile uint32_t RESERVED2[16];
    volatile uint32_t UR;       /* 0x80 user register */
} syscfg_regs_t;

#define SYSCFG             ((syscfg_regs_t *)(SYSCFG_BASE))

#endif /* EDDY_PHANTOM_REGISTERS_H */