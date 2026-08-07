/*
 * drivers/power.h — Power Management for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_POWER_H
#define PRISM_TAP_POWER_H

#include <stdint.h>

int  power_init(void);
void power_poll(void);
uint8_t  power_get_battery_pct(void);
uint16_t power_get_battery_mv(void);
uint8_t  power_is_charging(void);
uint8_t  power_is_usb_powered(void);
void power_set_low_power_mode(uint8_t enable);
void power_enter_standby(void);
void power_wakeup(void);

/* ADC channel for battery monitoring */
#define BATT_ADC_CHANNEL  5

#endif /* PRISM_TAP_POWER_H */
/* Author: jayis1 */