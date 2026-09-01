/*
 * BootROM Banshee telemetry model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "telemetry.h"

static uint32_t g_current_limit_ma;

void telemetry_init(void) {
    g_current_limit_ma = BB_LIMIT_MAX_CURRENT_MA;
}

void telemetry_set_current_limit(uint32_t max_ma) {
    g_current_limit_ma = max_ma;
}

bb_telemetry_sample_t telemetry_sample(uint32_t now_ms, bool active_mode) {
    bb_telemetry_sample_t sample;
    sample.target_vccio_mv = 1800u + ((now_ms / 100u) % 5u) * 10u;
    sample.flash_vccio_mv = 1800u + ((now_ms / 200u) % 4u) * 8u;
    sample.rail_current_ma = active_mode ? (340u + (now_ms % 90u)) : (190u + (now_ms % 45u));
    sample.board_temp_c = active_mode ? (46 + (int32_t)((now_ms / 300u) % 10u)) : (34 + (int32_t)((now_ms / 400u) % 8u));
    sample.watchdog_alert = ((now_ms % 1700u) == 0u && active_mode);
    sample.trip = false;

    if (sample.rail_current_ma > g_current_limit_ma) {
        sample.trip = true;
    }
    if ((uint32_t)sample.board_temp_c > BB_LIMIT_MAX_TEMP_C) {
        sample.trip = true;
    }
    if (sample.target_vccio_mv < BB_LIMIT_MIN_VCCIO_MV || sample.target_vccio_mv > BB_LIMIT_MAX_VCCIO_MV) {
        sample.trip = true;
    }
    if (sample.flash_vccio_mv < BB_LIMIT_MIN_VCCIO_MV || sample.flash_vccio_mv > BB_LIMIT_MAX_VCCIO_MV) {
        sample.trip = true;
    }
    return sample;
}
