/*
 * registers.h — STM32G474 peripheral register definitions for Ember-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal direct-register-access definitions (no HAL). Covers the peripherals
 * actually used: RCC, GPIO, I2C1, I2C2, USB, TIM6, TIM7, EXTI, NVIC, SYSCFG,
 * PWR, FLASH.
 */

#ifndef EMBER_REGISTERS_H
#define EMBER_REGISTERS_H

#include <stdint.h>

#define __IO  volatile
#define __I   volatile const
#define __O   volatile

/* --- Base addresses (STM32G474, see RM0440) --- */
#define PERIPH_BASE        0x40000000U
#define APB1PERIPH_BASE    PERIPH_BASE
#define APB2PERIPH_BASE   (PERIPH_BASE + 0x00010000U)
#define AHB1PERIPH_BASE   (PERIPH_BASE + 0x00020000U)
#define AHB2PERIPH_BASE   (PERIPH_BASE + 0x00080000U)

#define RCC_BASE          (AHB1PERIPH_BASE + 0x00001000U)
#define PWR_BASE          (APB1PERIPH_BASE + 0x00000000U)
#define SYSCFG_BASE       (APB2PERIPH_BASE + 0x00000000U)
#define EXTI_BASE         (APB2PERIPH_BASE + 0x00000400U)
#define FLASH_R_BASE      (AHB1PERIPH_BASE + 0x00002000U)

#define GPIOA_BASE        (AHB2PERIPH_BASE + 0x00000000U)
#define GPIOB_BASE        (AHB2PERIPH_BASE + 0x00000400U)
#define GPIOC_BASE        (AHB2PERIPH_BASE + 0x00000800U)
#define GPIOF_BASE        (AHB2PERIPH_BASE + 0x00001C00U)
#define GPIOG_BASE        (AHB2PERIPH_BASE + 0x00002000U)

#define I2C1_BASE         (APB1PERIPH_BASE + 0x00005400U)
#define I2C2_BASE         (APB1PERIPH_BASE + 0x00005800U)
#define USB_BASE          (APB1PERIPH_BASE + 0x00005C00U)
#define USB_PMA_BASE      (APB1PERIPH_BASE + 0x00006000U)

#define TIM6_BASE         (APB1PERIPH_BASE + 0x00001000U)
#define TIM7_BASE         (APB1PERIPH_BASE + 0x00001400U)
#define TIM2_BASE         (APB1PERIPH_BASE + 0x00000000U)

#define NVIC_BASE         (0xE000E100U)
#define SCB_BASE          (0xE000ED00U)

/* --- GPIO --- */
typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __I  uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t LCKR;
    __IO uint32_t AFRL;
    __IO uint32_t AFRH;
    __IO uint32_t BRR;
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOF ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG ((GPIO_TypeDef *)GPIOG_BASE)

/* GPIO mode bits */
#define GPIO_MODE_INPUT    0x0
#define GPIO_MODE_OUTPUT   0x1
#define GPIO_MODE_AF       0x2
#define GPIO_MODE_ANALOG   0x3

/* --- RCC --- */
typedef struct {
    __IO uint32_t CR;
    __IO uint32_t ICSCR;
    __IO uint32_t CRRCR;
    __IO uint32_t CFGR;
    __IO uint32_t PLLCFGR;
    __IO uint32_t RESERVED0;
    __IO uint32_t RESERVED1;
    __IO uint32_t RESERVED2;
    __IO uint32_t CIER;
    __IO uint32_t CIFR;
    __IO uint32_t CICR;
    __IO uint32_t RESERVED3;
    __IO uint32_t RESERVED4;
    __IO uint32_t BSRR;
    __IO uint32_t RESERVED5[2];
    __IO uint32_t AHB1RSTR;
    __IO uint32_t AHB2RSTR;
    __IO uint32_t AHB3RSTR;
    __IO uint32_t RESERVED6;
    __IO uint32_t APB1RSTR1;
    __IO uint32_t APB1RSTR2;
    __IO uint32_t APB2RSTR;
    __IO uint32_t RESERVED7;
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
    __IO uint32_t AHB3ENR;
    __IO uint32_t RESERVED8;
    __IO uint32_t APB1ENR1;
    __IO uint32_t APB1ENR2;
    __IO uint32_t APB2ENR;
    __IO uint32_t RESERVED9;
    __IO uint32_t AHB1SMENR;
    __IO uint32_t AHB2SMENR;
    __IO uint32_t AHB3SMENR;
    __IO uint32_t RESERVED10;
    __IO uint32_t APB1SMENR1;
    __IO uint32_t APB1SMENR2;
    __IO uint32_t APB2SMENR;
    __IO uint32_t RESERVED11;
    __IO uint32_t CCIPR;
    __IO uint32_t RESERVED12;
    __IO uint32_t BDCR;
    __IO uint32_t CSR;
    __IO uint32_t CRRCR;
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION      (1U << 8)
#define RCC_CR_HSIRDY     (1U << 10)
#define RCC_CR_HSEON      (1U << 16)
#define RCC_CR_HSERDY     (1U << 17)
#define RCC_CR_PLLON      (1U << 24)
#define RCC_CR_PLLRDY     (1U << 25)

/* RCC CFGR bits */
#define RCC_CFGR_SW_HSI   0x1
#define RCC_CFGR_SW_PLL   0x3
#define RCC_CFGR_SWS_PLL  (0x3 << 3)

/* RCC AHB1ENR bits */
#define RCC_AHB1ENR_FLASHEN  (1U << 8)
#define RCC_AHB1ENR_CRCEN    (1U << 11)

/* RCC AHB2ENR bits */
#define RCC_AHB2ENR_GPIOAEN  (1U << 0)
#define RCC_AHB2ENR_GPIOBEN  (1U << 1)
#define RCC_AHB2ENR_GPIOCEN  (1U << 2)
#define RCC_AHB2ENR_GPIOFEN  (1U << 5)
#define RCC_AHB2ENR_GPIOGEN  (1U << 6)

/* RCC APB1ENR1 bits */
#define RCC_APB1ENR1_TIM2EN   (1U << 0)
#define RCC_APB1ENR1_TIM6EN   (1U << 4)
#define RCC_APB1ENR1_TIM7EN   (1U << 5)
#define RCC_APB1ENR1_I2C1EN   (1U << 21)
#define RCC_APB1ENR1_USBFSEN  (1U << 23)

/* RCC APB1ENR2 bits */
#define RCC_APB1ENR2_I2C2EN   (1U << 0)

/* --- I2C --- */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t OAR1;
    __IO uint32_t OAR2;
    __IO uint32_t TIMINGR;
    __IO uint32_t TIMEOUTR;
    __IO uint32_t ISR;
    __IO uint32_t ICR;
    __IO uint32_t PECR;
    __IO uint32_t RXDR;
    __IO uint32_t TXDR;
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)
#define I2C2 ((I2C_TypeDef *)I2C2_BASE)

