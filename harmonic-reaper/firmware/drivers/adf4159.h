/*
 * adf4159.h — TX synthesizer driver (Analog Devices ADF4159, fractional-N)
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_ADF4159_H
#define HARMONIC_REAPER_ADF4159_H

#include <stdint.h>

/* Initialize the ADF4159 for 2.4 GHz CW output. */
void adf4159_init(void);

/* Program a new fundamental frequency in Hz (range 2.0–2.6 GHz). */
void adf4159_set_freq(uint32_t freq_hz);

/* Enable / disable the synthesizer output (CE pin + register bit). */
void adf4159_enable(uint8_t en);

/* Configure pulse mode: PRF and pulse width (ns). Pass PRF=0 for CW. */
void adf4159_set_pulse(uint32_t prf_hz, uint16_t pulse_width_ns);

/* Enable quiet mode: randomizes PRF to avoid stable spectral signature. */
void adf4159_set_quiet(uint8_t en);

#endif /* HARMONIC_REAPER_ADF4159_H */
/* EOF — adf4159.h — jayis1 */