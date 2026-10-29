/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "registers.h"

static volatile uint32_t ticks;
static uint32_t reset_flags;
void SysTick_Handler(void)
{
    ++ticks;
}
uint32_t board_millis(void)
{
    return ticks;
}
uint32_t board_reset_flags(void)
{
    return reset_flags;
}
void board_init(void)
{
    /* Reset HSI clock; no target-facing output or internal pull enabled. */
    reset_flags = RCC_CSR;
    RCC_CSR |= 1u << 24;
    RCC_AHB1ENR |= 3u;
    RCC_APB1ENR |= 1u << 17;
    (void)RCC_AHB1ENR;
    (void)RCC_APB1ENR;
    GPIO_MODER(GPIOA_BASE) &= ~((3u << 0) | (3u << 2) | (3u << 8));
    GPIO_MODER(GPIOB_BASE) &= ~3u;
    GPIO_PUPDR(GPIOA_BASE) &= ~((3u << 0) | (3u << 2) | (3u << 8));
    GPIO_PUPDR(GPIOB_BASE) &= ~3u;
    /* PA2 TX goes only to ST-LINK; USART receiver is disabled. */
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3u << 4)) | (2u << 4);
    GPIO_AFRL(GPIOA_BASE) = (GPIO_AFRL(GPIOA_BASE) & ~(15u << 8)) | (7u << 8);
    USART2_CR1 = 0;
    USART2_CR2 = 0;
    USART2_CR3 = 0;
    USART2_BRR = (CORE_HZ + SERIAL_BAUD / 2u) / SERIAL_BAUD;
    USART2_CR1 = (1u << 13) | (1u << 3);
    ticks = 0;
    SYSF_RVR = CORE_HZ / 1000u - 1u;
    SYSF_CVR = 0;
    SYSF_CSR = 7u;
}
uint8_t board_levels(void)
{
    uint32_t a = GPIO_IDR(GPIOA_BASE);
    uint32_t b = GPIO_IDR(GPIOB_BASE);
    /* Two port reads: no claim of atomic acquisition across ports. */
    return (uint8_t)((a & 3u) | ((a >> 2) & 4u) | ((b & 1u) << 3));
}
bool board_watchdog_start(void)
{
    RCC_CSR |= 1u;
    uint32_t start = board_millis();
    while (!(RCC_CSR & 2u)) {
        if (board_millis() - start >= 100u)
            return false;
    }
    IWDG_KR = 0x5555u;
    IWDG_PR = 4u;
    IWDG_RLR = 1023u;
    start = board_millis();
    while (IWDG_SR) {
        if (board_millis() - start >= 100u)
            return false;
    }
    IWDG_KR = 0xaaaau;
    IWDG_KR = 0xccccu;
    return true;
}
void board_watchdog_feed(void)
{
    IWDG_KR = 0xaaaau;
}
