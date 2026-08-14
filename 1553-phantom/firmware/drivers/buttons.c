/*
 * buttons.c — single-button debounce + long-press + panic
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * One physical tactile button (PA10) plus a capacitive "panic" pad (PB10).
 * Short press = role cycle / mode; long press = arm confirm or panic
 * disarm depending on state. All callbacks routed to main.c
 * (buttons_long_press) and rt_role_panic().
 */

#include "../board.h"
#include "../registers.h"
#include "buttons.h"
#include "rt_role.h"

static uint32_t press_ms = 0;
static uint8_t  pressed  = 0;
static uint8_t  long_fired = 0;
static uint32_t last_panic_ms = 0;

void buttons_init(void) {
    press_ms = 0;
    pressed = 0;
    long_fired = 0;
}

/* Called from TIM6 1 ms ISR */
void buttons_tick(uint32_t now_ms) {
    uint8_t lvl = GPIO_GET(GPIOA, PA10_BTN) ? 0 : 1;  /* active low */

    /* Panic pad: PB10 pulled down; cap-touch pulls high briefly. */
    if (GPIO_GET(GPIOB, PB10_PANIC_SENSE)) {
        if (now_ms - last_panic_ms > 1000) {
            last_panic_ms = now_ms;
            rt_role_panic();   /* stops all TX, clears arm */
        }
    }

    if (lvl && !pressed) {
        pressed = 1;
        press_ms = now_ms;
        long_fired = 0;
    } else if (!lvl && pressed) {
        pressed = 0;
        if (!long_fired) {
            /* short press — could cycle mode; we let main handle */
        }
    } else if (lvl && pressed && !long_fired &&
               (now_ms - press_ms) >= ARM_BUTTON_HOLD_MS) {
        long_fired = 1;
        buttons_long_press();   /* main.c implements */
    }
}

void buttons_on_press(uint32_t now_ms) {
    /* EXTI fired (falling edge) — record press start */
    press_ms = now_ms;
    pressed = 1;
    long_fired = 0;
}