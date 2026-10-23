/* STM32G474 register subset used by LIN Mimic. Author: jayis1. */
#ifndef LIN_MIMIC_REGISTERS_H
#define LIN_MIMIC_REGISTERS_H
#include <stdint.h>
#define REG32(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define RCC_BASE 0x40021000u
#define GPIOA_BASE 0x48000000u
#define GPIOB_BASE 0x48000400u
#define USART2_BASE 0x40004400u
#define USART3_BASE 0x40004800u
#define TIM2_BASE 0x40000000u
#define ADC1_BASE 0x50000000u
#define RCC_AHB2ENR REG32(RCC_BASE+0x4Cu)
#define RCC_APB1ENR1 REG32(RCC_BASE+0x58u)
#define GPIO_MODER(b) REG32((b)+0x00u)
#define GPIO_AFRL(b) REG32((b)+0x20u)
#define USART_CR1(b) REG32((b)+0x00u)
#define USART_CR2(b) REG32((b)+0x04u)
#define USART_BRR(b) REG32((b)+0x0Cu)
#define USART_ISR(b) REG32((b)+0x1Cu)
#define USART_RDR(b) REG32((b)+0x24u)
#define USART_TDR(b) REG32((b)+0x28u)
#define USART_CR2_LINEN (1u<<14)
#define USART_CR2_LBDL (1u<<5)
#define USART_CR1_UE (1u<<0)
#define USART_CR1_RE (1u<<2)
#define USART_CR1_TE (1u<<3)
#define USART_ISR_RXNE (1u<<5)
#define USART_ISR_TXE (1u<<7)
#define USART_ISR_LBDF (1u<<8)
#define TIM_CR1(b) REG32((b)+0x00u)
#define TIM_CNT(b) REG32((b)+0x24u)
#define ADC_DR(b) REG32((b)+0x40u)
#endif
