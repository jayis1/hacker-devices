/*
 * Type-C Specter VBUS telemetry
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef TYPEC_SPECTER_VBUS_METER_H
#define TYPEC_SPECTER_VBUS_METER_H

#include <stdint.h>
#include "../board.h"

void vbus_meter_init(void);
ts_power_sample_t vbus_meter_sample(void);
void vbus_meter_set_current_limit(uint32_t milliamps);
uint32_t vbus_meter_get_current_limit(void);

#endif
