/*
 * SDIO Sentinel board definition
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef SDIO_SENTINEL_BOARD_H
#define SDIO_SENTINEL_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SS_FIRMWARE_VERSION       "1.0.0"
#define SS_PROTOCOL_VERSION       1u
#define SS_BOARD_ID               0x53445331u
#define SS_CAPTURE_RING_SIZE      128u
#define SS_MAX_FRAME_BYTES        64u
#define SS_MAX_HOST_PAYLOAD       256u
#define SS_POLICY_RULES           32u
#define SS_AUDIT_CAPACITY         64u
#define SS_DEFAULT_CLOCK_LIMIT_HZ 25000000u
#define SS_MAX_CLOCK_HZ           50000000u
#define SS_INACTIVITY_TIMEOUT_MS  30000u
#define SS_ARM_HOLD_MS            1500u
#define SS_MAX_ACTIVE_SECONDS     900u

/* Logical MCU pin map. The production target is STM32U575 in LQFP-100. */
enum ss_pin {
    PIN_SD_HOST_CLK = 0,
    PIN_SD_HOST_CMD,
    PIN_SD_HOST_D0,
    PIN_SD_HOST_D1,
    PIN_SD_HOST_D2,
    PIN_SD_HOST_D3,
    PIN_SD_CARD_CLK,
    PIN_SD_CARD_CMD,
    PIN_SD_CARD_D0,
    PIN_SD_CARD_D1,
    PIN_SD_CARD_D2,
    PIN_SD_CARD_D3,
    PIN_FPGA_IRQ,
    PIN_FPGA_RESET_N,
    PIN_BYPASS_ENABLE,
    PIN_ARM_SWITCH,
    PIN_STATUS_RED,
    PIN_STATUS_GREEN,
    PIN_USB_DP,
    PIN_USB_DM,
    PIN_COUNT
};

enum ss_mode {
    SS_MODE_SAFE_BYPASS = 0,
    SS_MODE_PASSIVE_OBSERVE,
    SS_MODE_ENFORCE_POLICY,
    SS_MODE_LAB_EMULATION,
    SS_MODE_FAULT
};

enum ss_direction {
    SS_DIR_HOST_TO_CARD = 0,
    SS_DIR_CARD_TO_HOST = 1
};

enum ss_severity {
    SS_SEV_INFO = 0,
    SS_SEV_NOTICE,
    SS_SEV_WARNING,
    SS_SEV_CRITICAL
};

struct ss_platform {
    uint32_t (*millis)(void);
    bool (*read_pin)(enum ss_pin pin);
    void (*write_pin)(enum ss_pin pin, bool high);
    bool (*usb_read)(uint8_t *byte);
    bool (*usb_write)(const uint8_t *data, size_t length);
    void (*watchdog_kick)(void);
    void (*secure_zero)(void *data, size_t length);
};

#endif
