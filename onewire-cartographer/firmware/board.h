/*
 * OneWire Cartographer board definition
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef OWC_BOARD_H
#define OWC_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OWC_FW_VERSION             "1.0.0"
#define OWC_CAPTURE_DEPTH          1024u
#define OWC_EVENT_QUEUE_DEPTH      64u
#define OWC_ROM_BYTES              8u
#define OWC_MAX_DEVICES            32u
#define OWC_SAMPLE_HZ              8000000u
#define OWC_USB_FRAME_MAX          256u
#define OWC_INJECTION_BUDGET       64u

/* STM32G474 pin assignment on the four-layer reference board. */
#define PIN_UPSTREAM_SENSE         0u   /* PA0 / ADC1_IN1 / TIM2_CH1 */
#define PIN_DOWNSTREAM_SENSE       1u   /* PA1 / ADC1_IN2 / TIM2_CH2 */
#define PIN_UPSTREAM_PULL          4u   /* PB4, open-drain MOSFET gate */
#define PIN_DOWNSTREAM_PULL        5u   /* PB5, open-drain MOSFET gate */
#define PIN_BRIDGE_ENABLE          6u   /* PB6, normally closed bypass */
#define PIN_STRONG_PULLUP          7u   /* PB7, current-limited 5 V pull-up */
#define PIN_ARM_SWITCH             8u   /* PC8, hardware consent switch */
#define PIN_STATUS_LED             9u   /* PC9 */
#define PIN_FAULT_LED              10u  /* PC10 */
#define PIN_TRIGGER_IN             11u  /* PC11 */
#define PIN_TRIGGER_OUT            12u  /* PC12 */

#define OWC_ADC_LOW_MV             800u
#define OWC_ADC_HIGH_MV            2200u
#define OWC_SHORT_MV               250u
#define OWC_OVER_VOLT_MV           6000u
#define OWC_SLOT_MIN_US            45u
#define OWC_SLOT_MAX_US            135u
#define OWC_RESET_MIN_US           350u
#define OWC_RESET_MAX_US           1000u

/* Platform services. Hardware implementations replace weak definitions. */
uint32_t board_micros(void);
uint32_t board_millis(void);
uint16_t board_adc_mv(unsigned channel);
bool board_gpio_read(unsigned pin);
void board_gpio_write(unsigned pin, bool value);
void board_delay_us(uint32_t delay);
void board_usb_write(const uint8_t *data, size_t length);
void board_log(const char *message);
void board_watchdog_kick(void);
void board_init(void);

#endif /* OWC_BOARD_H */
