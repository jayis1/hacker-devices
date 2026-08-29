/*
 * Type-C Specter VBUS telemetry
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "vbus_meter.h"

static uint32_t g_limit_ma = 3000u;
static uint32_t g_sample_index = 0u;

void vbus_meter_init(void) {
    g_limit_ma = 3000u;
    g_sample_index = 0u;
}

ts_power_sample_t vbus_meter_sample(void) {
    ts_power_sample_t s;
    s.target_vbus_mv = 5000u + ((g_sample_index % 4u) * 40u);
    s.peer_vbus_mv = 9000u;
    s.target_ibus_ma = 1200u + ((g_sample_index * 137u) % 700u);
    s.peer_ibus_ma = 900u + ((g_sample_index * 97u) % 500u);
    s.board_temp_c = 39 + (int32_t)(g_sample_index % 11u);
    s.trip = false;

    if (s.target_ibus_ma > g_limit_ma) {
        s.trip = true;
    }
    if (s.target_vbus_mv > TS_LIMIT_VBUS_MV_MAX || s.target_vbus_mv < TS_LIMIT_VBUS_MV_MIN) {
        s.trip = true;
    }
    if ((uint32_t)s.board_temp_c > TS_LIMIT_TEMP_C_MAX) {
        s.trip = true;
    }

    g_sample_index++;
    return s;
}

void vbus_meter_set_current_limit(uint32_t milliamps) {
    if (milliamps < 500u) {
        g_limit_ma = 500u;
    } else if (milliamps > TS_LIMIT_IBUS_MA_MAX) {
        g_limit_ma = TS_LIMIT_IBUS_MA_MAX;
    } else {
        g_limit_ma = milliamps;
    }
}

uint32_t vbus_meter_get_current_limit(void) {
    return g_limit_ma;
}
