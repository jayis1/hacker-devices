/*
 * hart-bleeder — registers.h
 * STM32G474 register definitions for the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_REGISTERS_H
#define HART_BLEEDER_REGISTERS_H

#include <stdint.h>

/* ── GPIO register layout ─────────────────────────────────────── */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 mode register          */
    volatile uint32_t OTYPER;   /* 0x04 output type            */
    volatile uint32_t OSPEEDR;  /* 0x08 output speed           */
    volatile uint32_t PUPDR;    /* 0x0C pull-up/pull-down      */
    volatile uint32_t IDR;      /* 0x10 input data             */
    volatile uint32_t ODR;      /* 0x14 output data            */
    volatile uint32_t BSRR;     /* 0x18 bit set/reset          */
    volatile uint32_t LCKR;     /* 0x1C lock                   */
    volatile uint32_t AFR[2];   /* 0x20-0x24 alternate function */
    volatile uint32_t BRR;      /* 0x28 bit reset              */
} gpio_regs_t;

#define GPIO_MODE_INPUT   0
#define GPIO_MODE_OUTPUT   1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG   3
#define GPIO_SPEED_LOW     0
#define GPIO_SPEED_HIGH     3
#define GPIO_PUPD_NONE     0
#define GPIO_PUPD_PU       1
#define GPIO_PUPD_PD       2
#define GPIO_AF7_USART12   7
#define GPIO_AF5_SPI1      5
#define GPIO_AF4_I2C       4
#define GPIO_AF6_SPI3      6
#define GPIO_AF10_QSPI     10

#define GPIO(port)  ((gpio_regs_t *)(GPIO##port##_BASE))

static inline void gpio_set_mode(gpio_regs_t *g, uint8_t pin, uint8_t mode) {
    uint32_t v = g->MODER;
    v &= ~(0x3UL << (pin * 2));
    v |= (uint32_t)mode << (pin * 2);
    g->MODER = v;
}
static inline void gpio_set_af(gpio_regs_t *g, uint8_t pin, uint8_t af) {
    uint32_t idx = pin >> 3;
    uint32_t shift = (pin & 7) * 4;
    uint32_t v = g->AFR[idx];
    v &= ~(0xFUL << shift);
    v |= (uint32_t)af << shift;
    g->AFR[idx] = v;
}
static inline void gpio_set_pupd(gpio_regs_t *g, uint8_t pin, uint8_t pupd) {
    uint32_t v = g->PUPDR;
    v &= ~(0x3UL << (pin * 2));
    v |= (uint32_t)pupd << (pin * 2);
    g->PUPDR = v;
}
static inline void gpio_set_speed(gpio_regs_t *g, uint8_t pin, uint8_t sp) {
    uint32_t v = g->OSPEEDR;
    v &= ~(0x3UL << (pin * 2));
    v |= (uint32_t)sp << (pin * 2);
    g->OSPEEDR = v;
}
static inline void gpio_set_otyper(gpio_regs_t *g, uint8_t pin, uint8_t od) {
    if (od) g->OTYPER |= (1UL << pin);
    else    g->OTYPER &= ~(1UL << pin);
}
static inline void gpio_write(gpio_regs_t *g, uint8_t pin, uint8_t val) {
    if (val) g->BSRR = 1UL << pin;
    else     g->BSRR = 1UL << (pin + 16);
}
static inline uint8_t gpio_read(gpio_regs_t *g, uint8_t pin) {
    return (g->IDR >> pin) & 1U;
}

/* ── USART register layout ────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 control 1              */
    volatile uint32_t CR2;   /* 0x04 control 2              */
    volatile uint32_t CR3;   /* 0x08 control 3              */
    volatile uint32_t BRR;   /* 0x0C baudrate                 */
    volatile uint32_t GTPR;  /* 0x10 guard time / prescaler   */
    volatile uint32_t RTOR;  /* 0x14 receiver timeout         */
    volatile uint32_t RQR;   /* 0x18 request                   */
    volatile uint32_t ISR;   /* 0x1C interrupt/status          */
    volatile uint32_t ICR;   /* 0x20 interrupt flag clear      */
    volatile uint32_t RDR;   /* 0x24 receive data               */
    volatile uint32_t TDR;   /* 0x28 transmit data              */
} usart_regs_t;

