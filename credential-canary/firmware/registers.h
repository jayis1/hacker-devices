/* STM32G474 register subset for Credential Canary
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef CREDENTIAL_CANARY_REGISTERS_H
#define CREDENTIAL_CANARY_REGISTERS_H
#include <stdint.h>

typedef struct { volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFRL, AFRH, BRR; } gpio_regs_t;
typedef struct { volatile uint32_t CR1, CR2, CR3, BRR, GTPR, RTOR, RQR, ISR, ICR, RDR, TDR, PRESC; } usart_regs_t;
typedef struct { volatile uint32_t CR1, CR2, SMCR, DIER, SR, EGR, CCMR1, CCMR2, CCER, CNT, PSC, ARR, RCR, CCR1, CCR2, CCR3, CCR4, BDTR, DCR, DMAR, OR1, CCMR3, CCR5, CCR6, AF1, AF2, TISEL; } tim_regs_t;
typedef struct { volatile uint32_t CR, CFGR, CIR, APB2RSTR, APB1RSTR1, APB1RSTR2, AHB1ENR, AHB2ENR, AHB3ENR, APB1ENR1, APB1ENR2, APB2ENR; } rcc_regs_t;
typedef struct { volatile uint32_t ACR, KEYR, OPTKEYR, SR, CR, ECCR, RESERVED, OPTR; } flash_regs_t;

#define PERIPH_BASE 0x40000000UL
#define AHB2_BASE (PERIPH_BASE + 0x08000000UL)
#define GPIOA ((gpio_regs_t *)(AHB2_BASE + 0x0000UL))
#define GPIOB ((gpio_regs_t *)(AHB2_BASE + 0x0400UL))
#define GPIOC ((gpio_regs_t *)(AHB2_BASE + 0x0800UL))
#define RCC ((rcc_regs_t *)0x40021000UL)
#define TIM2 ((tim_regs_t *)0x40000000UL)
#define USART1 ((usart_regs_t *)0x40013800UL)
#define USART2 ((usart_regs_t *)0x40004400UL)
#define FLASH ((flash_regs_t *)0x40022000UL)
#define IWDG_KR (*(volatile uint32_t *)0x40003000UL)
#define DWT_CTRL (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
#define DEMCR (*(volatile uint32_t *)0xE000EDFCUL)

#define RCC_AHB2ENR_GPIOAEN (1u << 0)
#define RCC_AHB2ENR_GPIOBEN (1u << 1)
#define RCC_AHB2ENR_GPIOCEN (1u << 2)
#define RCC_APB1ENR1_TIM2EN (1u << 0)
#define RCC_APB1ENR1_USART2EN (1u << 17)
#define RCC_APB2ENR_USART1EN (1u << 14)
#define USART_CR1_UE (1u << 0)
#define USART_CR1_RE (1u << 2)
#define USART_CR1_TE (1u << 3)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_ISR_RXNE (1u << 5)
#define USART_ISR_TXE (1u << 7)
#define USART_ISR_TC (1u << 6)
#define TIM_CR1_CEN (1u << 0)

static inline void gpio_set(gpio_regs_t *g, uint8_t pin) { g->BSRR = 1u << pin; }
static inline void gpio_clear(gpio_regs_t *g, uint8_t pin) { g->BSRR = 1u << (pin + 16u); }
static inline bool gpio_read(gpio_regs_t *g, uint8_t pin) { return (g->IDR & (1u << pin)) != 0u; }
#endif
