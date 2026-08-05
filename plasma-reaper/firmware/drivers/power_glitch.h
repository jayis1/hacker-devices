/*
 * power_glitch.h — Power glitch driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef POWER_GLITCH_H
#define POWER_GLITCH_H

#include <stdint.h>
#include "../board.h"

void power_glitch_init(void);
void power_glitch_configure(uint16_t depth_mv, uint8_t series_r_idx,
                            uint32_t width_ns);
void power_glitch_fire(void);
void power_glitch_disable(void);

/* Glitch quality classification from ADC waveform */
typedef enum {
    GLITCH_QUALITY_CLEAN = 0,
    GLITCH_QUALITY_OVERSHOOT,
    GLITCH_QUALITY_UNDERDEPTH,
    GLITCH_QUALITY_INVALID,
    GLITCH_QUALITY_NO_GLITCH
} glitch_quality_t;

#endif /* POWER_GLITCH_H */