/*
 * ad9226_if.h — 3f0 IF channel (ADL5802 mixer + AD9226 ADC)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The 3f0 (7.2 GHz) channel uses a heterodyne approach: the ADL5802
 * active mixer down-converts 7.2 GHz to a 200 MHz IF, which the AD9226
 * (12-bit, 80 MSPS) digitizes. The FPGA owns the sample clock and the
 * parallel data bus; the MCU only controls enable/power-down pins and
 * the mixer bias via I²C.
 */
#ifndef HARMONIC_REAPER_AD9226_IF_H
#define HARMONIC_REAPER_AD9226_IF_H

#include <stdint.h>

void ad9226_if_init(void);
void ad9226_power_down(uint8_t pd);
void adl5802_enable(uint8_t en);
void adl5802_set_bias(uint8_t bias_pct);   /* mixer bias, 0..100 */

#endif /* HARMONIC_REAPER_AD9226_IF_H */
/* EOF — ad9226_if.h — jayis1 */