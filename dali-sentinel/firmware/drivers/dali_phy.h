/* DALI physical/link layer API. Author: jayis1. */
#ifndef DALI_PHY_H
#define DALI_PHY_H
#include "../board.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t tick; bus_side_t side; bool level_high; uint16_t bus_mv; } dali_edge_t;
typedef struct {
    uint32_t start_tick;
    uint32_t duration_ticks;
    uint32_t raw;
    uint16_t minimum_mv;
    uint8_t bit_count;
    uint8_t quality;
    uint8_t collision_score;
    bus_side_t side;
    bool framing_ok;
} dali_frame_t;
typedef enum { PHY_OK=0, PHY_EMPTY, PHY_BUSY, PHY_INVALID, PHY_DENIED, PHY_OVERFLOW } phy_result_t;
typedef struct {
    operating_mode_t mode;
    uint32_t active_deadline_ms;
    uint32_t arm_epoch;
    uint32_t edge_overflows;
    uint32_t frame_overflows;
    bool sink_enabled[2];
    bool bypass_commanded_closed;
    bool armed_latched;
} dali_safety_status_t;
void dali_phy_init(void);
void dali_phy_capture_edge(bus_side_t side, bool high, uint32_t tick, uint16_t bus_mv);
void dali_phy_poll(uint32_t now_tick);
phy_result_t dali_phy_next_frame(dali_frame_t *frame);
phy_result_t dali_phy_request_mode(operating_mode_t mode, const board_inputs_t *inputs);
phy_result_t dali_phy_schedule(bus_side_t side, uint32_t raw, uint8_t bits, uint8_t repeats, uint32_t expiry_ms, bool high_impact);
void dali_phy_service_tx(uint32_t now_tick, uint32_t now_ms);
void dali_phy_emergency_stop(const char *reason);
dali_safety_status_t dali_phy_status(void);
bool dali_phy_run_self_test(void);
#endif
