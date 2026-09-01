/*
 * BootROM Banshee telemetry model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef BOOTROM_BANSHEE_TELEMETRY_H
#define BOOTROM_BANSHEE_TELEMETRY_H

#include "../board.h"

void telemetry_init(void);
void telemetry_set_current_limit(uint32_t max_ma);
bb_telemetry_sample_t telemetry_sample(uint32_t now_ms, bool active_mode);

#endif