#define USART_CR1_UE        (1U << 0)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_IDLEIE    (1U << 4)
#define USART_CR1_RXNEIE    (1U << 5)
#define USART_CR1_TCIE      (1U << 6)
#define USART_CR1_TXEIE     (1U << 7)
#define USART_CR1_M0        (1U << 12)
#define USART_CR1_OVER8     (1U << 15)
#define USART_CR2_STOP_1    (1U << 13)  /* 1 stop bit (0 stop bits field) */
#define USART_CR3_CTSE      (1U << 8)
#define USART_CR3_RTSE      (1U << 9)
#define USART_CR3_HDSEL     (1U << 3)   /* half-duplex selection */
#define USART_ISR_PE        (1U << 0)
#define USART_ISR_FE        (1U << 1)
#define USART_ISR_NE        (1U << 2)
#define USART_ISR_ORE       (1U << 3)
#define USART_ISR_IDLE      (1U << 4)
#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TC        (1U << 6)
#define USART_ISR_TXE       (1U << 7)
#define USART_ISR_BUSY      (1U << 16)
#define USART_ISR_RTOF      (1U << 11)  /* receiver timeout flag */
#define USART_CR1_RTOIE     (1U << 26)  /* receiver timeout IE   */

#define UART(regs) ((usart_regs_t *)(regs##_BASE))

/* ── RCC ──────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR;       /* 0x00 clock control              */
    volatile uint32_t ICSCR;    /* 0x04 internal clock source calib */
    volatile uint32_t CFGR;     /* 0x08 clock config                */
    volatile uint32_t CRR;      /* 0x0C (reserved on G4)            */
    volatile uint32_t PLLCFGR;  /* 0x0C (offsets shifted on G4)     */
    /* The G4 RCC layout differs — use explicit offsets below. */
} rcc_dummy_t;

#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_ICSCR       (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_CIER        (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_CIFR        (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_CICR        (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x48))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB3ENR     (*(volatile uint32_t *)(RCC_BASE + 0x50))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR2    (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_CCIPR       (*(volatile uint32_t *)(RCC_BASE + 0x88))

#define RCC_CR_HSION    (1U << 8)
#define RCC_CR_HSIRDY   (1U << 10)
#define RCC_CR_HSEON    (1U << 16)
#define RCC_CR_HSERDY   (1U << 17)
#define RCC_CR_HSECSSON (1U << 19)
#define RCC_CR_PLLON    (1U << 24)
#define RCC_CR_PLLRDY   (1U << 25)

#define RCC_CFGR_SW_HSI  0
#define RCC_CFGR_SW_HSE  (1U << 0)
#define RCC_CFGR_SW_PLL  (1U << 1)
#define RCC_CFGR_SWS_MSK (3U << 2)

#define RCC_AHB1ENR_DMA1   (1U << 0)
#define RCC_AHB1ENR_DMA2   (1U << 1)
#define RCC_AHB1ENR_DMAMUX1 (1U << 2)
#define RCC_AHB2ENR_GPIOA  (1U << 0)
#define RCC_AHB2ENR_GPIOB  (1U << 1)
#define RCC_AHB2ENR_GPIOC  (1U << 2)
#define RCC_AHB2ENR_GPIOF  (1U << 5)
#define RCC_AHB2ENR_GPIOG  (1U << 6)
#define RCC_AHB2ENR_ADC12  (1U << 13)
#define RCC_AHB2ENR_DAC1EN (1U << 16)
#define RCC_APB1ENR1_USART2 (1U << 17)
#define RCC_APB1ENR1_USART3 (1U << 18)
#define RCC_APB1ENR1_UART4  (1U << 19)
#define RCC_APB1ENR1_UART5  (1U << 20)
#define RCC_APB1ENR1_TIM2   (1U << 0)
#define RCC_APB1ENR1_TIM3   (1U << 1)
#define RCC_APB1ENR1_TIM6   (1U << 4)
#define RCC_APB1ENR1_TIM7   (1U << 5)
#define RCC_APB1ENR1_I2C2   (1U << 22)
#define RCC_APB1ENR1_SPI2   (1U << 14)
#define RCC_APB2ENR_USART1  (1U << 14)
#define RCC_APB2ENR_SPI1    (1U << 12)
#define RCC_APB2ENR_TIM8    (1U << 1)
#define RCC_APB2ENR_SYSCFG   (1U << 0)
#define RCC_AHB3ENR_QSPI    (1U << 8)

/* ── FLASH controller (for latency / prefetch) ────────────────── */
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE_REG + 0x00))
#define FLASH_ACR_LATENCY_MASK  (0xFU)
#define FLASH_ACR_PRFTEN        (1U << 3)
#define FLASH_ACR_ICEN          (1U << 8)
#define FLASH_ACR_DCEN          (1U << 9)

