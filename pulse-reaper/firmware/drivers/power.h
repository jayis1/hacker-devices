/*
 * power.h — fuel gauge + charger management
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_POWER_H
#define PULSEREAPER_POWER_H

void power_init(void);
void power_poll(void);
int  power_is_low(void);
int  power_battery_pct(void);   /* 0..100 */

#endif