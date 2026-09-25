/*
 * SENT Sentinel board contract
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef SENT_SENTINEL_BOARD_H
#define SENT_SENTINEL_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SS_AUTHOR "jayis1"
#define SS_FIRMWARE_VERSION "0.1.0"
#define SS_CHANNEL_COUNT 4u
#define SS_MAX_NIBBLES 8u
#define SS_PROFILE_CAPACITY 32u
#define SS_EVENT_TEXT_LENGTH 112u
#define SS_CAPTURE_CLOCK_HZ 170000000u
#define SS_TIMER_TICKS_PER_US 170u
#define SS_SENT_TICK_MIN_US 2u
#define SS_SENT_TICK_MAX_US 90u
#define SS_SYNC_TICKS 56u
#define SS_NIBBLE_OFFSET_TICKS 12u
#define SS_FRAME_GAP_TICKS 100u
#define SS_MIN_LEARNING_FRAMES 16u
#define SS_DEFAULT_SCORE_THRESHOLD 45u
#define SS_DEFAULT_TIMING_TOLERANCE_PPM 35000u
#define SS_DEFAULT_ANALOG_TOLERANCE_MV 120u

/* STM32G474 pin assignment for the intended target board. */
#define SS_CH0_PIN 0u
#define SS_CH1_PIN 1u
#define SS_CH2_PIN 6u
#define SS_CH3_PIN 7u
#define SS_ADC0_PIN 0u
#define SS_ADC1_PIN 1u
#define SS_ADC2_PIN 2u
#define SS_ADC3_PIN 3u
#define SS_USB_DP_PIN 12u
#define SS_USB_DM_PIN 11u
#define SS_BUTTON_PIN 13u
#define SS_LED_RED_PIN 8u
#define SS_LED_GREEN_PIN 9u
#define SS_LED_BLUE_PIN 10u
#define SS_BUFFER_OE_SAFE_LEVEL 0u
#define SS_ANALOG_FULL_SCALE_MV 5000u
#define SS_ADC_COUNTS 4095u

typedef enum {
    SS_MODE_SAFE = 0,
    SS_MODE_LEARN,
    SS_MODE_OBSERVE,
    SS_MODE_REPLAY_LAB
} ss_mode_t;

typedef enum {
    SS_CHANNEL_0 = 0,
    SS_CHANNEL_1 = 1,
    SS_CHANNEL_2 = 2,
    SS_CHANNEL_3 = 3
} ss_channel_t;

typedef struct {
    uint32_t timing_tolerance_ppm;
    uint16_t analog_tolerance_mv;
    uint8_t score_threshold;
    uint8_t minimum_observations;
    bool retain_raw_nibbles;
} ss_config_t;

static inline bool ss_channel_valid(ss_channel_t channel)
{
    return (unsigned)channel < SS_CHANNEL_COUNT;
}

static inline ss_config_t ss_default_config(void)
{
    ss_config_t config;
    config.timing_tolerance_ppm = SS_DEFAULT_TIMING_TOLERANCE_PPM;
    config.analog_tolerance_mv = SS_DEFAULT_ANALOG_TOLERANCE_MV;
    config.score_threshold = SS_DEFAULT_SCORE_THRESHOLD;
    config.minimum_observations = SS_MIN_LEARNING_FRAMES;
    config.retain_raw_nibbles = false;
    return config;
}

#endif
