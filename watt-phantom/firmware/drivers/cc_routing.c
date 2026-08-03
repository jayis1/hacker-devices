/*
 * cc_routing.c — TMUX2512 CC line routing mux driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls the TMUX2512 4:1 analog multiplexers that route the CC1 and
 * CC2 lines between the source and sink USB-C ports. This allows
 * WattPhantom to operate in different modes:
 *
 *   PASSTHROUGH: CC lines pass through transparently (MITM monitoring)
 *   ISOLATED:    Each port's CC lines are isolated (independent control)
 *   CROSSOVER:   CC1 and CC2 are swapped (orientation reversal)
 *   MCU_CTRL:    MCU directly drives the CC lines (active attack)
 *
 * The TMUX2512 is controlled by two GPIO pins per mux (SEL0, SEL1).
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* ---- GPIO pin definitions ---- */
/* TMUX2512 #1 (source CC): PA4 = SEL0, PA5 = SEL1 */
/* TMUX2512 #2 (sink CC):   PA6 = SEL0, PA7 = SEL1 */
#define MUX1_SEL0_PIN  4
#define MUX1_SEL1_PIN  5
#define MUX2_SEL0_PIN  6
#define MUX2_SEL1_PIN  7

/* ---- GPIO write helper ---- */
static void gpio_write_pa(uint8_t pin, uint8_t val) {
    volatile uint32_t *bsrr = (volatile uint32_t *)(GPIOA_BASE + GPIO_BSRR_OFFSET);
    if (val) {
        *bsrr = (1u << pin);
    } else {
        *bsrr = (1u << (pin + 16));
    }
}

/* ---- Set CC routing mode ---- */
void cc_routing_set(cc_routing_mode_t mode) {
    uint8_t sel1_src, sel0_src, sel1_snk, sel0_snk;

    switch (mode) {
    case CC_MODE_PASSTHROUGH:
        sel0_src = 0; sel1_src = 0;
        sel0_snk = 0; sel1_snk = 0;
        break;
    case CC_MODE_ISOLATED:
        sel0_src = 1; sel1_src = 0;
        sel0_snk = 1; sel1_snk = 0;
        break;
    case CC_MODE_CROSSOVER:
        sel0_src = 0; sel1_src = 1;
        sel0_snk = 0; sel1_snk = 1;
        break;
    case CC_MODE_MCU_CTRL:
        sel0_src = 1; sel1_src = 1;
        sel0_snk = 1; sel1_snk = 1;
        break;
    default:
        sel0_src = 0; sel1_src = 0;
        sel0_snk = 0; sel1_snk = 0;
        break;
    }

    gpio_write_pa(MUX1_SEL0_PIN, sel0_src);
    gpio_write_pa(MUX1_SEL1_PIN, sel1_src);
    gpio_write_pa(MUX2_SEL0_PIN, sel0_snk);
    gpio_write_pa(MUX2_SEL1_PIN, sel1_snk);
}