/* I2C CR1 bits */
#define I2C_CR1_PE        (1U << 0)
#define I2C_CR1_RXIE      (1U << 2)
#define I2C_CR1_TXIE      (1U << 1)
#define I2C_CR1_NACKIE    (1U << 4)
#define I2C_CR1_STOPIE    (1U << 5)

/* I2C CR2 bits */
#define I2C_CR2_START     (1U << 13)
#define I2C_CR2_STOP      (1U << 14)
#define I2C_CR2_RD_WRN    (1U << 10)
#define I2C_CR2_AUTOEND   (1U << 15)
#define I2C_CR2_NBYTES_Pos 16
#define I2C_CR2_NBYTES(n) ((n) << I2C_CR2_NBYTES_Pos)

/* I2C ISR bits */
#define I2C_ISR_TXIS      (1U << 1)
#define I2C_ISR_RXNE      (1U << 2)
#define I2C_ISR_NACKF     (1U << 13)
#define I2C_ISR_STOPF     (1U << 5)
#define I2C_ISR_TC        (1U << 6)
#define I2C_ISR_TCR       (1U << 7)
#define I2C_ISR_BUSY      (1U << 15)

/* I2C ICR bits */
#define I2C_ICR_NACKCF    (1U << 13)
#define I2C_ICR_STOPCF    (1U << 5)

/* --- USB (STM32 USB FS, RM0440 §34) --- */
typedef struct {
    __IO uint32_t EP0R;
    __IO uint32_t EP1R;
    __IO uint32_t EP2R;
    __IO uint32_t EP3R;
    __IO uint32_t EP4R;
    __IO uint32_t EP5R;
    __IO uint32_t EP6R;
    __IO uint32_t EP7R;
    __IO uint32_t CNTR;
    __IO uint32_t ISTR;
    __IO uint32_t FNR;
    __IO uint32_t DADDR;
    __IO uint32_t BTABLE;
} USB_TypeDef;

#define USB ((USB_TypeDef *)USB_BASE)

/* USB CNTR bits */
#define USB_CNTR_CTRM      (1U << 0)
#define USB_CNTR_RESETM    (1U << 1)
#define USB_CNTR_SUSPM     (1U << 3)
#define USB_CNTR_WKUPM     (1U << 4)
#define USB_CNTR_FSUSP     (1U << 3)

/* USB ISTR bits */
#define USB_ISTR_CTR       (1U << 0)
#define USB_ISTR_RESET     (1U << 2)
#define USB_ISTR_SUSP      (1U << 3)
#define USB_ISTR_DIR       (1U << 4)
#define USB_ISTR_EP_ID_Pos 0
#define USB_ISTR_EP_ID_MSK 0xFU

/* EP bits */
#define USB_EP_CTR_RX      (1U << 15)
#define USB_EP_CTR_TX      (1U << 0)

/* --- TIM --- */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMCR;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    __IO uint32_t CCMR1;
    __IO uint32_t CCMR2;
    __IO uint32_t CCER;
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
    __IO uint32_t RCR;
    __IO uint32_t CCR1;
    __IO uint32_t CCR2;
    __IO uint32_t CCR3;
    __IO uint32_t CCR4;
    __IO uint32_t BDTR;
    __IO uint32_t DCR;
    __IO uint32_t DMAR;
    __IO uint32_t OR;
} TIM_TypeDef;

