/*
 * trigger.c — Trigger subsystem driver
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Manages the multi-source trigger pipeline. All trigger sources are
 * routed through the FPGA, which arbitrates them and generates the
 * glitch enable signal. This driver configures the trigger source,
 * arms the pipeline, and checks whether the trigger has fired.
 *
 * The UART trigger word is detected by the MCU (in the UART monitor
 * ISR) and communicated to the FPGA via a GPIO. The power envelope
 * threshold is set via an analog comparator (TLV3501) whose output
 * goes directly to the FPGA.
 */

#include "trigger.h"
#include "fpga_if.h"
#include "../registers.h"
#include "../board.h"

/* ---- Private state ------------------------------------------------- */

static trigger_config_t g_config;
static uint8_t  g_uart_word_buf[16];
static uint8_t  g_uart_word_len;
static uint8_t  g_uart_match_idx;

/* ---- Initialization ------------------------------------------------ */

void trigger_init(void)
{
    /*
     * Configure trigger input pins:
     *  PC0 = GPIO trigger input (from external source)
     *  PC1 = External sync input (from sideprobe or other device)
     *  PC2 = UART word trigger output (MCU → FPGA)
     *  PC3 = Manual trigger output (MCU → FPGA)
     *
     * The FPGA reads PC0/PC1 and drives the glitch. The MCU drives
     * PC2/PC3 to signal UART-word and manual triggers.
     */
    gpio_config(GPIOC, TRIG_GPIO_INPUT_PIN, GPIO_MODE_INPUT,
                GPIO_OSPEED_VHIGH, GPIO_PUPD_PULLDOWN, 0);
    gpio_config(GPIOC, TRIG_EXT_SYNC_PIN, GPIO_MODE_INPUT,
                GPIO_OSPEED_VHIGH, GPIO_PUPD_PULLDOWN, 0);
    gpio_config(GPIOC, TRIG_UART_WORD_PIN, GPIO_MODE_OUTPUT,
                GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 0);
    gpio_config(GPIOC, TRIG_MANUAL_PIN, GPIO_MODE_OUTPUT,
                GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 0);

    /* Default all outputs low */
    GPIOC->BRR = BIT(TRIG_UART_WORD_PIN);
    GPIOC->BRR = BIT(TRIG_MANUAL_PIN);

    /* Zero the config */
    for (uint8_t *p = (uint8_t *)&g_config;
         p < (uint8_t *)&g_config + sizeof(g_config); p++)
        *p = 0;
    g_config.source = TRIG_SOURCE_GPIO;
    g_config.trigger_edge = 0;

    g_uart_word_len = 0;
    g_uart_match_idx = 0;
}

/* ---- Configure trigger source -------------------------------------- */

void trigger_configure(const trigger_config_t *config)
{
    g_config = *config;

    /* If UART word trigger, configure the word buffer */
    if (config->source == TRIG_SOURCE_UART_WORD) {
        trigger_set_uart_word(config->uart_word, config->uart_word_len);
    }

    /* If power envelope trigger, set the comparator threshold.
     * The TLV3501 comparator output goes to the FPGA; the threshold
     * is set by a DAC (not shown in detail here). */
    if (config->source == TRIG_SOURCE_POWER_ENV) {
        trigger_set_power_threshold(config->power_env_threshold_mv);
    }
}

/* ---- Arm / Disarm -------------------------------------------------- */

void trigger_arm(void)
{
    /* Arm via the FPGA */
    fpga_arm_trigger(g_config.source, 0);
}

void trigger_disarm(void)
{
    fpga_disarm();
}

bool trigger_fired(void)
{
    return fpga_glitch_fired();
}

/* ---- UART trigger word matching ------------------------------------ */

void trigger_set_uart_word(const uint8_t *word, uint8_t len)
{
    if (len > 16)
        len = 16;
    for (uint8_t i = 0; i < len; i++)
        g_uart_word_buf[i] = word[i];
    g_uart_word_len = len;
    g_uart_match_idx = 0;
}

bool trigger_check_uart_word(uint8_t byte)
{
    /*
     * Called from the UART monitor ISR for each received byte.
     * If the byte matches the next expected byte in the trigger word,
     * advance the match index. If all bytes match, assert the UART
     * trigger GPIO to the FPGA and return true.
     */
    if (g_uart_word_len == 0)
        return false;

    if (byte == g_uart_word_buf[g_uart_match_idx]) {
        g_uart_match_idx++;
        if (g_uart_match_idx >= g_uart_word_len) {
            /* Full match — pulse the trigger GPIO to the FPGA */
            GPIOC->BSRR = BIT(TRIG_UART_WORD_PIN);
            /* Tiny pulse (~10 ns) */
            for (volatile int i = 0; i < 2; i++)
                ;
            GPIOC->BRR = BIT(TRIG_UART_WORD_PIN);
            g_uart_match_idx = 0;
            return true;
        }
    } else {
        /* Mismatch — reset match index */
        g_uart_match_idx = 0;
        /* But check if this byte matches the first character */
        if (byte == g_uart_word_buf[0]) {
            g_uart_match_idx = 1;
        }
    }
    return false;
}

/* ---- Power envelope threshold -------------------------------------- */

void trigger_set_power_threshold(uint16_t threshold_mv)
{
    /*
     * Set the TLV3501 comparator threshold via a DAC.
     * The threshold is in mV; the DAC converts this to an analog
     * voltage that the comparator compares against the target's
     * current-sense signal.
     *
     * In a full implementation, this would use a second DAC channel
     * or the same DAC5311 with a mux. Here we store the threshold
     * for reference.
     */
    (void)threshold_mv;
    /* DAC write would go here */
}

/* ---- End of file --------------------------------------------------- */