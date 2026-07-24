/*
 * registers.h — STM32L432 peripheral register definitions for
 *               Joule-Phantom smart-battery MITM implant.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Minimal bare-metal register map; no CMSIS dependency.  Only the
 * peripherals actually used by Joule-Phantom are described.
 */

#ifndef JOULE_PHANTOM_REGISTERS_H
#define JOULE_PHANTOM_REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Cortex-M4 system control                                           */
/* ------------------------------------------------------------------ */
#define SCB_BASE        0xE000ED00u
#define SCB_VTOR        (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_CPACR       (*(volatile uint32_t *)(SCB_BASE + 0x88))
#define SCB_CCR         (*(volatile uint32_t *)(SCB_BASE + 0x14))

#define SYSTICK_BASE    0xE000E010u
#define SYST_CSR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYST_RVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYST_CVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYST_CALIB      (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

#define NVIC_BASE       0xE000E100u
#define NVIC_ISER(n)    (*(volatile uint32_t *)(NVIC_BASE + 0x000 + 4*(n)))
#define NVIC_ICER(n)    (*(volatile uint32_t *)(NVIC_BASE + 0x080 + 4*(n)))
#define NVIC_ISPR(n)    (*(volatile uint32_t *)(NVIC_BASE + 0x100 + 4*(n)))
#define NVIC_ICPR(n)    (*(volatile uint32_t *)(NVIC_BASE + 0x180 + 4*(n)))
#define NVIC_IP(n)      (*(volatile uint8_t  *)(NVIC_BASE + 0x300 + n))

