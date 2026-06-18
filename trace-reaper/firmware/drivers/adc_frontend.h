/*
 * TRACE-REAPER — analog front-end + trigger driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ADC_FRONTEND_H
#define ADC_FRONTEND_H

#include <stdint.h>
#include "../board.h"

void adc_frontend_init(void);
void adc_frontend_set_input(input_src_t s);
input_src_t adc_frontend_get_input(void);
void adc_frontend_set_gain(gain_t g);
gain_t adc_frontend_get_gain(void);
void adc_frontend_set_trigger(trigger_src_t src, int16_t threshold);
trigger_src_t adc_frontend_get_trigger(void);
int16_t adc_frontend_get_threshold(void);
int16_t adc_frontend_auto_threshold(void);
int adc_frontend_selftest(void);

#endif /* ADC_FRONTEND_H */