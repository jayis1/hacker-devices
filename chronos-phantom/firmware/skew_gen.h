/*
 * skew_gen.h — Skew profile generator (step, ramp, stealth, jitter, sawtooth)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_SKEW_GEN_H
#define CHRONOS_PHANTOM_SKEW_GEN_H

#include <stdint.h>
#include "ptp_engine.h"

/* Preset skew profiles for quick selection from the app */
typedef enum {
    SKEW_PRESET_KERBEROS_EXT   = 0,  /* +5 min step                          */
    SKEW_PRESET_KERBEROS_REPLAY = 1,  /* -10 min step                         */
    SKEW_PRESET_PMU_SLOW        = 2,  /* 0.5 ppm stealth drift                */
    SKEW_PRESET_PMU_SAWTOOTH    = 3,  /* ±1 ms sawtooth, 60s period           */
    SKEW_PRESET_JITTER_100US    = 4,  /* ±100 µs jitter                       */
    SKEW_PRESET_CUSTOM          = 0xFF
} skew_preset_t;

void skew_gen_apply_preset(skew_config_t *cfg, skew_preset_t preset);
void skew_gen_set_custom(skew_config_t *cfg, skew_profile_t profile,
                          int64_t offset_ns, int64_t rate_nsps,
                          uint32_t jitter_amp_ns, uint32_t sawtooth_period_ms);

/* Returns a human-readable label for a profile (for the app) */
const char *skew_gen_profile_label(skew_profile_t p);

#endif /* CHRONOS_PHANTOM_SKEW_GEN_H */