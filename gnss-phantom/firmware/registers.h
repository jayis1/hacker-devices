/*
 * registers.h — MMIO register map for GNSS-Phantom
 *
 * Target MCU: STM32H743VI (Cortex-M7, 480 MHz, BGA-100)
 * Author:     jayis1
 * License:    Proprietary — Authorized Security Research Use Only
 *
 * This header defines base addresses and offset maps for every peripheral
 * touched by the firmware: RCC, GPIO, USART, SPI, TIM, ADC, DAC, DMA, EXTI,
 * NVIC, SysTick, and the HackRF-style MAX5864 ADC / MAX2837 2.4 GHz transceiver
 * (mapped onto SPI2).  The GNSS spoofing signal chain is:
 *
 *   GPS L1 C/A (1575.42 MHz) synthesized by Si4463 (SiLabs)
 *   Galileo E1  (1575.42 MHz) synthesized by Si4463
 *   GLONASS L1  (1602 MHz)    synthesized by Si4463
 *   BeiDou B1   (1561.098 MHz) synthesized by Si4463
 *
 * The Si4463 transceivers generate the 2.4 GHz IF, which is upconverted by
 * the analog front-end to L1.  All RF blocks share a common 26 MHz TCXO.
 */

#ifndef GNSS_PHANTOM_REGISTERS_H
#define GNSS_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ----- Cortex-M7 system peripherals ----- */
#define PERIPH_BASE        0x40000000U
#define PERIPH_BASE_AHB1   0x40018000U
#define PERIPH_BASE_AHB2   0x48010000U
#define PERIPH_BASE_APB1   0x40004000U
#define PERIPH_BASE_APB2   0x48000000U

/* NVIC */
#define NVIC_BASE          0xE000E100U
#define NVIC_ISER0         (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ISER1         (*(volatile uint32_t *)(NVIC_BASE + 0x004))
#define NVIC_ICER0         (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ICER1         (*(volatile uint32_t *)(NVIC_BASE + 0x084))
#define NVIC_ISPR0         (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ISPR1         (*(volatile uint32_t *)(NVIC_BASE + 0x104))
#define NVIC_ICPR0         (*(volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_ICPR1         (*(volatile uint32_t *)(NVIC_BASE + 0x184))
#define NVIC_IPR(n)        (*(volatile uint8_t *)(NVIC_BASE + 0x300 + (n)))

/* SCB */
#define SCB_BASE           0xE000ED00U
#define SCB_CPUID         (*(volatile uint32_t *)(SCB_BASE + 0x000))
#define SCB_ICSR          (*(volatile uint32_t *)(SCB_BASE + 0x004))
#define SCB_VTOR          (*(volatile uint32_t *)(SCB_BASE + 0x008))
#define SCB_SCR           (*(volatile uint32_t *)(SCB_BASE + 0x010))
#define SCB_CCR           (*(volatile uint32_t *)(SCB_BASE + 0x014))
#define SCB_SHPR(n)       (*(volatile uint8_t *)(SCB_BASE + 0x018 + (n)))
#define SCB_CPACR         (*(volatile uint32_t *)(SCB_BASE + 0x088))

/* SysTick */
#define SYSTICK_BASE       0xE000E010U
#define SYST_CSR          (*(volatile uint32_t *)(SYSTICK_BASE + 0x000))
#define SYST_RVR          (*(volatile uint32_t *)(SYSTICK_BASE + 0x004))
#define SYST_CVR          (*(volatile uint32_t *)(SYSTICK_BASE + 0x008))
#define SYST_CALIB        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00C))

/* ----- Reset and Clock Controller (RCC) ----- */
#define RCC_BASE           0x58024400U
#define RCC_CR            (*(volatile uint32_t *)(RCC_BASE + 0x000))
#define RCC_PLLCFGR       (*(volatile uint32_t *)(RCC_BASE + 0x008))
#define RCC_CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x00C))
#define RCC_AHB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0D0))
#define RCC_AHB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0D4))
#define RCC_AHB3ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0D8))
#define RCC_APB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0E0))
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0E4))
#define RCC_APB4ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0E8))
#define RCC_D1CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x010))
#define RCC_D2CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x014))
#define RCC_D3CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x018))
#define RCC_PLL1DIVR      (*(volatile uint32_t *)(RCC_BASE + 0x020))
#define RCC_PLL1FRACR     (*(volatile uint32_t *)(RCC_BASE + 0x024))
#define RCC_CRRCR         (*(volatile uint32_t *)(RCC_BASE + 0x060))

