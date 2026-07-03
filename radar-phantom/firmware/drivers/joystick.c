/*
 * drivers/joystick.c — 5-way joystick debounce + event queue
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The 5-way joystick uses a resistor ladder on PA0 (ADC) for UP/DOWN/
 * LEFT/RIGHT and a digital press on PC13. This module samples both,
 * debounces, and exposes a small event queue to the UI.
 */
#include "../board.h"
#include "../registers.h"

typedef enum {
    JOY_NONE = 0,
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT,
    JOY_PRESS,
    JOY_RELEASE
} joy_event_t;

#define JOY_QUEUE_SZ  16
static joy_event_t queue[JOY_QUEUE_SZ];
static uint8_t q_head, q_tail;

static uint16_t adc_read_joystick(void)
{
    ADC_CR(ADC1_BASE) |= ADC_CR_ADEN;
    while (!(ADC_ISR(ADC1_BASE) & ADC_ISR_ADRDY)) { }
    ADC_SQR1(ADC1_BASE) = JOY_ADC_CHANNEL;
    ADC_CR(ADC1_BASE) |= ADC_CR_ADSTART;
    while (!(ADC_ISR(ADC1_BASE) & ADC_ISR_EOC)) { }
    return (uint16_t)(ADC_DR1(ADC1_BASE) & 0xFFF);
}

static joy_event_t classify(uint16_t adc, uint8_t press)
{
    static uint8_t last_press = 1;
    static uint16_t last_adc = 0;
    static uint8_t stable = 0;
    /* debounce: require same reading 3 times */
    if (adc == last_adc) {
        if (stable < 3) stable++;
    } else {
        stable = 0;
        last_adc = adc;
        return JOY_NONE;
    }
    if (stable < 3) return JOY_NONE;

    /* press edge */
    if (press != last_press) {
        last_press = press;
        stable = 0;
        return press ? JOY_PRESS : JOY_RELEASE;
    }
    /* direction (only on release-to-press transitions handled above;
     * here emit repeats while held)
     */
    if (adc > JOY_TH_NONE_HI)     return JOY_NONE;
    else if (adc > JOY_TH_RIGHT_LO && adc < JOY_TH_RIGHT_HI) return JOY_RIGHT;
    else if (adc > JOY_TH_DOWN_LO  && adc < JOY_TH_DOWN_HI)  return JOY_DOWN;
    else if (adc > JOY_TH_LEFT_LO  && adc < JOY_TH_LEFT_HI)  return JOY_LEFT;
    else if (adc > JOY_TH_UP_LO    && adc < JOY_TH_UP_HI)    return JOY_UP;
    return JOY_NONE;
}

static void q_push(joy_event_t e)
{
    uint8_t next = (q_head + 1) % JOY_QUEUE_SZ;
    if (next == q_tail) return;       /* overflow */
    queue[q_head] = e;
    q_head = next;
}

joy_event_t joystick_get_event(void)
{
    if (q_tail == q_head) return JOY_NONE;
    joy_event_t e = queue[q_tail];
    q_tail = (q_tail + 1) % JOY_QUEUE_SZ;
    return e;
}

void joystick_task(void)
{
    uint16_t adc = adc_read_joystick();
    uint8_t press = (GPIO_IDR(JOY_PRESS_PORT) & GPIO_PIN(JOY_PRESS_PIN)) ? 0 : 1;
    joy_event_t e = classify(adc, press);
    if (e != JOY_NONE) q_push(e);
}

const char *joystick_event_name(joy_event_t e)
{
    switch (e) {
    case JOY_UP:      return "UP";
    case JOY_DOWN:    return "DOWN";
    case JOY_LEFT:    return "LEFT";
    case JOY_RIGHT:   return "RIGHT";
    case JOY_PRESS:   return "PRESS";
    case JOY_RELEASE: return "RELEASE";
    default:          return "NONE";
    }
}