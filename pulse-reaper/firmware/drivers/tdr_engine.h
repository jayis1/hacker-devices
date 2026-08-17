/*
 * tdr_engine.h — TDR acquisition and cable classification
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_TDR_ENGINE_H
#define PULSEREAPER_TDR_ENGINE_H

#include <stdint.h>

/* Cable type classification result */
typedef enum {
    CABLE_UNKNOWN = 0,
    CABLE_CAT5E,
    CABLE_CAT6,
    CABLE_CAT6A,
    CABLE_CAT7,
    CABLE_PROFIBUS_DP,     /* purple, 150 ohm */
    CABLE_RS485_BELDEN9841,/* 120 ohm         */
    CABLE_MIL1553_TWINAX,  /* 78 ohm twinax    */
    CABLE_POTS_RISER,      /* 100 ohm telecom   */
    CABLE_POWER            /* mains — refuse    */
} cable_type_t;

typedef struct {
    cable_type_t type;
    uint32_t     impedance_mohm;   /* milliohms */
    uint32_t     length_mm;        /* millimetres */
    uint32_t     dist_to_near_mm;
    uint32_t     dist_to_far_mm;
    int          terminated_near;
    int          terminated_far;
    int          shielded;
    int          live_conductor;   /* hazardous voltage detected */
    char         label[32];
} tdr_result_t;

void tdr_engine_init(void);

/* Acquire a reflectogram and classify the cable. */
int  tdr_engine_acquire_and_classify(void);

/* Get the last classification result. */
const tdr_result_t *tdr_engine_get_result(void);

/* Show the result on the OLED. */
void tdr_engine_show_result(void);

/* Send the reflectogram over BLE for the app's TDR screen. */
void tdr_engine_send_reflectogram_over_ble(void);

/* Safety: is the cable safe for injection? (jaw closed + unpowered) */
int  tdr_engine_cable_is_safe_for_inject(void);

#endif