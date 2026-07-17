/*
 * power_path.h — VBUS / DAC / eFuse / HRTIM / INA226 driver for CC-Stiletto.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef CC_STILETTO_POWER_PATH_H
#define CC_STILETTO_POWER_PATH_H

#include <stdint.h>
#include <stdbool.h>

void    power_path_init(void);

/* Program the VBUS reference DAC (MAX5370 channel A) to a target millivolt.
 * The DAC drives the feedback node of the LM5180 flyback / pass regulator. */
int     power_path_set_vbus(uint16_t mv);

/* Set the eFuse current limit (mA). Range 1500..5000. */
int     power_path_set_ilimit(uint16_t ma);

/* Enable / disable the VBUS source path (MOSFET bank). */
void    power_path_source_enable(bool en);
void    power_path_sink_enable(bool en);

/* Read INA226 telemetry from both legs. */
void    power_path_read_src(uint16_t *mv, int16_t *ma);
void    power_path_read_snk(uint16_t *mv, int16_t *ma);

/* MOSFET bank temperature (°C). */
uint8_t power_path_read_temp(void);

/* HRTIM one-shot glitch pulse generator (sub-microsecond VBUS drop). */
void    power_path_glitch_pulse(uint32_t low_ns);

/* eFuse fault flag */
bool    power_path_efuse_fault(void);

#endif /* CC_STILETTO_POWER_PATH_H */