/* ------------------------------------------------------------------ */
/*  RCC                                                                */
/* ------------------------------------------------------------------ */
#define RCC_BASE        0x40021000u
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_ICSCR       (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4C + 4))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR2    (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_CCIPR       (*(volatile uint32_t *)(RCC_BASE + 0x88))

/* RCC bit definitions                                                 */
#define RCC_CR_HSION        (1u << 8)
#define RCC_CR_HSIRDY       (1u << 10)
#define RCC_CR_PLLON        (1u << 24)
#define RCC_CR_PLLRDY       (1u << 25)
#define RCC_CR_MSION        (1u << 0)
#define RCC_CR_MSIRDY       (1u << 1)

#define RCC_AHB2ENR_GPIOAEN (1u << 0)
#define RCC_AHB2ENR_GPIOBEN (1u << 1)
#define RCC_AHB2ENR_ADCEN   (1u << 13)

#define RCC_APB1ENR1_I2C1EN (1u << 21)
#define RCC_APB1ENR1_I2C2EN (1u << 22)
#define RCC_APB1ENR1_USART2EN (1u << 17)
#define RCC_APB1ENR1_TIM2EN (1u << 0)

#define RCC_APB2ENR_SYSCFGEN (1u << 0)
#define RCC_APB2ENR_TIM16EN  (1u << 17)

/* ------------------------------------------------------------------ */
/*  PWR                                                                */
/* ------------------------------------------------------------------ */
#define PWR_BASE        0x40007000u
#define PWR_CR1         (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR2         (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_CR3         (*(volatile uint32_t *)(PWR_BASE + 0x08))
#define PWR_CR4         (*(volatile uint32_t *)(PWR_BASE + 0x0C))
#define PWR_SR1         (*(volatile uint32_t *)(PWR_BASE + 0x10))
#define PWR_SR2         (*(volatile uint32_t *)(PWR_BASE + 0x14))

#define PWR_CR3_UCPD_DBDIS  (1u << 14)

/* ------------------------------------------------------------------ */
/*  GPIO                                                               */
/* ------------------------------------------------------------------ */
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

#define GPIOA           ((GPIO_TypeDef *)0x48000000u)
#define GPIOB           ((GPIO_TypeDef *)0x48000400u)

#define GPIO_MODE_INPUT     0u
#define GPIO_MODE_OUTPUT    1u
#define GPIO_MODE_AF        2u
#define GPIO_MODE_ANALOG    3u

#define GPIO_OTYPE_PP       0u
#define GPIO_OTYPE_OD       1u

#define GPIO_PUPD_NONE      0u
#define GPIO_PUPD_PU        1u
#define GPIO_PUPD_PD        2u

#define GPIO_SPEED_LOW      0u
#define GPIO_SPEED_HIGH     3u

#define GPIO_AF4            4u      /* I2C1/I2C2 on L4 */
#define GPIO_AF7            7u      /* USART2          */

/* ------------------------------------------------------------------ */
/*  I2C (STM32L4 — SMBus capable)                                      */
/* ------------------------------------------------------------------ */
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

#define I2C1            ((I2C_TypeDef *)0x40005400u)
#define I2C2            ((I2C_TypeDef *)0x40005800u)

#define I2C_CR1_PE          (1u << 0)
#define I2C_CR1_TXIE        (1u << 1)
#define I2C_CR1_RXIE        (1u << 2)
#define I2C_CR1_ADDRIE      (1u << 3)
#define I2C_CR1_NACKIE      (1u << 4)
#define I2C_CR1_STOPIE      (1u << 5)
#define I2C_CR1_TCIE        (1u << 6)
#define I2C_CR1_ERRIE       (1u << 7)
#define I2C_CR1_SMBHEN      (1u << 20)
#define I2C_CR1_SMBDEN      (1u << 21)
#define I2C_CR1_ALERTEN     (1u << 22)
#define I2C_CR1_PECEN       (1u << 23)

#define I2C_CR2_SADD_MASK   0x3FFu
#define I2C_CR2_RD_WRN      (1u << 10)
#define I2C_CR2_AUTOEND     (1u << 15)
#define I2C_CR2_RELOAD      (1u << 24)
#define I2C_CR2_NBYTES_POS  16
#define I2C_CR2_NBYTES_MASK 0xFFu
#define I2C_CR2_START       (1u << 13)
#define I2C_CR2_STOP        (1u << 14)
#define I2C_CR2_PECBYTE     (1u << 26)

#define I2C_ISR_TXE         (1u << 0)
#define I2C_ISR_TXIS        (1u << 1)
#define I2C_ISR_RXNE        (1u << 2)
#define I2C_ISR_ADDR        (1u << 3)
#define I2C_ISR_NACKF       (1u << 4)
#define I2C_ISR_STOPF       (1u << 5)
#define I2C_ISR_TC          (1u << 6)
#define I2C_ISR_TCR         (1u << 7)
#define I2C_ISR_BERR        (1u << 8)
#define I2C_ISR_ARLO        (1u << 9)
#define I2C_ISR_OVR         (1u << 10)
#define I2C_ISR_PECERR      (1u << 11)
#define I2C_ISR_TIMEOUT     (1u << 12)
#define I2C_ISR_ALERT       (1u << 13)
#define I2C_ISR_BUSY        (1u << 15)

#define I2C_ICR_CLEAR_ALL   0x3F38u

/* ------------------------------------------------------------------ */
/*  USART                                                              */
/* ------------------------------------------------------------------ */
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

#define USART2          ((USART_TypeDef *)0x40004400u)

#define USART_CR1_UE        (1u << 0)
#define USART_CR1_RE        (1u << 2)
#define USART_CR1_TE        (1u << 3)
#define USART_CR1_RXNEIE    (1u << 5)
#define USART_CR1_TCIE      (1u << 6)
#define USART_CR1_TXEIE     (1u << 7)

#define USART_ISR_TXE       (1u << 7)
#define USART_ISR_TC        (1u << 6)
#define USART_ISR_RXNE      (1u << 5)

/* ------------------------------------------------------------------ */
/*  ADC (L4 — simplified)                                              */
/* ------------------------------------------------------------------ */
#define ADC1_BASE       0x50040000u
#define ADC_ISR         (*(volatile uint32_t *)(ADC1_BASE + 0x08))
#define ADC_CR          (*(volatile uint32_t *)(ADC1_BASE + 0x08 + 8))
#define ADC_CFGR        (*(volatile uint32_t *)(ADC1_BASE + 0x24))
#define ADC_SQR1        (*(volatile uint32_t *)(ADC1_BASE + 0x30))
#define ADC_DR          (*(volatile uint32_t *)(ADC1_BASE + 0x40))

/* ------------------------------------------------------------------ */
/*  TIM2 (1 ms tick) / TIM16 (glitch pulse)                            */
/* ------------------------------------------------------------------ */
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
    volatile uint32_t CCR1;     /* 0x34 */
} TIM_TypeDef;

#define TIM2            ((TIM_TypeDef *)0x40000000u)
#define TIM16           ((TIM_TypeDef *)0x40014400u)

#define TIM_CR1_CEN         (1u << 0)
#define TIM_DIER_UIE        (1u << 0)
#define TIM_SR_UIF          (1u << 0)

/* ------------------------------------------------------------------ */
/*  IRQ numbers (L432)                                                 */
/* ------------------------------------------------------------------ */
#define IRQ_I2C1_EV         31
#define IRQ_I2C1_ER         32
#define IRQ_I2C2_EV         33
#define IRQ_I2C2_ER         34
#define IRQ_USART2          38
#define IRQ_TIM2            28
#define IRQ_TIM16           44

/* ------------------------------------------------------------------ */
/*  Flash / SRAM base for linker                                       */
/* ------------------------------------------------------------------ */
#define FLASH_BASE_ADDR     0x08000000u
#define FLASH_SIZE          (256 * 1024)
#define SRAM_BASE_ADDR      0x20000000u
#define SRAM_SIZE           (64 * 1024)

#ifdef __cplusplus
}
#endif

#endif /* JOULE_PHANTOM_REGISTERS_H */