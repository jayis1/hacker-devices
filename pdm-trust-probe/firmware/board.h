/* PDM Trust Probe board contract
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef PTP_BOARD_H
#define PTP_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PTP_CHANNEL_COUNT 4u
#define PTP_EVENT_CAPACITY 128u
#define PTP_FRAME_MAX 256u
#define PTP_CLOCK_MIN_HZ 512000u
#define PTP_CLOCK_MAX_HZ 4800000u
#define PTP_ARM_LEASE_MS 60000u
#define PTP_MAX_INJECT_MS 250u
#define PTP_FIRMWARE_VERSION 0x00010000u
enum ptp_pin { PIN_ARM=0, PIN_BYPASS=1, PIN_RED_LED=2, PIN_GREEN_LED=3, PIN_RELAY=4, PIN_FPGA_IRQ=5 };
uint32_t board_millis(void);
uint64_t board_ticks(void);
bool board_gpio_read(unsigned pin);
void board_gpio_write(unsigned pin, bool value);
void board_usb_write(const uint8_t *data, size_t length);
bool board_run_fixed_pattern(uint8_t pattern,
                             uint8_t channel_mask,
                             uint32_t duration_ms);
void board_watchdog_kick(void);
void board_init(void);
#endif
