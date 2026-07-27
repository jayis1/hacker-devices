/*
 * skew_gen.c — Skew profile generator presets and helpers
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "skew_gen.h"

void skew_gen_apply_preset(skew_config_t *cfg, skew_preset_t preset)
{
    switch (preset) {
    case SKEW_PRESET_KERBEROS_EXT:
        /* +5 minutes step — extend Kerberos ticket validity */
        cfg->profile = SKEW_STEP;
        cfg->offset_ns = 5LL * 60 * 1000000000LL;  /* +5 min in ns */
        cfg->rate_nsps = 0;
        cfg->jitter_amp_ns = 0;
        cfg->sawtooth_period_ms = 0;
        cfg->active = 1;
        break;
    case SKEW_PRESET_KERBEROS_REPLAY:
        /* -10 minutes step — allow replay of recently-expired tickets */
        cfg->profile = SKEW_STEP;
        cfg->offset_ns = -10LL * 60 * 1000000000LL;
        cfg->rate_nsps = 0;
        cfg->jitter_amp_ns = 0;
        cfg->sawtooth_period_ms = 0;
        cfg->active = 1;
        break;
    case SKEW_PRESET_PMU_SLOW:
        /* 0.5 ppm stealth drift — desync PMUs over ~30 min to 1 ms */
        cfg->profile = SKEW_STEALTH;
        cfg->offset_ns = 0;
        cfg->rate_nsps = 500;  /* 0.5 ppm = 500 ppb → 500 ns/s */
        cfg->jitter_amp_ns = 0;
        cfg->sawtooth_period_ms = 0;
        cfg->active = 1;
        break;
    case SKEW_PRESET_PMU_SAWTOOTH:
        /* ±1 ms sawtooth, 60s period — cause periodic relay misalignment */
        cfg->profile = SKEW_SAWTOOTH;
        cfg->offset_ns = 1000000LL;  /* 1 ms peak */
        cfg->rate_nsps = 0;
        cfg->jitter_amp_ns = 0;
        cfg->sawtooth_period_ms = 60000;  /* 60 s */
        cfg->active = 1;
        break;
    case SKEW_PRESET_JITTER_100US:
        /* ±100 µs jitter — disrupt phase-sensitive apps */
        cfg->profile = SKEW_JITTER;
        cfg->offset_ns = 0;
        cfg->rate_nsps = 0;
        cfg->jitter_amp_ns = 100000;  /* 100 µs */
        cfg->sawtooth_period_ms = 0;
        cfg->active = 1;
        break;
    default:
        cfg->active = 0;
        break;
    }
}

void skew_gen_set_custom(skew_config_t *cfg, skew_profile_t profile,
                          int64_t offset_ns, int64_t rate_nsps,
                          uint32_t jitter_amp_ns, uint32_t sawtooth_period_ms)
{
    cfg->profile = profile;
    cfg->offset_ns = offset_ns;
    cfg->rate_nsps = rate_nsps;
    cfg->jitter_amp_ns = jitter_amp_ns;
    cfg->sawtooth_period_ms = sawtooth_period_ms;
    cfg->active = 1;
}

const char *skew_gen_profile_label(skew_profile_t p)
{
    switch (p) {
    case SKEW_STEP:     return "Step (instant)";
    case SKEW_RAMP:     return "Ramp (linear)";
    case SKEW_STEALTH:  return "Stealth (sub-ppm drift)";
    case SKEW_JITTER:   return "Jitter (pseudo-random)";
    case SKEW_SAWTOOTH: return "Sawtooth (periodic)";
    default:           return "Unknown";
    }
}