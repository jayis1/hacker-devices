/* Minimal STM32G474 register map used by DALI Sentinel.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef DALI_SENTINEL_REGISTERS_H
#define DALI_SENTINEL_REGISTERS_H
#include <stdint.h>
typedef struct { volatile uint32_t MODER,OTYPER,OSPEEDR,PUPDR,IDR,ODR,BSRR,LCKR,AFRL,AFRH,BRR; } gpio_regs_t;
typedef struct { volatile uint32_t CR1,CR2,SMCR,DIER,SR,EGR,CCMR1,CCMR2,CCER,CNT,PSC,ARR,RCR,CCR1,CCR2,CCR3,CCR4,BDTR,DCR,DMAR,OR1,CCMR3,CCR5,CCR6,AF1,AF2,TISEL; } tim_regs_t;
typedef struct { volatile uint32_t CR1,CR2,CR3,BRR,GTPR,RTOR,RQR,ISR,ICR,RDR,TDR,PRESC; } usart_regs_t;
typedef struct { volatile uint32_t CR,ICSCR,CFGR,PLLCFGR,RES0[2],CIER,CIFR,CICR,AHB1RSTR,AHB2RSTR,AHB3RSTR,RES1,APB1RSTR1,APB1RSTR2,APB2RSTR,RES2,AHB1ENR,AHB2ENR,AHB3ENR,RES3,APB1ENR1,APB1ENR2,APB2ENR; } rcc_regs_t;
#define PERIPH(addr,type) ((type *)(uintptr_t)(addr))
#define RCC PERIPH(0x40021000u,rcc_regs_t)
#define GPIOA PERIPH(0x48000000u,gpio_regs_t)
#define GPIOB PERIPH(0x48000400u,gpio_regs_t)
#define GPIOC PERIPH(0x48000800u,gpio_regs_t)
#define TIM2 PERIPH(0x40000000u,tim_regs_t)
#define TIM3 PERIPH(0x40000400u,tim_regs_t)
#define USART1 PERIPH(0x40013800u,usart_regs_t)
#define BIT(n) (1u << (n))
#define PIN_RX_CTRL 0u
#define PIN_RX_GEAR 1u
#define PIN_TX_CTRL 4u
#define PIN_TX_GEAR 5u
#define PIN_BYPASS 8u
#define PIN_BYPASS_FB 9u
#define PIN_ARM 13u
#define PIN_AUTH 14u
#define TIM_DIER_CC1IE BIT(1)
#define TIM_DIER_CC2IE BIT(2)
#define TIM_SR_CC1IF BIT(1)
#define TIM_SR_CC2IF BIT(2)
#define USART_ISR_RXNE BIT(5)
#define USART_ISR_TXE BIT(7)
#endif