/* RCC bitfield helpers */
#define RCC_CR_HSION      (1U << 0)
#define RCC_CR_HSIRDY     (1U << 1)
#define RCC_CR_HSEON      (1U << 16)
#define RCC_CR_HSERDY     (1U << 17)
#define RCC_CR_HSEBYP     (1U << 18)
#define RCC_CR_CSION      (1U << 20)
#define RCC_CR_CSIRDY     (1U << 21)
#define RCC_CR_PLL1ON     (1U << 24)
#define RCC_CR_PLL1RDY    (1U << 25)

/* ----- Power Controller (PWR) ----- */
#define PWR_BASE           0x58024800U
#define PWR_CR1           (*(volatile uint32_t *)(PWR_BASE + 0x000))
#define PWR_CR3           (*(volatile uint32_t *)(PWR_BASE + 0x008))
#define PWR_SR1           (*(volatile uint32_t *)(PWR_BASE + 0x010))
#define PWR_SR2           (*(volatile uint32_t *)(PWR_BASE + 0x014))

/* ----- Flash Controller ----- */
#define FLASH_BASE         0x52002000U
#define FLASH_ACR         (*(volatile uint32_t *)(FLASH_BASE + 0x000))
#define FLASH_KEYR        (*(volatile uint32_t *)(FLASH_BASE + 0x004))
#define FLASH_SR1         (*(volatile uint32_t *)(FLASH_BASE + 0x008))
#define FLASH_CR1         (*(volatile uint32_t *)(FLASH_BASE + 0x00C))
#define FLASH_OPTCR       (*(volatile uint32_t *)(FLASH_BASE + 0x018))

/* ----- GPIO ----- */
typedef struct {
    volatile uint32_t MODER;   /* 0x00 */
    volatile uint32_t OTYPER;  /* 0x04 */
    volatile uint32_t OSPEEDR; /* 0x08 */
    volatile uint32_t PUPDR;   /* 0x0C */
    volatile uint32_t IDR;     /* 0x10 */
    volatile uint32_t ODR;     /* 0x14 */
    volatile uint32_t BSRR;    /* 0x18 */
    volatile uint32_t LCKR;    /* 0x1C */
    volatile uint32_t AFRL;    /* 0x20 */
    volatile uint32_t AFRH;    /* 0x24 */
    volatile uint32_t BRR;     /* 0x28 */
} GPIO_TypeDef;

#define GPIOA_BASE         0x58020000U
#define GPIOB_BASE         0x58020400U
#define GPIOC_BASE         0x58020800U
#define GPIOD_BASE         0x58020C00U
#define GPIOE_BASE         0x58021000U
#define GPIOF_BASE         0x58021400U
#define GPIOG_BASE         0x58021800U
#define GPIOH_BASE         0x58021C00U
#define GPIOI_BASE         0x58022000U
#define GPIOJ_BASE         0x58022400U
#define GPIOK_BASE         0x58022800U

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG ((GPIO_TypeDef *)GPIOG_BASE)
#define GPIOH ((GPIO_TypeDef *)GPIOH_BASE)
#define GPIOI ((GPIO_TypeDef *)GPIOI_BASE)

/* GPIO mode constants */
#define GPIO_MODE_INPUT    0
#define GPIO_MODE_OUTPUT   1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG   3
#define GPIO_OTYPE_PP      0
#define GPIO_OTYPE_OD      1
#define GPIO_OSPEED_LOW    0
#define GPIO_OSPEED_MED    1
#define GPIO_OSPEED_HIGH   2
#define GPIO_OSPEED_VHIGH  3
#define GPIO_PUPD_NONE     0
#define GPIO_PUPD_PU       1
#define GPIO_PUPD_PD       2

static inline void gpio_set_mode(GPIO_TypeDef *port, int pin, int mode) {
    uint32_t m = port->MODER;
    m &= ~(3U << (pin * 2));
    m |= (uint32_t)(mode & 3U) << (pin * 2);
    port->MODER = m;
}

