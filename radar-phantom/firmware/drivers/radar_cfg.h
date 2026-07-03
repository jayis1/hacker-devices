/*
 * radar_cfg.h — victim-band sniff declarations
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef RADARPHANTOM_RADAR_CFG_H
#define RADARPHANTOM_RADAR_CFG_H

#include <stdint.h>

uint8_t  radar_sniff(uint8_t timeout_s);
uint8_t  radar_band_valid(void);
uint64_t radar_band_start_hz(void);
uint64_t radar_band_stop_hz(void);
uint32_t radar_band_chirp_us(void);
uint32_t radar_band_pri_us(void);
uint64_t radar_band_bw_hz(void);
uint64_t radar_band_center_hz(void);

#endif /* RADARPHANTOM_RADAR_CFG_H */