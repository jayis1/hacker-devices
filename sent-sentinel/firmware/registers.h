/*
 * SENT Sentinel STM32G474 and capture register map
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef SENT_SENTINEL_REGISTERS_H
#define SENT_SENTINEL_REGISTERS_H

#include <stdint.h>

#define SS_REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define SS_RCC_BASE 0x40021000u
#define SS_GPIOA_BASE 0x48000000u
#define SS_GPIOB_BASE 0x48000400u
#define SS_GPIOC_BASE 0x48000800u
#define SS_TIM2_BASE 0x40000000u
#define SS_TIM4_BASE 0x40000800u
#define SS_ADC12_BASE 0x50000000u
#define SS_DMA1_BASE 0x40020000u
#define SS_CRC_BASE 0x40023000u
#define SS_USB_BASE 0x40005C00u

#define SS_RCC_AHB2ENR SS_REG32(SS_RCC_BASE + 0x4Cu)
#define SS_RCC_APB1ENR1 SS_REG32(SS_RCC_BASE + 0x58u)
#define SS_RCC_APB2ENR SS_REG32(SS_RCC_BASE + 0x60u)
#define SS_GPIO_MODER(base) SS_REG32((base) + 0x00u)
#define SS_GPIO_AFRL(base) SS_REG32((base) + 0x20u)
#define SS_GPIO_AFRH(base) SS_REG32((base) + 0x24u)
#define SS_TIM_CR1(base) SS_REG32((base) + 0x00u)
#define SS_TIM_DIER(base) SS_REG32((base) + 0x0Cu)
#define SS_TIM_SR(base) SS_REG32((base) + 0x10u)
#define SS_TIM_CCMR1(base) SS_REG32((base) + 0x18u)
#define SS_TIM_CCER(base) SS_REG32((base) + 0x20u)
#define SS_TIM_CNT(base) SS_REG32((base) + 0x24u)
#define SS_TIM_PSC(base) SS_REG32((base) + 0x28u)
#define SS_TIM_ARR(base) SS_REG32((base) + 0x2Cu)
#define SS_TIM_CCR1(base) SS_REG32((base) + 0x34u)
#define SS_TIM_CCR2(base) SS_REG32((base) + 0x38u)
#define SS_ADC_ISR SS_REG32(SS_ADC12_BASE + 0x00u)
#define SS_ADC_CR SS_REG32(SS_ADC12_BASE + 0x08u)
#define SS_ADC_CFGR SS_REG32(SS_ADC12_BASE + 0x0Cu)
#define SS_ADC_SQR1 SS_REG32(SS_ADC12_BASE + 0x30u)
#define SS_ADC_DR SS_REG32(SS_ADC12_BASE + 0x40u)

#define SS_RCC_GPIOAEN (1u << 0)
#define SS_RCC_GPIOBEN (1u << 1)
#define SS_RCC_GPIOCEN (1u << 2)
#define SS_RCC_TIM2EN (1u << 0)
#define SS_RCC_TIM4EN (1u << 2)
#define SS_TIM_CEN (1u << 0)
#define SS_TIM_UIE (1u << 0)
#define SS_TIM_CC1IE (1u << 1)
#define SS_TIM_CC2IE (1u << 2)
#define SS_TIM_CC1E (1u << 0)
#define SS_TIM_CC1P (1u << 1)
#define SS_TIM_CC2E (1u << 4)
#define SS_TIM_CC2P (1u << 5)
#define SS_ADC_ADEN (1u << 0)
#define SS_ADC_ADSTART (1u << 2)
#define SS_ADC_EOC (1u << 2)

#define SS_CAPTURE_MAGIC 0x53454E54u
#define SS_CAPTURE_VERSION 1u
#define SS_CAPTURE_STATUS_OVERFLOW (1u << 0)
#define SS_CAPTURE_STATUS_CLOCK_BAD (1u << 1)
#define SS_CAPTURE_STATUS_FRONTEND_FAULT (1u << 2)

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t edge_ticks;
    uint16_t analog_counts;
    uint8_t channel;
    uint8_t status;
} ss_capture_record_t;

#endif