static inline void gpio_set_af(GPIO_TypeDef *port, int pin, int af) {
    if (pin < 8) {
        uint32_t r = port->AFRL;
        r &= ~(0xFU << (pin * 4));
        r |= (uint32_t)(af & 0xFU) << (pin * 4);
        port->AFRL = r;
    } else {
        int p = pin - 8;
        uint32_t r = port->AFRH;
        r &= ~(0xFU << (p * 4));
        r |= (uint32_t)(af & 0xFU) << (p * 4);
        port->AFRH = r;
    }
}

static inline void gpio_set_speed(GPIO_TypeDef *port, int pin, int speed) {
    uint32_t m = port->OSPEEDR;
    m &= ~(3U << (pin * 2));
    m |= (uint32_t)(speed & 3U) << (pin * 2);
    port->OSPEEDR = m;
}

static inline void gpio_set_pupd(GPIO_TypeDef *port, int pin, int pupd) {
    uint32_t m = port->PUPDR;
    m &= ~(3U << (pin * 2));
    m |= (uint32_t)(pupd & 3U) << (pin * 2);
    port->PUPDR = m;
}

static inline void gpio_set_otype(GPIO_TypeDef *port, int pin, int otype) {
    if (otype == GPIO_OTYPE_OD)
        port->OTYPER |= (1U << pin);
    else
        port->OTYPER &= ~(1U << pin);
}

static inline void gpio_write(GPIO_TypeDef *port, int pin, int val) {
    if (val)
        port->BSRR = 1U << pin;
    else
        port->BSRR = 1U << (pin + 16);
}

static inline int gpio_read(GPIO_TypeDef *port, int pin) {
    return (port->IDR >> pin) & 1;
}

static inline void gpio_toggle(GPIO_TypeDef *port, int pin) {
    port->ODR ^= (1U << pin);
}

/* ----- USART (debug console on USART3) ----- */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t CR3;   /* 0x08 */
    volatile uint32_t BRR;   /* 0x0C */
    volatile uint32_t GTPR;  /* 0x10 */
    volatile uint32_t RTOR;  /* 0x14 */
    volatile uint32_t RQR;   /* 0x18 */
    volatile uint32_t ISR;   /* 0x1C */
    volatile uint32_t ICR;   /* 0x20 */
    volatile uint32_t RDR;   /* 0x24 */
    volatile uint32_t TDR;   /* 0x28 */
} USART_TypeDef;

#define USART1_BASE        0x58005000U
#define USART2_BASE        0x58006000U
#define USART3_BASE        0x58007000U
#define UART4_BASE         0x58008000U
#define UART4 ((USART_TypeDef *)UART4_BASE)
#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART2 ((USART_TypeDef *)USART2_BASE)
#define USART3 ((USART_TypeDef *)USART3_BASE)

#define USART_CR1_UE       (1U << 0)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_RE       (1U << 2)
#define USART_CR1_RXNEIE   (1U << 5)
#define USART_CR1_TXEIE    (1U << 7)
#define USART_ISR_TXE      (1U << 7)
#define USART_ISR_TC       (1U << 6)
#define USART_ISR_RXNE     (1U << 5)

/* ----- SPI (Si4463 transceivers + MAX2837 IF + MAX5864 ADC) ----- */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t CFG1;  /* 0x08 */
    volatile uint32_t CFG2;  /* 0x0C */
    volatile uint32_t IER;   /* 0x10 */
    volatile uint32_t SR;    /* 0x14 */
    volatile uint32_t IFCR;  /* 0x18 */
    volatile uint32_t TXDR;  /* 0x1C */
    volatile uint32_t RXDR;  /* 0x20 */
    volatile uint32_t CRCPR; /* 0x24 */
    volatile uint32_t RXCRCR;/* 0x28 */
    volatile uint32_t UDRCFG; /* 0x2C */
} SPI_TypeDef;

#define SPI1_BASE          0x58013000U
#define SPI2_BASE          0x58014000U
#define SPI3_BASE          0x58015000U
#define SPI4_BASE          0x58016000U
#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI2 ((SPI_TypeDef *)SPI2_BASE)
#define SPI3 ((SPI_TypeDef *)SPI3_BASE)
#define SPI4 ((SPI_TypeDef *)SPI4_BASE)

