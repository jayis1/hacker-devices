/*
 * safety.h - Safety monitoring and protection header
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef SAFETY_H
#define SAFETY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SAFETY_OK = 0,
    SAFETY_OVERTEMP,
    SAFETY_OVERCURRENT,
    SAFETY_BATTERY_LOW,
    SAFETY_BATTERY_CRITICAL,
    SAFETY_ESTOP_PRESSED,
    SAFETY_LASER_INTERLOCK,
    SAFETY_WATCHDOG,
    SAFETY_FAULT
} safety_status_t;

void safety_init(void);
void safety_tick(void);  /* Called every 10ms */
safety_status_t safety_get_status(void);
const char *safety_get_status_str(void);
bool safety_is_ok(void);
void safety_emergency_stop(void);
uint16_t safety_read_battery_mv(void);
uint16_t safety_get_battery_pct(void);
float safety_get_mcu_temp(void);
void safety_feed_watchdog(void);

#endif /* SAFETY_H */