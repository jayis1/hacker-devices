/*
 * tesla-phantom — drivers/trigger.c
 * Trigger routing: external SMA, internal timer, magnetic signature, manual.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

static trigger_source_t current_src = TRIG_EXTERNAL;
static volatile uint8_t trigger_armed_flag = 0;
static volatile uint8_t trigger_pending = 0;

/* ── Public API ───────────────────────────────────────────────────── */

int trigger_init(void) {
    current_src = TRIG_EXTERNAL;
    trigger_armed_flag = 0;
    trigger_pending = 0;

    /* Configure the external trigger pin (PC4) as EXTI input
     * (already done in board_init_exti) */

    /* Configure trigger output to FPGA (PC5) — default low */
    gpio_clr(GPIO(GPIOC), TRIG_OUT_PIN);

    return 0;
}

int trigger_set_source(trigger_source_t src) {
    current_src = src;

    /* Configure FPGA trigger mode accordingly */
    fpga_set_trigger_mode(src);

    /* For magnetic signature trigger, configure FPGA pattern matcher */
    if (src == TRIG_MAGNETIC) {
        /* Default threshold and pattern — user can override */
        fpga_set_mag_trigger(500, 0);
    }

    return 0;
}

int trigger_arm(void) {
    trigger_armed_flag = 1;
    trigger_pending = 0;

    /* Clear any pending EXTI interrupt */
    EXTI->PR1 = (1U << 4);  /* Clear EXTI line 4 pending */

    /* Arm the FPGA */
    fpga_arm();

    return 0;
}

int trigger_disarm(void) {
    trigger_armed_flag = 0;
    trigger_pending = 0;

    /* Disarm the FPGA */
    fpga_disarm();

    return 0;
}

int trigger_wait(uint32_t timeout_ms) {
    uint32_t start = millis();

    while (trigger_armed_flag && !trigger_pending) {
        if ((millis() - start) > timeout_ms) {
            return -1;  /* timeout */
        }

        /* For manual trigger, fire immediately */
        if (current_src == TRIG_MANUAL) {
            fpga_fire();
            trigger_pending = 1;
        }

        /* For external trigger, the FPGA handles it.
         * Check FPGA status for fire confirmation. */
        if (current_src == TRIG_EXTERNAL || current_src == TRIG_MAGNETIC) {
            uint8_t stat = fpga_get_status();
            if (stat & FPGA_ST_FIRED) {
                trigger_pending = 1;
            }
        }
    }

    trigger_armed_flag = 0;
    return trigger_pending ? 0 : -1;
}

/* EXTI line 4 interrupt handler (external trigger) */
void exti4_irqhandler(void) {
    /* Clear pending bit */
    EXTI->PR1 = (1U << 4);

    if (trigger_armed_flag && current_src == TRIG_EXTERNAL) {
        /* Forward trigger to FPGA via PC5 */
        gpio_set(GPIO(GPIOC), TRIG_OUT_PIN);
        delay_us(1);
        gpio_clr(GPIO(GPIOC), TRIG_OUT_PIN);

        trigger_pending = 1;
        trigger_ext_callback();
    }
}