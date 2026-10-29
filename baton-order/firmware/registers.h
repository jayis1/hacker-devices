/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef BO_REGISTERS_H
#define BO_REGISTERS_H
#include <stdint.h>
/* STM32F401RE RM0368: RCC, GPIO, USART, IWDG; Cortex-M4 SysTick. */
#define MMIO32(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define RCC_AHB1ENR MMIO32(0x40023830u)
#define RCC_APB1ENR MMIO32(0x40023840u)
#define RCC_CSR MMIO32(0x40023874u)
#define GPIOA_BASE 0x40020000u
#define GPIOB_BASE 0x40020400u
#define GPIO_MODER(b) MMIO32((b) + 0x00u)
#define GPIO_PUPDR(b) MMIO32((b) + 0x0cu)
#define GPIO_IDR(b) MMIO32((b) + 0x10u)
#define GPIO_AFRL(b) MMIO32((b) + 0x20u)
#define USART2_SR MMIO32(0x40004400u)
#define USART2_DR MMIO32(0x40004404u)
#define USART2_BRR MMIO32(0x40004408u)
#define USART2_CR1 MMIO32(0x4000440cu)
#define USART2_CR2 MMIO32(0x40004410u)
#define USART2_CR3 MMIO32(0x40004414u)
#define IWDG_KR MMIO32(0x40003000u)
#define IWDG_PR MMIO32(0x40003004u)
#define IWDG_RLR MMIO32(0x40003008u)
#define IWDG_SR MMIO32(0x4000300cu)
#define SYST_CSR MMIO32(0xe000e010u)
#define SYST_RVR MMIO32(0xe000e014u)
#define SYST_CVR MMIO32(0xe000e018u)
#endif
