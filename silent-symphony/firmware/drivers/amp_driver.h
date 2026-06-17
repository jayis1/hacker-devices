/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Amplifier Driver — MAX98357A + AD8694 Control
 *
 * Controls the MAX98357A class-D amplifier (Tx) and
 * the AD8694 quad op-amp (Rx pre-amplifier).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef AMP_DRIVER_H
#define AMP_DRIVER_H

#include <stdint.h>
#include "../board.h"

/* =========================================================================
 * MAX98357A Class-D Amplifier (I²S audio output → Murata piezo)
 * ========================================================================= */

/**
 * The MAX98357A is configured entirely through I²S data format and
 * hardware pin strapping. There is no I²C/SPI control interface.
 * 
 * FS (frame sync) rate determines gain:
 *   - 48 kHz    → 15 dB gain (high power)
 *   - 96 kHz    → 9 dB gain  (medium power) — DEFAULT
 *   - 192 kHz   → 3 dB gain  (low power)
 * 
 * GAIN pin (PC2):
 *   - LOW (0)   → Gain set by FS rate
 *   - HIGH (1)  → +6 dB additional gain
 * 
 * SHDN pin (PC1):
 *   - LOW (0)   → Shutdown (1 µA)
 *   - HIGH (1)  → Active
 * 
 * Our I²S1 Tx runs at 96 kHz → 9 dB base gain.
 * GAIN=0 gives 9 dB (default). GAIN=1 gives 15 dB.
 * For whisper mode we can switch to 192 kHz I²S → 3 dB + GAIN=1 → 9 dB.
 */

/**
 * Initialize MAX98357A amplifier.
 * Sets SHDN high (active), GAIN low (9 dB).
 */
static inline void amp_max98357a_init(void)
{
    uint32_t moder = mmio_read32(PORT_AMP + GPIO_MODER);

    /* PC1 = SHDN: output */
    moder &= ~(GPIO_MODE_MASK(PIN_AMP_SD));
    moder |= (GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(PIN_AMP_SD));
    
    /* PC2 = GAIN: output */
    moder &= ~(GPIO_MODE_MASK(PIN_AMP_GAIN));
    moder |= (GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(PIN_AMP_GAIN));
    
    mmio_write32(PORT_AMP + GPIO_MODER, moder);

    /* Default: SHDN high (active), GAIN low (9 dB) */
    mmio_write32(PORT_AMP + GPIO_BSRR, 
                 (1U << PIN_AMP_SD) |                     /* SHDN = high (set) */
                 ((uint32_t)1U << (PIN_AMP_GAIN + 16)));  /* GAIN = low (reset) */
}

/**
 * Set amplifier shutdown state.
 * 
 * @param on  0 = shutdown (1 µA), 1 = active
 */
static inline void amp_max98357a_set_shutdown(uint8_t on)
{
    if (on) {
        mmio_write32(PORT_AMP + GPIO_BSRR, (1U << PIN_AMP_SD));        /* Set = high = active */
    } else {
        mmio_write32(PORT_AMP + GPIO_BSRR, ((uint32_t)1U << (PIN_AMP_SD + 16))); /* Reset = low = shutdown */
    }
}

/**
 * Set amplifier gain boost.
 * 
 * @param boost  0 = normal (9 dB @ 96 kHz), 1 = +6 dB (15 dB boost)
 */
static inline void amp_max98357a_set_boost(uint8_t boost)
{
    if (boost) {
        mmio_write32(PORT_AMP + GPIO_BSRR, (1U << PIN_AMP_GAIN));      /* Set = high = +6dB */
    } else {
        mmio_write32(PORT_AMP + GPIO_BSRR, ((uint32_t)1U << (PIN_AMP_GAIN + 16))); /* Reset = low = normal */
    }
}

/* =========================================================================
 * AD8694 Quad Op-Amp Pre-Amplifier (MEMS mic → bandpass → I²S)
 * ========================================================================= */

/**
 * The AD8694 is configured with fixed external passives for:
 *   - Two-stage bandpass filter: 10 kHz high-pass, 45 kHz low-pass
 *   - Programmable gain: ×1, ×10, ×50 via analog switch (controlled by GPIO)
 * 
 * For simplicity, we use a single gain pin (PC3) to toggle between:
 *   - LOW (×1)   — for strong signals / near-field
 *   - HIGH (×50) — for long-range / weak signals
 * 
 * SHDN pin (PC3):
 *   - LOW (0)  → Shutdown (power saving)
 *   - HIGH (1) → Active
 */

/**
 * Initialize AD8694 pre-amplifier.
 * Sets SHDN low (shutdown initially), gain low (×1).
 */
static inline void amp_ad8694_init(void)
{
    uint32_t moder = mmio_read32(PORT_PREAMP + GPIO_MODER);

    /* PC3 = PREAMP_SD: output */
    moder &= ~(GPIO_MODE_MASK(PIN_PREAMP_SD));
    moder |= (GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(PIN_PREAMP_SD));
    
    mmio_write32(PORT_PREAMP + GPIO_MODER, moder);

    /* Shutdown by default until codec is ready */
    mmio_write32(PORT_PREAMP + GPIO_BSRR, 
                 ((uint32_t)1U << (PIN_PREAMP_SD + 16)));  /* Reset = low = shutdown */
}

/**
 * Set pre-amplifier shutdown state.
 * 
 * @param on  0 = shutdown (power saving), 1 = active
 */
static inline void amp_ad8694_set_shutdown(uint8_t on)
{
    if (on) {
        mmio_write32(PORT_PREAMP + GPIO_BSRR, (1U << PIN_PREAMP_SD));   /* Set = high = active */
    } else {
        mmio_write32(PORT_PREAMP + GPIO_BSRR, ((uint32_t)1U << (PIN_PREAMP_SD + 16))); /* Reset = low = shutdown */
    }
}

/**
 * Set pre-amplifier gain.
 * 
 * @param gain  0 = ×1 (0 dB), 1 = ×50 (34 dB)
 */
static inline void amp_ad8694_set_gain(uint8_t gain)
{
    /* For now, we route through same pin. In full design, separate pin for gain select. */
    /* This is a placeholder — full design would have an analog switch control. */
    (void)gain;
}

/* =========================================================================
 * GPIO Mode macros (used by inline functions above)
 * ========================================================================= */
#define GPIO_MODE_MASK(pin)     (3U << ((pin) * 2))
#define GPIO_MODE_SHIFT(pin)    ((pin) * 2)

/* =========================================================================
 * Power Level Control
 * ========================================================================= */

/**
 * Set Tx power level by configuring amplifier and I²S sample rate.
 * 
 * @param level  Power level (TX_POWER_WHISPER, _LOW, _MED, _HIGH)
 */
void amp_set_tx_power(enum tx_power_level level);

/**
 * Set Rx gain level.
 * 
 * @param level  Gain level (RX_GAIN_LOW, _MED, _HIGH, _AUTO)
 */
void amp_set_rx_gain(enum rx_gain_level level);

#endif /* AMP_DRIVER_H */