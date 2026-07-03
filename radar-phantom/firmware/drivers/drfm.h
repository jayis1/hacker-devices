/*
 * drfm.h — DRFM FPGA control interface (declarations)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Declares the DRFM control functions implemented in drivers/drfm.c so
 * other modules (scenario.c, main.c) can call them.
 */
#ifndef RADARPHANTOM_DRFM_H
#define RADARPHANTOM_DRFM_H

#include <stdint.h>

typedef struct {
    uint32_t rel_delay;     /* in 5 ns units relative to base */
    int32_t  rel_doppler;   /* milli-Hz relative offset */
    uint8_t  gain;          /* 0..255 */
} drfm_tap_t;

typedef struct {
    uint32_t range_cm;
    int32_t  vel_mmps;
    int16_t  rcs_qdb;
    uint8_t  n_taps;
    uint8_t  noise_mode;
    uint8_t  microdoppler;
} phantom_desc_t;

void     drfm_reset(void);
uint8_t  drfm_is_ready(void);
uint8_t  drfm_selftest(void);
void     drfm_arm(uint8_t armed);
void     drfm_clear(void);
void     drfm_noise_mode(uint8_t on);
void     drfm_set_range_cm(uint32_t range_cm);
void     drfm_set_velocity_mmps(int32_t vel_mmps);
void     drfm_set_rcs_qdb(int16_t rcs_qdb);
void     drfm_set_taps(uint8_t n_taps, const drfm_tap_t *taps);
void     drfm_set_microdoppler(uint8_t enable, uint8_t depth);
void     drfm_configure(const phantom_desc_t *p);
uint8_t  drfm_status(void);
void     drfm_write_u8(uint8_t addr, uint8_t val);
uint8_t  drfm_read_u8(uint8_t addr);
void     drfm_write_u24(uint8_t addr, uint32_t val);
uint32_t drfm_read_u24(uint8_t addr);

#endif /* RADARPHANTOM_DRFM_H */