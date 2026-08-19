/*
 * ad9361.h — 2f0 receiver driver (Analog Devices AD9361 agile transceiver)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * We only use the AD9361 as a direct-conversion receiver at 4.8 GHz
 * (2 × 2.4 GHz fundamental). TX side is unused. I/Q samples stream over
 * the parallel CMOS data bus to the Spartan-7 FPGA, which does the FFT.
 */
#ifndef HARMONIC_REAPER_AD9361_H
#define HARMONIC_REAPER_AD9361_H

#include <stdint.h>

void ad9361_init(void);
void ad9361_enable(uint8_t en);
void ad9361_set_gain(uint8_t gain_db);   /* 0..73 dB, manual gain mode */
uint8_t ad9361_get_rssi_db(void);        /* approximate RSSI from 2f0 channel */

#endif /* HARMONIC_REAPER_AD9361_H */
/* EOF — ad9361.h — jayis1 */