/**
 * drivers/timing.h — cycle counters and delay helpers
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_TIMING_H
#define PHOTONSTRIKE_TIMING_H

#include <stdint.h>

void     timing_init(void);
void     ps_delay_us(uint32_t us);
void     ps_delay_ms(uint32_t ms);
uint32_t ps_millis(void);

#endif