#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM6 ((TIM_TypeDef *)TIM6_BASE)
#define TIM7 ((TIM_TypeDef *)TIM7_BASE)

#define TIM_CR1_CEN       (1U << 0)
#define TIM_DIER_UIE      (1U << 0)
#define TIM_SR_UIF        (1U << 0)

/* --- EXTI / SYSCFG --- */
typedef struct {
    __IO uint32_t FPR1;
    __IO uint32_t FPR2;
    __IO uint32_t IMR1;
    __IO uint32_t IMR2;
    __IO uint32_t EMR1;
    __IO uint32_t EMR2;
    __IO uint32_t RESERVED[2];
    __IO uint32_t FTSR1;
    __IO uint32_t FTSR2;
    __IO uint32_t RTSR1;
    __IO uint32_t RTSR2;
    __IO uint32_t SWIER1;
    __IO uint32_t SWIER2;
    __IO uint32_t RPR1;
    __IO uint32_t RPR2;
} EXTI_TypeDef;

#define EXTI ((EXTI_TypeDef *)EXTI_BASE)

typedef struct {
    __IO uint32_t SECCFGR1;
    __IO uint32_t SECCFGR2;
    __IO uint32_t SECCFGR3;
    __IO uint32_t RESERVED0;
    __IO uint32_t CFGR1;
    __IO uint32_t RESERVED1;
    __IO uint32_t EXTICR1;
    __IO uint32_t EXTICR2;
    __IO uint32_t EXTICR3;
    __IO uint32_t EXTICR4;
    __IO uint32_t RESERVED2[4];
    __IO uint32_t SYSCFGR;
} SYSCFG_TypeDef;

#define SYSCFG ((SYSCFG_TypeDef *)SYSCFG_BASE)

/* --- PWR --- */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t CR3;
    __IO uint32_t CR4;
    __IO uint32_t SR1;
    __IO uint32_t SR2;
    __IO uint32_t SCR;
    __IO uint32_t RESERVED0;
    __IO uint32_t PUCRA;
    __IO uint32_t PDCRA;
    __IO uint32_t PUCRB;
    __IO uint32_t PDCRB;
    __IO uint32_t PUCRC;
    __IO uint32_t PDCRC;
    __IO uint32_t PUCRD;
    __IO uint32_t PDCRD;
    __IO uint32_t PUCRE;
    __IO uint32_t PDCRE;
    __IO uint32_t PUCRF;
    __IO uint32_t PDCRF;
    __IO uint32_t PUCRG;
    __IO uint32_t PDCRG;
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)PWR_BASE)

/* --- FLASH controller --- */
typedef struct {
    __IO uint32_t ACR;
    __IO uint32_t PDKEYR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t ECCR;
    __IO uint32_t RESERVED0;
    __IO uint32_t OPTR;
    __IO uint32_t PCROP1SR;
    __IO uint32_t PCROP1ER;
    __IO uint32_t WRP1AR;
    __IO uint32_t WRP1BR;
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)FLASH_R_BASE)

#define FLASH_ACR_LATENCY_Pos 0
#define FLASH_ACR_LATENCY(lat) ((lat) << FLASH_ACR_LATENCY_Pos)
#define FLASH_ACR_PRFTEN      (1U << 8)
#define FLASH_ACR_ICEN        (1U << 9)
#define FLASH_ACR_DCEN        (1U << 10)

/* --- NVIC --- */
typedef struct {
    __IO uint32_t ISER[16];
    __IO uint32_t RESERVED0[16];
    __IO uint32_t ICER[16];
    __IO uint32_t RESERVED1[16];
    __IO uint32_t ISPR[16];
    __IO uint32_t RESERVED2[16];
    __IO uint32_t ICPR[16];
    __IO uint32_t RESERVED3[16];
    __IO uint32_t IABR[16];
} NVIC_TypeDef;

#define NVIC ((NVIC_TypeDef *)NVIC_BASE)

/* IRQ numbers (STM32G474) */
#define USB_FS_IRQn       20
#define I2C1_EV_IRQn      22
#define I2C1_ER_IRQn      23
#define TIM6_DAC1_IRQn    38
#define TIM7_IRQn         39
#define EXTI1_IRQn        7
#define EXTI2_IRQn        8
#define EXTI3_IRQn        9

static inline void nvic_enable(int irq) {
    NVIC->ISER[irq >> 5] = (1U << (irq & 31));
}
static inline void nvic_clear(int irq) {
    NVIC->ICPR[irq >> 5] = (1U << (irq & 31));
}

/* --- SCB --- */
typedef struct {
    __IO uint32_t RESERVED0[2];
    __IO uint32_t VTOR;
} SCB_vtor_TypeDef;

#define SCB_VTOR ((volatile uint32_t *)(SCB_BASE + 0x08U))

#endif /* EMBER_REGISTERS_H */
/* end of file — author: jayis1 */