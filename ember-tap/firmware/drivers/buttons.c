/*
 * buttons.c — debounced button reader (3 tactile buttons)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Buttons: PA0 (UP), PA1 (DN), PA2 (SEL), all active-low with pull-ups.
 */

#include "buttons.h"
#include "board.h"
#include "registers.h"

static uint8_t deb_state = 0x07;   /* all released (pull-ups) */
static uint8_t deb_prev  = 0x07;
static uint8_t edge_latch = 0;

void buttons_init(void) {
    /* Enable GPIOA clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* PA0/PA1/PA2 as input with pull-up */
    uint32_t mask = (0x3U << (BUTTON_UP_PIN * 2)) |
                    (0x3U << (BUTTON_DN_PIN * 2)) |
                    (0x3U << (BUTTON_SEL_PIN * 2));
    GPIOA->MODER &= ~mask;   /* input */
    GPIOA->PUPDR |= (0x1U << (BUTTON_UP_PIN * 2)) |
                    (0x1U << (BUTTON_DN_PIN * 2)) |
                    (0x1U << (BUTTON_SEL_PIN * 2));
}

/* Called from the 1 ms SysTick / TIM6 handler */
void buttons_tick(void) {
    uint8_t raw = 0x07;
    if (!(GPIOA->IDR & (1U << BUTTON_UP_PIN)))  raw &= ~0x01;
    if (!(GPIOA->IDR & (1U << BUTTON_DN_PIN)))  raw &= ~0x02;
    if (!(GPIOA->IDR & (1U << (BUTTON_SEL_PIN)))) raw &= ~0x04;

    /* Shift-register debounce: only accept state if stable for N ticks */
    static uint8_t shift = 0x07;
    static uint8_t stable_cnt = 0;
    if (raw == shift) {
        if (stable_cnt < 8) stable_cnt++;
        if (stable_cnt == 8) {
            uint8_t pressed = deb_state & ~raw;   /* newly pressed */
            edge_latch |= pressed;
            deb_prev = deb_state;
            deb_state = raw;
        }
    } else {
        stable_cnt = 0;
        shift = raw;
    }
}

uint8_t buttons_read(void) {
    uint8_t e = edge_latch;
    edge_latch = 0;
    return e;
}

/* end of file — author: jayis1 */