#define SPI_CR1_SPE        (1U << 0)
#define SPI_CR1_CPHA       (1U << 0)
#define SPI_CR1_CPOL       (1U << 1)
#define SPI_CR1_MSTR       (1U << 2)
#define SPI_CR1_BR_SHIFT   (3)
#define SPI_CR1_BR_MASK    (7U << SPI_CR1_BR_SHIFT)
#define SPI_CR1_LSBFIRST   (1U << 6)
#define SPI_CR1_SSI        (1U << 8)
#define SPI_CR1_SSM        (1U << 9)
#define SPI_CR2_TSIZE      (1U << 0)
#define SPI_CR2_RXDMAEN    (1U << 0)
#define SPI_CR2_TXDMAEN    (1U << 1)
#define SPI_SR_RXP         (1U << 0)
#define SPI_SR_TXP         (1U << 1)
#define SPI_SR_EOT         (1U << 3)
#define SPI_SR_OVR         (1U << 6)
#define SPI_SR_MODF        (1U << 8)
#define SPI_SR_TIFRE       (1U << 9)

/* ----- DMA (stream-based, for SPI TX/RX and DAC circular) ----- */
typedef struct {
    volatile uint32_t LISR;  /* 0x00 low interrupt status */
    volatile uint32_t HISR;  /* 0x04 high interrupt status */
    volatile uint32_t LIFCR; /* 0x08 low flag clear */
    volatile uint32_t HIFCR; /* 0x0C high flag clear */
} DMA_Stream_common;

typedef struct {
    volatile uint32_t CR;   /* stream config */
    volatile uint32_t NDTR; /* number of data to transfer */
    volatile uint32_t PAR;  /* peripheral address */
    volatile uint32_t M0AR; /* memory 0 address */
    volatile uint32_t M1AR; /* memory 1 address */
    volatile uint32_t FCR;  /* FIFO control */
} DMA_Stream_TypeDef;

#define DMA1_BASE          0x58000000U
#define DMA1_Stream0 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x010))
#define DMA1_Stream1 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x028))
#define DMA1_Stream2 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x040))
#define DMA1_Stream3 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x058))
#define DMA1_Stream4 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x070))
#define DMA1_Stream5 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x088))
#define DMA1_Stream6 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x0A0))
#define DMA1_Stream7 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x0B8))

#define DMA2_Stream0 ((DMA_Stream_TypeDef *)(0x58001000U + 0x010))
#define DMA2_Stream1 ((DMA_Stream_TypeDef *)(0x58001000U + 0x028))
#define DMA2_Stream2 ((DMA_Stream_TypeDef *)(0x58001000U + 0x040))
#define DMA2_Stream3 ((DMA_Stream_TypeDef *)(0x58001000U + 0x058))
#define DMA2_Stream4 ((DMA_Stream_TypeDef *)(0x58001000U + 0x070))
#define DMA2_Stream5 ((DMA_Stream_TypeDef *)(0x58001000U + 0x088))
#define DMA2_Stream6 ((DMA_Stream_TypeDef *)(0x58001000U + 0x0A0))
#define DMA2_Stream7 ((DMA_Stream_TypeDef *)(0x58001000U + 0x0B8))

/* DMA stream CR bits */
#define DMA_CR_EN           (1U << 0)
#define DMA_CR_DMEIE        (1U << 1)
#define DMA_CR_TEIE         (1U << 2)
#define DMA_CR_HTIE         (1U << 3)
#define DMA_CR_TCIE         (1U << 4)
#define DMA_CR_PFCTRL       (1U << 5)
#define DMA_CR_DIR_SHIFT    (6)
#define DMA_CR_CIRC         (1U << 8)
#define DMA_CR_PINC         (1U << 9)
#define DMA_CR_MINC         (1U << 10)
#define DMA_CR_PSIZE_SHIFT  (11)
#define DMA_CR_MSIZE_SHIFT  (13)
#define DMA_CR_PL_SHIFT     (16)
#define DMA_CR_PBURST_SHIFT (21)
#define DMA_CR_MBURST_SHIFT (23)
#define DMA_CR_CHSEL_SHIFT  (25)
#define DMA_CR_CT           (1U << 18)
#define DMA_CR_DBM          (1U << 19)

