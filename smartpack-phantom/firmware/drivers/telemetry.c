/*
 * SmartPack Phantom telemetry model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "telemetry.h"

void telemetry_init(sp_telemetry_t *telemetry) {
    if (telemetry == 0) {
        return;
    }
    telemetry->host_voltage_mv = 0u;
    telemetry->pack_voltage_mv = 0u;
    telemetry->pack_current_ma = 0;
    telemetry->board_temp_c = 27;
    telemetry->target_current_ma = 0u;
    telemetry->current_limit_tripped = false;
    telemetry->emergency_cutoff = false;
}

void telemetry_apply_profile(sp_runtime_t *runtime) {
    if (runtime == 0) {
        return;
    }
    runtime->telemetry.pack_voltage_mv = runtime->profile.design_voltage_mv;
    runtime->telemetry.host_voltage_mv = runtime->profile.design_voltage_mv - 45u;
    runtime->telemetry.pack_current_ma = runtime->profile.current_ma;
    runtime->telemetry.target_current_ma = (uint16_t)(runtime->profile.current_ma < 0 ? -runtime->profile.current_ma : runtime->profile.current_ma);
    runtime->telemetry.board_temp_c = runtime->profile.temperature_c + 3;
    runtime->telemetry.current_limit_tripped = runtime->profile.charge_limit_ma < 900u;
}

void telemetry_tick(sp_runtime_t *runtime, uint32_t now_ms) {
    uint32_t phase;
    int16_t current_wave;
    if (runtime == 0) {
        return;
    }

    phase = (now_ms / 200u) % 8u;
    current_wave = (int16_t)((phase < 4u) ? (phase * 15) : ((8u - phase) * 15));
    runtime->telemetry.host_voltage_mv = (uint16_t)(runtime->profile.design_voltage_mv - 40u - phase);
    runtime->telemetry.pack_voltage_mv = (uint16_t)(runtime->profile.design_voltage_mv - (runtime->profile.remaining_capacity_mah < 1000u ? 180u : 65u));
    runtime->telemetry.pack_current_ma = (int16_t)(runtime->profile.current_ma - current_wave);
    runtime->telemetry.target_current_ma = (uint16_t)(runtime->telemetry.pack_current_ma < 0 ? -runtime->telemetry.pack_current_ma : runtime->telemetry.pack_current_ma);
    runtime->telemetry.board_temp_c = (int16_t)(runtime->profile.temperature_c + 2 + (int16_t)(phase));
    runtime->telemetry.current_limit_tripped = runtime->profile.charge_limit_ma < 750u;
    runtime->telemetry.emergency_cutoff = runtime->profile.temperature_c > 72 || runtime->telemetry.target_current_ma > 6000u;

    if (runtime->telemetry.emergency_cutoff) {
        runtime->flags |= SP_FLAG_CURRENT_LIMIT;
    }
    if (runtime->profile.temperature_c > 55) {
        runtime->flags |= SP_FLAG_THERMAL_WARNING;
    } else {
        runtime->flags &= ~SP_FLAG_THERMAL_WARNING;
    }
}

const char *telemetry_health_string(const sp_runtime_t *runtime) {
    if (runtime == 0) {
        return "unknown";
    }
    if (runtime->telemetry.emergency_cutoff) {
        return "cutoff";
    }
    if (runtime->telemetry.current_limit_tripped) {
        return "limited";
    }
    if (runtime->profile.temperature_c > 55) {
        return "thermal-warning";
    }
    if (runtime->profile.remaining_capacity_mah < 800u) {
        return "near-empty";
    }
    return "nominal";
}
