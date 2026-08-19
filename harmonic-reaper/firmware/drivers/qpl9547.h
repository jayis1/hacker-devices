/*
 * qpl9547.h — PA + AGC driver (Qorvo QPL9547)
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_QPL9547_H
#define HARMONIC_REAPER_QPL9547_H

#include <stdint.h>

void qpl9547_init(void);

/* Set TX power in dBm (−10..+15). Drives both SPI attenuator and AGC DAC. */
void qpl9547_set_power(int8_t dbm);

/* Increment/decrement TX power by 1 dB (AGC loop calls this). */
void qpl9547_step_power(int8_t delta_db);

/* Enable / disable the PA (TX_EN pin). */
void qpl9547_tx_enable(uint8_t en);

/* Read back currently commanded power. */
int8_t qpl9547_get_power(void);

#endif /* HARMONIC_REAPER_QPL9547_H */
/* EOF — qpl9547.h — jayis1 */