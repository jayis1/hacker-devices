/*
 * drivers/joystick.c — 5-way analog joystick driver for ZIGBEE-PHANTOM
 * Author: jayis1
 * License: GPL-2.0
 *
 * The joystick produces an analog voltage that varies by direction; a
 * resistive divider encodes up/down/left/right as distinct ADC bands, and a
 * separate digital button senses center press. We sample the ADC, band-classify
 * the value, and debounce.
 */
#include "joystick.h"
#include "../board.h"
#include "../registers.h"

/* ADC thresholds for 12-bit readings (0..4095). Tune per joystick model. */
#define JOY_THRESH_UP    900
#define JOY_THRESH_DOWN  3000
#define JOY_THRESH_LEFT  2000
#define JOY_THRESH_RIGHT 1200
#define JOY_DEAD_LOW     1700
#define JOY_DEAD_HIGH    2300

static joy_dir_t last_dir = JOY_NONE;
static uint32_t debounce_until = 0;

static uint16_t adc_sample(void)
{
    volatile uint32_t *ctl  = (volatile uint32_t *)(AUXADC_BASE + AUXADC_O_CTL);
    volatile uint32_t *data = (volatile uint32_t *)(AUXADC_BASE + AUXADC_O_DATA);
    *ctl |= AUXADC_CTL_START;
    for (volatile int i=0;i<100;i++);   /* conversion time */
    return (uint16_t)(*data & 0x0FFF);
}

void joystick_init(void)
{
    GPIO_INPUT(JOY_BTN_DIO);
    volatile uint32_t *ctl = (volatile uint32_t *)(AUXADC_BASE + AUXADC_O_CTL);
    *ctl = AUXADC_CTL_EN;
    last_dir = JOY_NONE;
    debounce_until = 0;
}

joy_dir_t joystick_read(void)
{
    uint32_t now = 0; /* rf_get_timestamp_us() would be used in production */
    if (now < debounce_until) return JOY_NONE;

    /* Center press (digital) takes priority */
    if (GPIO_RD(JOY_BTN_DIO) == 0) {
        last_dir = JOY_CENTER;
        debounce_until = now + 200000U;   /* 200 ms debounce */
        return JOY_CENTER;
    }

    uint16_t v = adc_sample();
    joy_dir_t d = JOY_NONE;
    if (v < JOY_THRESH_UP)         d = JOY_UP;
    else if (v > JOY_THRESH_DOWN) d = JOY_DOWN;
    else if (v > JOY_THRESH_LEFT && v <= JOY_DEAD_LOW)  d = JOY_LEFT;
    else if (v > JOY_THRESH_RIGHT && v < JOY_THRESH_LEFT) d = JOY_RIGHT;

    if (d != JOY_NONE) {
        last_dir = d;
        debounce_until = now + 200000U;
        return d;
    }
    last_dir = JOY_NONE;
    return JOY_NONE;
}

joy_dir_t joystick_wait(uint32_t timeout_ms)
{
    uint32_t start = 0;
    while ((0 - start) < (timeout_ms * 1000U)) {
        joy_dir_t d = joystick_read();
        if (d != JOY_NONE) return d;
    }
    return JOY_NONE;
}

void joystick_clear(void)
{
    last_dir = JOY_NONE;
    debounce_until = 0;
}