/* RFFE Sentinel board contract
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef RFFE_SENTINEL_BOARD_H
#define RFFE_SENTINEL_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define RS_BOARD_NAME             "RFFE Sentinel A0"
#define RS_CPU_HZ                 170000000u
#define RS_MAX_RFFE_HZ            52000000u
#define RS_USB_MAX_PAYLOAD        512u
#define RS_EVENT_CAPACITY         256u
#define RS_POLICY_CAPACITY        64u
#define RS_ARM_WINDOW_US          10000000ull
#define RS_MAX_LEASE_US           300000000ull
#define RS_HOST_TIMEOUT_US        2000000ull
#define RS_MAX_DELAY_NS           2000u
#define RS_VIO_MIN_MV             1050u
#define RS_VIO_MAX_MV             1950u
#define RS_OVERCURRENT_MA         40u

/* Logical pins. The embedded port maps these to STM32 GPIOs. */
enum rs_pin {
    RS_PIN_BYPASS_EN = 0,
    RS_PIN_FPGA_RESET,
    RS_PIN_FPGA_CS,
    RS_PIN_FLASH_CS,
    RS_PIN_ARM_BUTTON,
    RS_PIN_MODE_BUTTON,
    RS_PIN_LED_RED,
    RS_PIN_LED_GREEN,
    RS_PIN_LED_BLUE,
    RS_PIN_VIO_SELECT,
    RS_PIN_COUNT
};

enum rs_mode {
    RS_MODE_SAFE_BYPASS = 0,
    RS_MODE_PASSIVE_CAPTURE = 1,
    RS_MODE_ARMED_FORWARD = 2,
    RS_MODE_FAULT_LATCHED = 3
};

enum rs_fault {
    RS_FAULT_NONE = 0,
    RS_FAULT_VIO_RANGE = 1u << 0,
    RS_FAULT_OVERCURRENT = 1u << 1,
    RS_FAULT_FPGA_WATCHDOG = 1u << 2,
    RS_FAULT_FIFO_OVERFLOW = 1u << 3,
    RS_FAULT_HOST_TIMEOUT = 1u << 4,
    RS_FAULT_BAD_POLICY = 1u << 5,
    RS_FAULT_PROTOCOL = 1u << 6
};

struct rs_measurements {
    uint16_t vio_mv;
    uint16_t target_ma;
    uint16_t sdata_high_mv;
    uint16_t sclk_high_mv;
};

uint64_t board_time_us(void);
bool board_button_recent(uint64_t now_us);
void board_note_arm_press(uint64_t at_us);
void board_set_bypass(bool active_path);
void board_set_led(enum rs_mode mode);
void board_read_measurements(struct rs_measurements *out);
void board_watchdog_kick(void);
void board_sim_set_measurements(uint16_t vio_mv, uint16_t target_ma);
void board_sim_advance(uint64_t delta_us);

#endif