/* ----- Timer (TIM) for PPS and DAC trigger ----- */
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
    volatile uint32_t CCR1;  /* 0x34 */
    volatile uint32_t CCR2;  /* 0x38 */
    volatile uint32_t CCR3;  /* 0x3C */
    volatile uint32_t CCR4;  /* 0x40 */
    volatile uint32_t DCR;   /* 0x48 */
    volatile uint32_t DOR;   /* 0x4C */
} TIM_TypeDef;

#define TIM1_BASE          0x58010000U
#define TIM2_BASE          0x58000000U
#define TIM3_BASE          0x58000400U
#define TIM4_BASE          0x58000800U
#define TIM5_BASE          0x58000C00U
#define TIM6_BASE          0x58001000U
#define TIM7_BASE          0x58001400U
#define TIM8_BASE          0x58010400U
#define TIM1 ((TIM_TypeDef *)TIM1_BASE)
#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM3 ((TIM_TypeDef *)TIM3_BASE)
#define TIM6 ((TIM_TypeDef *)TIM6_BASE)
#define TIM7 ((TIM_TypeDef *)TIM7_BASE)
#define TIM8 ((TIM_TypeDef *)TIM8_BASE)

#define TIM_CR1_CEN        (1U << 0)
#define TIM_CR1_ARPE       (1U << 7)
#define TIM_DIER_UIE       (1U << 0)
#define TIM_DIER_CC1IE     (1U << 1)
#define TIM_DIER_UDE       (1U << 8)
#define TIM_DIER_CC1DE     (1U << 9)
#define TIM_SR_UIF         (1U << 0)
#define TIM_SR_CC1IF       (1U << 1)
#define TIM_CCMR_OC1M_SHIFT (4)
#define TIM_CCMR_OC1PE     (1U << 3)
#define TIM_CCMR_OC1M_PWM1 (6U << 4)
#define TIM_CCMR_OC1M_PWM2 (7U << 4)

/* ----- ADC (battery monitoring) ----- */
typedef struct {
    volatile uint32_t ISR;   /* 0x00 */
    volatile uint32_t IER;   /* 0x04 */
    volatile uint32_t CR;    /* 0x08 */
    volatile uint32_t CFGR;  /* 0x0C */
    volatile uint32_t SMPR1; /* 0x10 */
    volatile uint32_t SMPR2; /* 0x14 */
    volatile uint32_t OFR1;  /* 0x18 */
    volatile uint32_t OFR2;  /* 0x1C */
    volatile uint32_t OFR3;  /* 0x20 */
    volatile uint32_t OFR4;  /* 0x24 */
    uint32_t RESERVED[4];   /* 0x28-0x34 */
    volatile uint32_t TR1;   /* 0x38 */
    uint32_t RESERVED2;     /* 0x3C */
    volatile uint32_t TR2;   /* 0x40 */
    uint32_t RESERVED3;     /* 0x44 */
    volatile uint32_t TR3;   /* 0x48 */
    uint32_t RESERVED4[4];  /* 0x4C-0x58 */
    volatile uint32_t JDR1;  /* 0x5C */
    volatile uint32_t JDR2;  /* 0x60 */
    volatile uint32_t JDR3;  /* 0x64 */
    volatile uint32_t JDR4;  /* 0x68 */
    volatile uint32_t DR;    /* 0x6C */
} ADC_TypeDef;

#define ADC1_BASE          0x58024000U
#define ADC1 ((ADC_TypeDef *)ADC1_BASE)

#define ADC_ISR_ADRDY      (1U << 0)
#define ADC_ISR_EOC        (1U << 2)
#define ADC_CR_ADEN        (1U << 0)
#define ADC_CR_ADSTART     (1U << 2)
#define ADC_CR_ADSTOP      (1U << 4)

/* ----- DAC (IF baseband I/Q waveform generation) ----- */
typedef struct {
    volatile uint32_t CR;   /* 0x00 */
    volatile uint32_t SWTRIGR;/* 0x04 */
    volatile uint32_t DHR12R1;/* 0x08 */
    volatile uint32_t DHR12L1;/* 0x0C */
    volatile uint32_t DHR8R1; /* 0x10 */
    volatile uint32_t DHR12R2;/* 0x14 */
    volatile uint32_t DHR12L2;/* 0x18 */
    volatile uint32_t DHR8R2; /* 0x1C */
    volatile uint32_t DOR1;  /* 0x20 */
    volatile uint32_t DOR2;  /* 0x24 */
    volatile uint32_t SR;    /* 0x28 */
} DAC_TypeDef;