/* ── ADC (simplified — 12-bit, single-ended) ──────────────────── */
typedef struct {
    volatile uint32_t ISR;    /* 0x00 */
    volatile uint32_t IER;    /* 0x04 */
    volatile uint32_t CR;     /* 0x08 */
    volatile uint32_t CFGR;   /* 0x0C */
    volatile uint32_t CFGR2;  /* 0x10 */
    volatile uint32_t SMPR[2];/* 0x14,0x18 */
    volatile uint32_t TR[2];  /* 0x20,0x24 */
    volatile uint32_t OFR[4]; /* 0x30.. */
    volatile uint32_t RES;    /* reserved */
    /* ... (full layout omitted for brevity) */
} adc_regs_t;

#define ADC_ISR_ADRDY    (1U << 0)
#define ADC_ISR_EOC      (1U << 2)
#define ADC_ISR_EOS      (1U << 3)
#define ADC_ISR_OVR      (1U << 4)
#define ADC_CR_ADEN      (1U << 0)
#define ADC_CR_ADSTART   (1U << 2)
#define ADC_CR_ADDIS     (1U << 1)
#define ADC_CR_ADCAL     (1U << 16)
#define ADC_CFGR_CONT    (1U << 1)
#define ADC_CFGR_DMAEN   (1U << 8)
#define ADC_CFGR_DMACFG  (1U << 9)
#define ADC(x) ((adc_regs_t *)(ADC##x##_BASE))

/* ── DAC ──────────────────────────────────────────────────────── */
#define DAC_CR           (*(volatile uint32_t *)(DAC1_BASE + 0x00))
#define DAC_DHR12R1      (*(volatile uint32_t *)(DAC1_BASE + 0x08))
#define DAC_DOR1         (*(volatile uint32_t *)(DAC1_BASE + 0x2C))
#define DAC_CR_EN1       (1U << 0)
#define DAC_CR_TSEL1_MSK (7U << 3)

/* ── TIM (simplified — used for HART timeouts / ADC triggers) ── */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t SMCR;  /* 0x08 */
    volatile uint32_t DIER;  /* 0x0C */
    volatile uint32_t SR;    /* 0x10 */
    volatile uint32_t EGR;   /* 0x14 */
    volatile uint32_t CCMR1; /* 0x18 */
    volatile uint32_t CCMR2; /* 0x1C */
    volatile uint32_t CCER;  /* 0x20 */
    volatile uint32_t CNT;   /* 0x24 */
    volatile uint32_t PSC;   /* 0x28 */
    volatile uint32_t ARR;   /* 0x2C */
    volatile uint32_t RCR;   /* 0x30 */
    volatile uint32_t CCR1;  /* 0x34 */
    volatile uint32_t CCR2;  /* 0x38 */
    volatile uint32_t CCR3;  /* 0x3C */
    volatile uint32_t CCR4;  /* 0x40 */
} tim_regs_t;

#define TIM_CR1_CEN    (1U << 0)
#define TIM_DIER_UIE   (1U << 0)
#define TIM_SR_UIF     (1U << 0)
#define TIM(x) ((tim_regs_t *)(TIM##x##_BASE))

/* ── DMA channel (simplified) ─────────────────────────────────── */
typedef struct {
    volatile uint32_t CCR;   /* 0x00 */
    volatile uint32_t CNDTR; /* 0x04 */
    volatile uint32_t CPAR;  /* 0x08 */
    volatile uint32_t CMAR;  /* 0x0C */
    volatile uint32_t Reserved;
} dma_channel_t;

#define DMA_CCR_EN      (1U << 0)
#define DMA_CCR_TCIE    (1U << 1)
#define DMA_CCR_HTIE    (1U << 2)
#define DMA_CCR_TEIE    (1U << 3)
#define DMA_CCR_DIR_P2M (0U << 4)
#define DMA_CCR_DIR_M2P (1U << 4)
#define DMA_CCR_MINC    (1U << 7)
#define DMA_CCR_PINC    (1U << 6)
#define DMA_CCR_PSIZE_8 0
#define DMA_CCR_PSIZE_16 (1U << 8)
#define DMA_CCR_MSIZE_8 0
#define DMA_CCR_MSIZE_16 (1U << 10)
#define DMA_CCR_CIRC    (1U << 8)
#define DMA_CCR_PL_HIGH (2U << 12)

#define DMA1_CHANNEL_BASE(n) (DMA1_BASE + 0x08 + (n - 1) * 0x14)

#endif /* HART_BLEEDER_REGISTERS_H */