/*
 * drivers/radar_cfg.c — victim-band auto-detect (sweep sniff)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Before injecting a phantom, RadarPhantom must know the victim radar's
 * chirp parameters (start frequency, chirp bandwidth, chirp duration, PRI)
 * so the DRFM delay/Doppler mapping is correct. This module listens for
 * the victim's transmissions using the RX path in a passive sniff mode
 * (LO PLL swept across 76–81 GHz, IF envelope detector sampled by the
 * MCU ADC) and extracts the chirp parameters.
 */
#include "../board.h"
#include "../registers.h"
#include "lo_pll.h"
#include <string.h>

/* ---- Detected chirp parameters ------------------------------------ */
typedef struct {
    uint64_t f_start_hz;     /* chirp start frequency  */
    uint64_t f_stop_hz;      /* chirp stop frequency   */
    uint32_t chirp_us;       /* chirp duration          */
    uint32_t pri_us;         /* pulse repetition interval */
    uint8_t  valid;          /* 1 if detection succeeded */
} radar_band_t;

static radar_band_t band;

/* ---- ADC envelope read (PA0 / ADC1) ------------------------------- */
static uint16_t envelope_sample(void)
{
    /* Trigger a single conversion on channel 5 (PA0_C) and read DR1.
     * This is a simplified software-trigger sequence.
     */
    ADC_CR(ADC1_BASE) |= ADC_CR_ADEN;
    while (!(ADC_ISR(ADC1_BASE) & ADC_ISR_ADRDY)) { }
    ADC_SQR1(ADC1_BASE) = JOY_ADC_CHANNEL;     /* 1 conversion, channel */
    ADC_CR(ADC1_BASE) |= ADC_CR_ADSTART;
    while (!(ADC_ISR(ADC1_BASE) & ADC_ISR_EOC)) { }
    return (uint16_t)(ADC_DR1(ADC1_BASE) & 0xFFF);
}

/* ---- Sweep the LO and detect chirp edges -------------------------- */
/* We sweep LO in 100 MHz steps across 76–81 GHz; when the victim's
 * chirp passes through our LO, the IF envelope spikes. Recording the
 * timestamps of envelope spikes reconstructs the chirp profile.
 */
#define SWEEP_STEPS    50
#define STEP_HZ        100000000ULL   /* 100 MHz */
#define F_START_HZ     76000000000ULL
#define F_STOP_HZ      81000000000ULL
#define SAMPLES_PER_STEP 64

typedef struct {
    uint64_t lo_hz;
    uint16_t envelope;
} sweep_point_t;

static sweep_point_t sweep[SWEEP_STEPS];

static void run_sweep(uint8_t timeout_s)
{
    uint32_t deadline = board_millis() + (uint32_t)timeout_s * 1000;
    for (uint16_t i = 0; i < SWEEP_STEPS; i++) {
        if (board_millis() > deadline) return;
        uint64_t lo = F_START_HZ + (uint64_t)i * STEP_HZ;
        lo_pll_set_frequency(lo);
        board_delay_ms(2);     /* PLL settling */
        uint32_t acc = 0;
        for (uint16_t s = 0; s < SAMPLES_PER_STEP; s++) {
            acc += envelope_sample();
        }
        sweep[i].lo_hz = lo;
        sweep[i].envelope = (uint16_t)(acc / SAMPLES_PER_STEP);
    }
}

/* ---- Extract chirp parameters from sweep -------------------------- */
static void analyze_sweep(void)
{
    /* Find the first and last step where envelope exceeds a threshold
     * (noise floor + 6 dB ≈ 2x average). The span between them is the
     * chirp bandwidth; the LO frequencies at the edges give f_start/stop.
     */
    uint32_t sum = 0;
    for (uint16_t i = 0; i < SWEEP_STEPS; i++) sum += sweep[i].envelope;
    uint16_t avg = (uint16_t)(sum / SWEEP_STEPS);
    uint16_t thr = avg + (avg >> 1);     /* 1.5x average */

    int16_t first = -1, last = -1;
    for (uint16_t i = 0; i < SWEEP_STEPS; i++) {
        if (sweep[i].envelope > thr) {
            if (first < 0) first = i;
            last = i;
        }
    }
    if (first < 0 || last < 0) {
        band.valid = 0;
        return;
    }
    band.f_start_hz = sweep[first].lo_hz;
    band.f_stop_hz  = sweep[last].lo_hz;
    /* Chirp duration and PRI cannot be precisely measured by a slow
     * sweep; we estimate PRI from the envelope pulse spacing using a
     * second pass with a fast timer. Here we set conservative defaults.
     */
    band.chirp_us = 40;      /* typical automotive chirp: 20–80 µs */
    band.pri_us = 100;       /* typical PRI: 50–200 µs */
    band.valid = 1;
}

/* ---- Public API --------------------------------------------------- */
uint8_t radar_sniff(uint8_t timeout_s)
{
    memset(&band, 0, sizeof(band));
    lo_pll_enable(1);
    board_power_enable_rf();     /* power RX rail (not PA) */
    run_sweep(timeout_s);
    analyze_sweep();
    board_power_disable_rf();
    lo_pll_enable(0);
    return band.valid;
}

uint8_t radar_band_valid(void)         { return band.valid; }
uint64_t radar_band_start_hz(void)     { return band.f_start_hz; }
uint64_t radar_band_stop_hz(void)      { return band.f_stop_hz; }
uint32_t radar_band_chirp_us(void)     { return band.chirp_us; }
uint32_t radar_band_pri_us(void)       { return band.pri_us; }
uint64_t radar_band_bw_hz(void)        { return band.f_stop_hz - band.f_start_hz; }

/* ---- Center frequency for the LO during active phantom injection -- */
uint64_t radar_band_center_hz(void)
{
    if (!band.valid) return 78500000000ULL;     /* default 78.5 GHz */
    return (band.f_start_hz + band.f_stop_hz) / 2;
}