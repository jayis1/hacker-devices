/*
 * em_pulse.h — EM pulse driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EM_PULSE_H
#define EM_PULSE_H

#include <stdint.h>
#include "../board.h"

void em_pulse_init(void);
void em_pulse_configure(uint16_t voltage_mv, uint16_t width_ns);
void em_pulse_fire(void);
void em_pulse_disable(void);

/* Thermal protection */
bool em_pulse_thermal_ok(void);
uint16_t em_pulse_get_temp_c(void);

/* Safety limits */
#define EM_PULSE_MAX_VOLTAGE_MV   60000  /* 60 V */
#define EM_PULSE_MAX_WIDTH_NS     500    /* 500 ns */
#define EM_PULSE_MAX_REPRATE_HZ   100    /* thermal limited */
#define EM_PULSE_THERMAL_LIMIT_C  70     /* shutdown threshold */

#endif /* EM_PULSE_H */