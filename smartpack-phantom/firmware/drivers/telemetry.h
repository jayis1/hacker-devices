/*
 * SmartPack Phantom telemetry interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef SMARTPACK_TELEMETRY_H
#define SMARTPACK_TELEMETRY_H

#include <stdint.h>
#include "../board.h"

void telemetry_init(sp_telemetry_t *telemetry);
void telemetry_tick(sp_runtime_t *runtime, uint32_t now_ms);
void telemetry_apply_profile(sp_runtime_t *runtime);
const char *telemetry_health_string(const sp_runtime_t *runtime);

#endif
