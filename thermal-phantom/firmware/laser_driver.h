/*
 * laser_driver.h - Laser pulse control and safety header
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef LASER_DRIVER_H
#define LASER_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

void laser_init(void);
bool laser_fire(float power_pct, uint32_t duration_ms);
void laser_disable(void);
bool laser_is_interlock_ok(void);
bool laser_open_shutter(void);
bool laser_close_shutter(void);
bool laser_is_shutter_open(void);
uint32_t laser_get_last_fire_ms(void);
float laser_get_power_pct(void);

#endif /* LASER_DRIVER_H */