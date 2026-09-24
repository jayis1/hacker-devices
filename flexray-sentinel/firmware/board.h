/*
 * FlexRay Sentinel board configuration
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef FLEXRAY_SENTINEL_BOARD_H
#define FLEXRAY_SENTINEL_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FS_BOARD_NAME               "FlexRay Sentinel A0"
#define FS_FIRMWARE_VERSION         "0.1.0"
#define FS_CPU_HZ                   400000000u
#define FS_CAPTURE_CLOCK_HZ         80000000u
#define FS_USB_VENDOR_ID            0x1209u
#define FS_USB_PRODUCT_ID           0xF17Au
#define FS_MAX_PAYLOAD_BYTES        254u
#define FS_MAX_SLOTS                2048u
#define FS_MAX_CYCLE                63u
#define FS_EVENT_QUEUE_DEPTH        128u
#define FS_CAPTURE_QUEUE_DEPTH      256u
#define FS_BASELINE_WINDOW_CYCLES   128u
#define FS_TICK_NS                  12u

#define FS_PIN_LED_RED              0u
#define FS_PIN_LED_GREEN            1u
#define FS_PIN_LED_BLUE             2u
#define FS_PIN_ARM_SWITCH           3u
#define FS_PIN_TRANSCEIVER_A_EN     4u
#define FS_PIN_TRANSCEIVER_B_EN     5u
#define FS_PIN_RELAY_BYPASS         6u
#define FS_PIN_FPGA_IRQ             7u
#define FS_PIN_SD_DETECT            8u

#define FS_SPI_FPGA                 1u
#define FS_SPI_STORAGE              2u
#define FS_I2C_SECURE_ELEMENT       1u
#define FS_UART_SERVICE             3u

#define FS_TRANSCEIVER_STANDBY_US   25u
#define FS_WATCHDOG_PERIOD_MS       250u
#define FS_EVIDENCE_FLUSH_MS        1000u
#define FS_USB_FRAME_MAGIC          0x46525331u

typedef enum {
    FS_CHANNEL_A = 0,
    FS_CHANNEL_B = 1,
    FS_CHANNEL_BOTH = 2
} fs_channel_t;

typedef enum {
    FS_MODE_SAFE = 0,
    FS_MODE_OBSERVE,
    FS_MODE_LEARN,
    FS_MODE_REPLAY_LAB
} fs_mode_t;

typedef enum {
    FS_LED_OFF = 0,
    FS_LED_GREEN,
    FS_LED_AMBER,
    FS_LED_RED,
    FS_LED_BLUE
} fs_led_t;

typedef struct {
    fs_mode_t mode;
    fs_channel_t channels;
    uint16_t anomaly_threshold;
    uint16_t timing_tolerance_ticks;
    uint8_t minimum_observations;
    bool store_payloads;
    bool privacy_hash_payloads;
    bool require_physical_arm;
} fs_config_t;

static inline fs_config_t fs_default_config(void)
{
    fs_config_t config;
    config.mode = FS_MODE_LEARN;
    config.channels = FS_CHANNEL_BOTH;
    config.anomaly_threshold = 60u;
    config.timing_tolerance_ticks = 80u;
    config.minimum_observations = 8u;
    config.store_payloads = false;
    config.privacy_hash_payloads = true;
    config.require_physical_arm = true;
    return config;
}

#endif
