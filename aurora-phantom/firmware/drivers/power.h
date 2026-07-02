/*
 * power.h — nPM1300 PMIC + status LED control
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_POWER_H
#define AURORA_POWER_H
#include <stdint.h>

int  power_init(void);
void power_rail_set(uint32_t mask);
void power_rail_clear(uint32_t mask);
uint32_t power_rail_state(void);
void power_status_led(uint8_t r, uint8_t g, uint8_t b);
uint8_t power_battery_pct(void);

#endif