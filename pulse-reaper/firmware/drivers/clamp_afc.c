/*
 * clamp_afc.c — clamp analog front-end control
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Thin wrapper around the board.h helpers that adds the safety state
 * machine: the inject driver cannot be enabled unless the jaw is
 * closed AND the TDR engine has reported a connected (non-live) cable.
 */

#include "clamp_afc.h"
#include "board.h"
#include "tdr_engine.h"

static int g_inject_armed = 0;
static int g_cable_verified_unpowered = 0;

void clamp_afc_init(void) {
    clamp_set_coupling(PR_COUPLING_BOTH);
    clamp_set_gain(PR_GAIN_LOW);
    clamp_inject_enable(0);
    g_inject_armed = 0;
    g_cable_verified_unpowered = 0;
}

void clamp_afc_set_coupling(clamp_coupling_t c) {
    /* Refuse to change coupling while inject is enabled */
    if (!g_inject_armed) {
        clamp_set_coupling(c);
    }
}

void clamp_afc_set_gain(clamp_gain_t g) {
    clamp_set_gain(g);
}

void clamp_afc_inject_enable(int on) {
    if (on) {
        if (!clamp_afc_inject_safety_ok()) {
            /* Safety violation — do not enable */
            return;
        }
        clamp_inject_enable(1);
        g_inject_armed = 1;
    } else {
        clamp_inject_enable(0);
        g_inject_armed = 0;
    }
}

int clamp_afc_jaw_closed(void) {
    return clamp_jaw_closed();
}

void clamp_afc_tdr_discharge(void) {
    clamp_tdr_discharge();
}

int clamp_afc_inject_safety_ok(void) {
    /* 1. Jaw must be closed (leaf switch). */
    if (!clamp_jaw_closed()) return 0;

    /* 2. TDR engine must have verified the cable is present and not
     *    carrying a hazardous voltage (the TDR classifier sets a flag). */
    if (!g_cable_verified_unpowered) {
        g_cable_verified_unpowered = tdr_engine_cable_is_safe_for_inject();
    }
    return g_cable_verified_unpowered;
}

/* Called by the TDR engine after a successful acquisition to update the
 * safety flag. */
void clamp_afc_set_cable_safe(int safe) {
    g_cable_verified_unpowered = safe;
}