#define DAC1_BASE          0x58008000U
#define DAC1 ((DAC_TypeDef *)DAC1_BASE)

#define DAC_CR_EN1         (1U << 0)
#define DAC_CR_DMAEN1      (1U << 12)
#define DAC_CR_TEN1        (1U << 1)
#define DAC_CR_TSEL1_SHIFT (3)
#define DAC_CR_WAVE1_SHIFT (6)

/* ----- EXTI ----- */
typedef struct {
    volatile uint32_t IMR1;  /* 0x00 */
    volatile uint32_t EMR1;  /* 0x04 */
    volatile uint32_t RTSR1; /* 0x08 */
    volatile uint32_t FTSR1; /* 0x0C */
    volatile uint32_t SWIER1;/* 0x10 */
    volatile uint32_t PR1;   /* 0x14 */
    volatile uint32_t IMR2;  /* 0x20 */
    volatile uint32_t EMR2;  /* 0x24 */
    volatile uint32_t RTSR2; /* 0x28 */
    volatile uint32_t FTSR2; /* 0x2C */
    volatile uint32_t SWIER2;/* 0x30 */
    volatile uint32_t PR2;   /* 0x34 */
} EXTI_TypeDef;

#define EXTI_BASE          0x5800CC00U
#define EXTI ((EXTI_TypeDef *)EXTI_BASE)

/* ----- I2C (MAX17048 fuel gauge + IMU) ----- */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t OAR1;  /* 0x08 */
    volatile uint32_t OAR2;  /* 0x0C */
    volatile uint32_t TIMINGR;/* 0x10 */
    volatile uint32_t TIMEOUTR;/*0x14 */
    volatile uint32_t ISR;   /* 0x18 */
    volatile uint32_t ICR;   /* 0x1C */
    volatile uint32_t PECR;  /* 0x20 */
    volatile uint32_t RXDR;  /* 0x24 */
    volatile uint32_t TXDR;  /* 0x28 */
} I2C_TypeDef;

#define I2C1_BASE          0x58010C00U
#define I2C2_BASE          0x58011000U
#define I2C1 ((I2C_TypeDef *)I2C1_BASE)
#define I2C2 ((I2C_TypeDef *)I2C2_BASE)

#define I2C_CR1_PE         (1U << 0)
#define I2C_ISR_TXE        (1U << 0)
#define I2C_ISR_RXNE       (1U << 2)
#define I2C_ISR_NACKF      (1U << 4)
#define I2C_ISR_STOPF      (1U << 5)
#define I2C_ISR_TC         (1U << 6)
#define I2C_ISR_BUSY       (1U << 15)

/* ----- RNG (for session nonces) ----- */
#define RNG_BASE           0x58001000U
#define RNG_CR            (*(volatile uint32_t *)(RNG_BASE + 0x000))
#define RNG_SR            (*(volatile uint32_t *)(RNG_BASE + 0x004))
#define RNG_DR            (*(volatile uint32_t *)(RNG_BASE + 0x008))
#define RNG_CR_RNGEN       (1U << 2)

/* ----- QSPI (external flash for scenario storage) ----- */
typedef struct {
    volatile uint32_t CR;    /* 0x00 */
    volatile uint32_t DCR;   /* 0x04 */
    volatile uint32_t SR;    /* 0x08 */
    volatile uint32_t FCR;   /* 0x0C */
    volatile uint32_t DLR;   /* 0x10 */
    volatile uint32_t CCR;   /* 0x14 */
    volatile uint32_t AR;    /* 0x18 */
    volatile uint32_t ABR;   /* 0x1C */
    volatile uint32_t DR;    /* 0x20 */
    volatile uint32_t PSMKR; /* 0x24 */
    volatile uint32_t PSMAR; /* 0x28 */
    volatile uint32_t PIR;   /* 0x2C */
    volatile uint32_t LPTR;  /* 0x30 */
} QSPI_TypeDef;

#define OCTOSPI1_BASE      0x58002800U
#define OCTOSPI1 ((QSPI_TypeDef *)OCTOSPI1_BASE)

#endif /* GNSS_PHANTOM_REGISTERS_H */