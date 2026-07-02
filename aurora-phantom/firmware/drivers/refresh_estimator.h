/*
 * refresh_estimator.h — refresh-rate / backlight-PWM estimator
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_REFRESH_ESTIMATOR_H
#define AURORA_REFRESH_ESTIMATOR_H
#include <stdint.h>

int      refresh_estimator_init(void);
void     refresh_estimator_set_fft_log(uint8_t log2);   /* 8..12 */
uint32_t refresh_estimator_run(uint32_t duration_ms);    /* returns peak Hz */
uint32_t refresh_estimator_get_freq(void);               /* last peak Hz */
uint16_t refresh_estimator_get_peak_mag(void);
bool     refresh_estimator_is_locked(void);

#endif