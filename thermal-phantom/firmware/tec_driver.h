/*
 * tec_driver.h - Peltier TEC H-bridge driver header
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef TEC_DRIVER_H
#define TEC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

void tec_init(void);
void tec_set_duty(float duty_pct);  /* -100% (max cool) to +100% (max heat) */
float tec_get_duty(void);
void tec_disable(void);
void tec_enable(void);
bool tec_is_fault(void);
uint16_t tec_read_current_ma(void);
void tec_soft_start(float target_duty, uint32_t ramp_ms);

#endif /* TEC_DRIVER_H */