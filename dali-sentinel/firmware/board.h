/* DALI Sentinel board contract
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef DALI_SENTINEL_BOARD_H
#define DALI_SENTINEL_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define BOARD_CORE_HZ 170000000u
#define CAPTURE_TIMER_HZ 2000000u
#define DALI_HALF_BIT_TICKS 833u
#define DALI_HALF_MIN_TICKS 650u
#define DALI_HALF_MAX_TICKS 1020u
#define DALI_FULL_MIN_TICKS 1450u
#define DALI_FULL_MAX_TICKS 1880u
#define EDGE_RING_CAPACITY 256u
#define FRAME_RING_CAPACITY 64u
#define FINDING_CAPACITY 64u
#define ACTIVE_SESSION_MAX_MS 600000u
#define TX_REPEAT_MAX 16u
#define BUS_LOW_MV 6500u
#define BUS_HIGH_MIN_MV 9000u
#define BUS_OVERVOLT_MV 24000u
#define SINK_CURRENT_MAX_MA 250u
#define BOARD_TEMP_MAX_C 75
#define STORAGE_BLOCK_BYTES 512u
#define DALI_SENTINEL_VERSION "0.1.0"
typedef enum { SIDE_CONTROLLER=0, SIDE_GEAR=1, SIDE_UNKNOWN=2 } bus_side_t;
typedef enum { MODE_OBSERVE=0, MODE_INLINE=1, MODE_LAB_ISOLATE=2, MODE_FAULT=3 } operating_mode_t;
typedef struct {
    uint32_t now_ms;
    uint16_t bus_mv[2];
    uint16_t sink_ma[2];
    int16_t temperature_c;
    bool arm_pressed;
    bool authorization_jumper;
    bool bypass_closed_feedback;
    bool isolated_power_good[2];
} board_inputs_t;
void board_init(void);
uint32_t board_millis(void);
uint32_t board_capture_ticks(void);
board_inputs_t board_inputs(void);
void board_set_sink(bus_side_t side, bool enabled);
void board_set_bypass(bool closed);
void board_set_status(uint8_t red, uint8_t green, uint8_t blue);
void board_watchdog_kick(void);
void board_log(const char *message);
#endif
