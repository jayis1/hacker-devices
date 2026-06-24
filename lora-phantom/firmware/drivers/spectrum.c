/*
 * lora-phantom / drivers/spectrum.c
 * Channel activity scan: RSSI mapping + CAD-based activity detection.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The spectrum scanner iterates over regional channels, measuring RSSI and
 * performing Channel Activity Detection (CAD) to determine if LoRa traffic
 * is present. This provides a deployment map (active channels, gateway
 * density estimate, DR distribution) for pre-engagement reconnaissance.
 */

#include "../board.h"
#include "../registers.h"
#include "../types.h"

/* Forward declarations */
int sx1262_cad(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint32_t timeout_ms);
int sx1262_get_rssi(int16_t *rssi);

static void delay_ms_local(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 48000; i++) { }
}

int spectrum_scan(const uint32_t *freqs, uint8_t n_freqs, uint8_t sf, uint8_t bw,
                  uint32_t dwell_ms, scan_channel_t *out, uint8_t out_max) {
    if (n_freqs == 0 || out == 0) return 0;
    if (n_freqs > out_max) n_freqs = out_max;

    for (uint8_t i = 0; i < n_freqs; i++) {
        out[i].freq_hz = freqs[i];
        out[i].rssi = 0;
        out[i].activity = 0;
        out[i].hits = 0;

        /* Measure RSSI (instantaneous) */
        int16_t rssi;
        if (sx1262_get_rssi(&rssi) == 0) {
            out[i].rssi = rssi;
        }

        /* Perform CAD for dwell_ms to detect LoRa activity */
        uint32_t cad_iters = dwell_ms / 100;  /* CAD ~100ms each at SF7 */
        if (cad_iters < 1) cad_iters = 1;
        for (uint32_t j = 0; j < cad_iters; j++) {
            int cad = sx1262_cad(freqs[i], sf, bw, 200);
            if (cad == 1) {
                out[i].activity = 1;
                out[i].hits++;
            }
            delay_ms_local(10);
        }
    }

    return (int)n_freqs;
}