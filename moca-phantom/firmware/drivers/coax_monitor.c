/*
 * MoCA Phantom Coax Spectrum Monitor Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>

#include "coax_monitor.h"

void mph_coax_monitor_init(void) {
    /* Intentionally empty for simulation. */
}

void mph_coax_monitor_tick(uint32_t epoch_ms, mph_spectrum_snapshot_t *snapshot, mph_metrics_t *metrics) {
    uint8_t channel;
    int16_t energy;

    if (snapshot == NULL || metrics == NULL) {
        return;
    }

    channel = (uint8_t)((epoch_ms / 100u) % MPH_MAX_CHANNELS);
    energy = (int16_t)(-58 + (int16_t)(channel * 3) - (int16_t)((epoch_ms / 400u) % 5u));

    snapshot->strongest_channel = channel;
    snapshot->strongest_energy_db = energy;
    snapshot->suspicious_hops = (uint8_t)((epoch_ms / 250u) % 4u);
    snapshot->leakage_suspected = (channel == 1u || channel == 6u);

    if (energy > -47) {
        metrics->spectrum_peaks += 1u;
    }
    if (snapshot->leakage_suspected) {
        metrics->alerts_raised += 1u;
    }
}

void mph_coax_monitor_print(const mph_spectrum_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }

    printf("[spectrum] strongest_channel=%u energy=%ddB suspicious_hops=%u leakage=%s\n",
           snapshot->strongest_channel,
           (int)snapshot->strongest_energy_db,
           snapshot->suspicious_hops,
           snapshot->leakage_suspected ? "yes" : "no");
}
