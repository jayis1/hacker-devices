/*
 * clamp_afc.h — clamp analog front-end control
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_CLAMP_AFC_H
#define PULSEREAPER_CLAMP_AFC_H

#include "board.h"

void clamp_afc_init(void);

/* Set coupling mode (cap / ind / both) */
void clamp_afc_set_coupling(clamp_coupling_t c);

/* Set front-end gain */
void clamp_afc_set_gain(clamp_gain_t g);

/* Enable / disable the inject driver */
void clamp_afc_inject_enable(int on);

/* Read the jaw-closed leaf switch (1=closed) */
int  clamp_afc_jaw_closed(void);

/* Discharge the TDR path */
void clamp_afc_tdr_discharge(void);

/* Safety check: refuse to enable inject if jaw open or cable live.
 * Returns 1 if safe to inject. */
int  clamp_afc_inject_safety_ok(void);

/* Called by the TDR engine after a successful acquisition to update the
 * cable-safety flag (1 = safe for inject, 0 = live or unknown). */
void clamp_afc_set_cable_safe(int safe);

#endif