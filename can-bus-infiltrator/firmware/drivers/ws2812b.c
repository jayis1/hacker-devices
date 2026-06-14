/*
 * ws2812b.c — WS2812B RGB LED driver for STM32F407
 * Bit-bang on GPIO PE7 (LED_DIN)
 *
 * WS2812B timing (approximate):
 *   0 bit: ~0.4 µs high, ~0.85 µs low
 *   1 bit: ~0.8 µs high, ~0.45 µs low
 *   Reset: >50 µs low
 *   Data: G[7:0] R[7:0] B[7:0] per LED
 */

#include "ws2812b.h"
#include "board.h"
#include "registers.h"

#define WS2812B_PORT     GPIOE
#define WS2812B_PIN      7  /* PE7 */

/* Timing constants in CPU cycles at 168 MHz */
#define T0H_CYCLES   67    /* ~0.4 µs */
#define T0L_CYCLES   143   /* ~0.85 µs */
#define T1H_CYCLES   134   /* ~0.8 µs */
#define T1L_CYCLES   76    /* ~0.45 µs */
#define RESET_CYCLES 8400  /* ~50 µs */

static inline void ws2812b_delay(volatile uint32_t cycles) {
    while (cycles--) __asm__ volatile("nop");
}

static inline void ws2812b_set_pin(void) {
    WS2812B_PORT->BSRR = (1U << WS2812B_PIN);
}

static inline void ws2812b_clear_pin(void) {
    WS2812B_PORT->BSRR = (1U << (WS2812B_PIN + 16));
}

void ws2812b_init(void) {
    /* Configure PE7 as GPIO output (already done in main, but ensure) */
    WS2812B_PORT->MODER &= ~(3U << (WS2812B_PIN * 2));
    WS2812B_PORT->MODER |= (1U << (WS2812B_PIN * 2));  /* Output */
    WS2812B_PORT->OSPEEDR |= (3U << (WS2812B_PIN * 2));  /* Very high speed */
    WS2812B_PORT->OTYPER &= ~(1U << WS2812B_PIN);  /* Push-pull */
    WS2812B_PORT->PUPDR &= ~(3U << (WS2812B_PIN * 2));  /* No pull */

    ws2812b_clear_pin();
}

static void ws2812b_send_byte(uint8_t byte) {
    for (int8_t bit = 7; bit >= 0; bit--) {
        if (byte & (1 << bit)) {
            /* Send '1' bit: T1H high, T1L low */
            ws2812b_set_pin();
            ws2812b_delay(T1H_CYCLES);
            ws2812b_clear_pin();
            ws2812b_delay(T1L_CYCLES);
        } else {
            /* Send '0' bit: T0H high, T0L low */
            ws2812b_set_pin();
            ws2812b_delay(T0H_CYCLES);
            ws2812b_clear_pin();
            ws2812b_delay(T0L_CYCLES);
        }
    }
}

void ws2812b_send(const ws2812b_color_t *colors, uint16_t count) {
    /* Disable interrupts for precise timing */
    __disable_irq();

    for (uint16_t i = 0; i < count; i++) {
        ws2812b_send_byte(colors[i].g);  /* Green first */
        ws2812b_send_byte(colors[i].r);
        ws2812b_send_byte(colors[i].b);
    }

    /* Reset signal: >50 µs low */
    ws2812b_clear_pin();
    ws2812b_delay(RESET_CYCLES);

    /* Re-enable interrupts */
    __enable_irq();
}

void ws2812b_clear(uint16_t count) {
    ws2812b_color_t black = {0, 0, 0};
    for (uint16_t i = 0; i < count; i++) {
        /* We can't create a VLA of structs in C99 portably, so send one at a time */
    }
    /* Send zeros for all LEDs */
    ws2812b_color_t off[LED_COUNT] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    ws2812b_send(off, count);
}