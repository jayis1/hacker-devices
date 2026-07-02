/*
 * lockin.h — digital lock-in demodulator control
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_LOCKIN_H
#define AURORA_LOCKIN_H
#include <stdint.h>
#include <stdbool.h>

int  lockin_init(void);
void lockin_set_lo_freq(uint32_t hz);
uint32_t lockin_get_lo_freq(void);
void lockin_set_integration_window(uint32_t us);
void lockin_set_frame_period(uint32_t us);
void lockin_enable(bool on);
bool lockin_is_running(void);

#endif