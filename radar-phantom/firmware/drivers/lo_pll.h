/*
 * lo_pll.h — LO synthesizer declarations
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef RADARPHANTOM_LO_PLL_H
#define RADARPHANTOM_LO_PLL_H

#include <stdint.h>

void     lo_pll_init(void);
uint8_t  lo_pll_locked(void);
void     lo_pll_set_frequency(uint64_t f_lo_hz);
void     lo_pll_enable(uint8_t on);
void     lo_pll_set_to_rf_band(uint64_t rf_start_hz, uint64_t rf_stop_hz);

#endif /* RADARPHANTOM_LO_PLL_H */