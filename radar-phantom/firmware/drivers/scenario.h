/*
 * scenario.h — scenario VM declarations
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef RADARPHANTOM_SCENARIO_H
#define RADARPHANTOM_SCENARIO_H

#include <stdint.h>

void     scenario_load_builtin(uint8_t idx);
void     scenario_load_custom(const uint8_t *code);
void     scenario_stop(void);
uint8_t  scenario_is_running(void);
uint8_t  scenario_is_armed(void);
void     scenario_tick(void);
void     scenario_override_range(uint32_t cm);
void     scenario_override_vel(int32_t mmps);
void     scenario_override_rcs(int16_t qdb);
uint32_t scenario_get_range_cm(void);
int32_t  scenario_get_vel_mmps(void);
int16_t  scenario_get_rcs_qdb(void);
uint8_t  scenario_get_taps(void);
uint8_t  scenario_get_armed(void);

#endif /* RADARPHANTOM_SCENARIO_H */