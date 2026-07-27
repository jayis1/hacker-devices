/*
 * imux_tamper.h — IMU-based tamper detection and zeroization
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_IMUX_TAMPER_H
#define CHRONOS_PHANTOM_IMUX_TAMPER_H

#include <stdint.h>

#define TAMPER_WINDOW_SAMPLES 8
#define TAMPER_DEFAULT_MG    1500   /* 1.5 g default threshold */

typedef enum {
    TAMPER_STATE_NORMAL = 0,
    TAMPER_STATE_ALERT  = 1,
    TAMPER_STATE_ZEROIZED = 2
} tamper_state_t;

typedef struct {
    tamper_state_t state;
    uint16_t threshold_mg;
    int16_t  accel_window[TAMPER_WINDOW_SAMPLES][3];  /* x,y,z mg */
    uint8_t  window_idx;
    uint32_t alert_count;
} tamper_t;

void tamper_init(tamper_t *t);
void tamper_set_threshold(tamper_t *t, uint16_t mg);
void tamper_sample(tamper_t *t, int16_t x_mg, int16_t y_mg, int16_t z_mg);
int  tamper_check(tamper_t *t);    /* returns 1 if tamper detected */
void tamper_zeroize(tamper_t *t);

#endif /* CHRONOS_PHANTOM_IMUX_TAMPER_H */