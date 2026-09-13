/* Credential Canary board support package
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "board.h"
#include "registers.h"

static volatile uint32_t millisecond_ticks;
static bool bypass_state = true;
static uint8_t usb_tx_shadow[256];
static uint16_t usb_tx_length;

static void gpio_output(gpio_regs_t *gpio, uint8_t pin, bool open_drain) {
    gpio->MODER = (gpio->MODER & ~(3u << (pin * 2u))) | (1u << (pin * 2u));
    if (open_drain) gpio->OTYPER |= 1u << pin;
    else gpio->OTYPER &= ~(1u << pin);
    gpio->OSPEEDR |= 3u << (pin * 2u);
}

static void gpio_input(gpio_regs_t *gpio, uint8_t pin, uint8_t pull) {
    gpio->MODER &= ~(3u << (pin * 2u));
    gpio->PUPDR = (gpio->PUPDR & ~(3u << (pin * 2u))) | ((uint32_t)pull << (pin * 2u));
}

static void clock_init(void) {
    /* Production boards use 16 MHz HSI + PLL at 170 MHz. The early GPIO
       setup intentionally runs before PLL switching so bypass is deterministic. */
    FLASH->ACR = (FLASH->ACR & ~7u) | 4u;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN | RCC_APB1ENR1_USART2EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC->AHB2ENR;
}

static void timer_init(void) {
    TIM2->CR1 = 0u;
    TIM2->PSC = (SYSTEM_CLOCK_HZ / 1000000u) - 1u;
    TIM2->ARR = 0xFFFFFFFFu;
    TIM2->CNT = 0u;
    TIM2->CR1 = TIM_CR1_CEN;
    DEMCR |= 1u << 24;
    DWT_CYCCNT = 0u;
    DWT_CTRL |= 1u;
}

static void uart_init(usart_regs_t *uart, uint32_t baud) {
    uart->CR1 = 0u;
    uart->BRR = APB_CLOCK_HZ / baud;
    uart->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
}

void board_init(void) {
    clock_init();
    gpio_output(GPIOB, PIN_BYPASS, false);
    gpio_set(GPIOB, PIN_BYPASS); /* energized bypass relay is fail transparent */
    gpio_output(GPIOB, PIN_LED_R, false);
    gpio_output(GPIOB, PIN_LED_G, false);
    gpio_output(GPIOB, PIN_LED_B, false);
    gpio_input(GPIOA, PIN_WA_D0, 1u);
    gpio_input(GPIOA, PIN_WA_D1, 1u);
    gpio_input(GPIOA, PIN_WB_D0, 1u);
    gpio_input(GPIOA, PIN_WB_D1, 1u);
    gpio_input(GPIOC, PIN_TAMPER, 1u);
    gpio_input(GPIOC, PIN_AUTH, 1u);
    gpio_output(GPIOB, PIN_RS485_A_DE, false);
    gpio_output(GPIOB, PIN_RS485_B_DE, false);
    gpio_clear(GPIOB, PIN_RS485_A_DE);
    gpio_clear(GPIOB, PIN_RS485_B_DE);
    timer_init();
    uart_init(USART1, OSDP_BAUD_DEFAULT);
    uart_init(USART2, OSDP_BAUD_DEFAULT);
    board_set_led(8u, 8u, 0u);
    IWDG_KR = 0xCCCCu;
}

uint32_t board_micros(void) { return TIM2->CNT; }
uint32_t board_millis(void) { return millisecond_ticks + board_micros() / 1000u; }

void board_delay_us(uint32_t us) {
    uint32_t start = board_micros();
    while ((uint32_t)(board_micros() - start) < us) { }
}

void board_feed_watchdog(void) { IWDG_KR = 0xAAAAu; }

void board_set_led(uint8_t r, uint8_t g, uint8_t b) {
    if (r) gpio_set(GPIOB, PIN_LED_R); else gpio_clear(GPIOB, PIN_LED_R);
    if (g) gpio_set(GPIOB, PIN_LED_G); else gpio_clear(GPIOB, PIN_LED_G);
    if (b) gpio_set(GPIOB, PIN_LED_B); else gpio_clear(GPIOB, PIN_LED_B);
}

void board_set_bypass(bool bypass) {
    bypass_state = bypass;
    if (bypass) gpio_set(GPIOB, PIN_BYPASS);
    else gpio_clear(GPIOB, PIN_BYPASS);
}

bool board_tamper_asserted(void) { return !gpio_read(GPIOC, PIN_TAMPER); }
bool board_authorization_asserted(void) { return !gpio_read(GPIOC, PIN_AUTH); }

void board_usb_write(const uint8_t *data, uint16_t length) {
    /* USB device transport replaces this shadow queue in the production HAL.
       Keeping a bounded copy makes command behavior testable without USB silicon. */
    usb_tx_length = (uint16_t)MIN_U32(length, sizeof usb_tx_shadow);
    for (uint16_t i = 0; i < usb_tx_length; ++i) usb_tx_shadow[i] = data[i];
}

int board_usb_read(uint8_t *data, uint16_t capacity) {
    (void)data;
    (void)capacity;
    return 0;
}

void board_rs485_drive(interface_t iface, bool enabled) {
    uint8_t pin = iface == IFACE_OSDP_READER ? PIN_RS485_A_DE : PIN_RS485_B_DE;
    if (enabled) gpio_set(GPIOB, pin); else gpio_clear(GPIOB, pin);
}

void board_rs485_write(interface_t iface, const uint8_t *data, uint16_t length) {
    usart_regs_t *uart = iface == IFACE_OSDP_READER ? USART1 : USART2;
    board_rs485_drive(iface, true);
    for (uint16_t i = 0; i < length; ++i) {
        while ((uart->ISR & USART_ISR_TXE) == 0u) { }
        uart->TDR = data[i];
    }
    while ((uart->ISR & USART_ISR_TC) == 0u) { }
    board_rs485_drive(iface, false);
}
