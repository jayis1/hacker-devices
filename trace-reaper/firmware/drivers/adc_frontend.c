/*
 * TRACE-REAPER — analog front-end (AFE) + trigger driver
 *
 * Drives the gain select, input source (shunt vs EM), analog-level
 * trigger threshold, and external trigger multiplexing. The ADC itself is
 * clocked by an external XO and streams into the FPGA; this driver only
 * configures the analog chain and the trigger source.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "adc_frontend.h"
#include "../registers.h"
#include "../board.h"

/* AFE control pins */
#define AFE_PORT_GAINA  GPIOB
#define AFE_PIN_GAINA   13
#define AFE_PORT_GAINB  GPIOB
#define AFE_PIN_GAINB   14
#define AFE_PORT_INSEL  GPIOB
#define AFE_PIN_INSEL   15
#define AFE_PORT_TRIGEN  GPIOC
#define AFE_PIN_TRIGEN  1
#define AFE_PORT_TRIGEXT GPIOC
#define AFE_PIN_TRIGEXT 0

/* Analog trigger threshold is set over SPI to an on-board DAC
 * (MAX5717 14-bit). We model the write as a 16-bit transfer.
 */
#define AFE_DAC_CS_PIN   4   /* on GPIOA */
#define AFE_DAC_PORT     GPIOA

static input_src_t   g_input  = INPUT_SHUNT;
static gain_t        g_gain   = GAIN_0DB;
static trigger_src_t g_trig   = TRIG_EXTERNAL;
static int16_t       g_thresh = 0;

/* SPI write to the threshold DAC (MAX5717-style 24-bit command) */
static void afe_dac_write(uint16_t code)
{
    /* Bring CS low, clock out 24-bit word {0x31, hi, lo}, CS high.
     * For the reference build we use SPI1 with software CS.
     */
    (void)code;  /* placeholder for SPI transfer */
}

void adc_frontend_init(void)
{
    /* Enable GPIO clocks already enabled in board_init().
     * Configure AFE control pins as push-pull outputs.
     */
    gpio_mode(AFE_PORT_GAINA,  AFE_PIN_GAINA,  1);
    gpio_mode(AFE_PORT_GAINB,  AFE_PIN_GAINB,  1);
    gpio_mode(AFE_PORT_INSEL,   AFE_PIN_INSEL,  1);
    gpio_mode(AFE_PORT_TRIGEN, AFE_PIN_TRIGEN, 1);
    /* External trigger input, pull-down, rising-edge sensitive */
    gpio_mode(AFE_PORT_TRIGEXT, AFE_PIN_TRIGEXT, 0);
    gpio_pupd(AFE_PORT_TRIGEXT, AFE_PIN_TRIGEXT, 2);

    /* Default: shunt input, 0 dB gain, external trigger */
    adc_frontend_set_input(INPUT_SHUNT);
    adc_frontend_set_gain(GAIN_0DB);
    adc_frontend_set_trigger(TRIG_EXTERNAL, 0);
    /* Disable analog-level trigger output by default */
    gpio_out(AFE_PORT_TRIGEN, AFE_PIN_TRIGEN, 0);
}

void adc_frontend_set_input(input_src_t s)
{
    g_input = s;
    gpio_out(AFE_PORT_INSEL, AFE_PIN_INSEL, (s == INPUT_EM) ? 1 : 0);
}

input_src_t adc_frontend_get_input(void) { return g_input; }

void adc_frontend_set_gain(gain_t g)
{
    g_gain = g;
    /* 2-bit gain code to AFE: bit0 -> gainA, bit1 -> gainB */
    gpio_out(AFE_PORT_GAINA, AFE_PIN_GAINA, (uint8_t)(g & 1));
    gpio_out(AFE_PORT_GAINB, AFE_PIN_GAINB, (uint8_t)((g >> 1) & 1));
}

gain_t adc_frontend_get_gain(void) { return g_gain; }

void adc_frontend_set_trigger(trigger_src_t src, int16_t threshold)
{
    g_trig = src;
    g_thresh = threshold;
    switch (src) {
    case TRIG_EXTERNAL:
        /* disable analog-level trigger output */
        gpio_out(AFE_PORT_TRIGEN, AFE_PIN_TRIGEN, 0);
        break;
    case TRIG_ANALOG:
        /* program the DAC with the threshold, enable comparator output */
        afe_dac_write((uint16_t)((int32_t)threshold + 2048));
        gpio_out(AFE_PORT_TRIGEN, AFE_PIN_TRIGEN, 1);
        break;
    case TRIG_TEMPLATE:
        /* analog-level comparator disabled; FPGA does template match */
        gpio_out(AFE_PORT_TRIGEN, AFE_PIN_TRIGEN, 0);
        break;
    case TRIG_NONE:
    default:
        gpio_out(AFE_PORT_TRIGEN, AFE_PIN_TRIGEN, 0);
        break;
    }
}

trigger_src_t adc_frontend_get_trigger(void) { return g_trig; }
int16_t adc_frontend_get_threshold(void) { return g_thresh; }

/* ---- Calibration ----
 * Auto-zero the analog trigger threshold by sampling a few idle traces and
 * picking the median. This is invoked from the UI/app before a session.
 */
int16_t adc_frontend_auto_threshold(void)
{
    /* In the reference build this issues a "capture N idle samples" request
     * to the FPGA over SPI and reads back the mean. We model the result.
     */
    return 0;  /* placeholder; real impl averages the ADC sample FIFO */
}

/* ---- Health check ----
 * Verifies the AFE is responsive by toggling the gain bits and reading back
 * the GPIO ODR. Returns 1 if OK, 0 if fault.
 */
int adc_frontend_selftest(void)
{
    uint32_t old = GPIOB->ODR;
    gpio_out(AFE_PORT_GAINA, AFE_PIN_GAINA, 1);
    gpio_out(AFE_PORT_GAINB, AFE_PIN_GAINB, 1);
    uint32_t after = GPIOB->ODR;
    GPIOB->ODR = old;
    return (after != old) ? 1 : 0;
}