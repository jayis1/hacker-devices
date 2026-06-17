/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Amplifier Driver — Implementation
 *
 * Controls MAX98357A and AD8694 amplifier gain/power levels.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "amp_driver.h"

void amp_set_tx_power(enum tx_power_level level)
{
    switch (level) {
    case TX_POWER_WHISPER:
        /* Lowest power: GAIN pin low, we'll use 192 kHz I²S for 3 dB gain */
        amp_max98357a_set_boost(0);
        /* Note: I²S sample rate switching handled by codec driver */
        break;

    case TX_POWER_LOW:
        /* Low: 96 kHz I²S, no boost = 9 dB */
        amp_max98357a_set_boost(0);
        break;

    case TX_POWER_MED:
        /* Medium: 96 kHz I²S, +6 dB boost = 15 dB */
        amp_max98357a_set_boost(1);
        break;

    case TX_POWER_HIGH:
        /* High: 48 kHz I²S, +6 dB boost = 21 dB (max power) */
        amp_max98357a_set_boost(1);
        /* Note: Codec driver should switch to 48 kHz for max power */
        break;
    }

    /* Ensure amplifier is active when power is set */
    amp_max98357a_set_shutdown(1);
}

void amp_set_rx_gain(enum rx_gain_level level)
{
    switch (level) {
    case RX_GAIN_LOW:
        /* ×1 — minimal amplification, for near-field strong signals */
        amp_ad8694_set_gain(0);
        break;

    case RX_GAIN_MED:
        /* ×10 — medium gain */
        /* Placeholder: full design uses analog switch for gain selection */
        amp_ad8694_set_gain(0);
        /* Would set second gain stage here */
        break;

    case RX_GAIN_HIGH:
        /* ×50 — maximum sensitivity */
        amp_ad8694_set_gain(1);
        break;

    case RX_GAIN_AUTO:
        /* Automatic — algorithm would measure SNR and adjust */
        /* Default to medium gain */
        amp_ad8694_set_gain(0);
        break;
    }

    /* Ensure pre-amp is active */
    amp_ad8694_set_shutdown(1);
}