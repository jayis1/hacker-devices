/*
 * power_mgmt.h — battery gauge + PMIC control
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_POWER_H
#define HARMONIC_REAPER_POWER_H

#include <stdint.h>

void power_init(void);
uint16_t power_read_vdd_mv(void);   /* battery voltage in millivolts */
uint8_t  power_mv_to_pct(uint16_t mv);  /* 0..100 charge percentage */
uint8_t  power_is_charging(void);       /* 1 if VBUS present + charging */
void     power_enable_rail(uint8_t rail_id, uint8_t en);  /* PMIC rail control */

#endif /* HARMONIC_REAPER_POWER_H */
/* EOF — power_mgmt.h — jayis1 */