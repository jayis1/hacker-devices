/* PDM Trust Probe diagnostics and calibration engine
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "diagnostics.h"
#include <limits.h>
#include <string.h>

#define REASON_NO_DATA       (1u << 0)
#define REASON_STUCK         (1u << 1)
#define REASON_DUPLICATE     (1u << 2)
#define REASON_DENSITY_SPAN  (1u << 3)
#define REASON_TRANSITION    (1u << 4)
#define REASON_CLOCK         (1u << 5)

static uint32_t clamp_u32(uint32_t value,
                          uint32_t minimum,
                          uint32_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static uint32_t average_u64(uint64_t total, uint32_t count)
{
    if (count == 0u) {
        return 0u;
    }
    return (uint32_t)(total / count);
}

static uint32_t mix_word(uint32_t hash, uint32_t value)
{
    unsigned index;

    for (index = 0u; index < 4u; ++index) {
        hash ^= (value >> (index * 8u)) & 0xFFu;
        hash *= 16777619u;
    }
    return hash;
}

void diagnostics_init(ptp_calibration_t *calibration, uint8_t channel_mask)
{
    unsigned channel;

    if (calibration == NULL) {
        return;
    }
    memset(calibration, 0, sizeof(*calibration));
    calibration->clock_min_hz = UINT_MAX;
    calibration->channel_mask = (uint8_t)(channel_mask & 0x0Fu);
    for (channel = 0u; channel < PTP_CHANNEL_COUNT; ++channel) {
        calibration->density_min[channel] = UINT_MAX;
        calibration->transition_min[channel] = UINT_MAX;
    }
}

bool diagnostics_observe(ptp_calibration_t *calibration,
                         const ptp_window_t *window,
                         uint32_t clock_hz)
{
    unsigned channel;

    if (calibration == NULL || window == NULL || window->clocks == 0u) {
        return false;
    }
    if ((window->channel_mask & calibration->channel_mask) == 0u) {
        return false;
    }

    calibration->sample_windows++;
    calibration->clock_sum_hz += clock_hz;
    if (clock_hz < calibration->clock_min_hz) {
        calibration->clock_min_hz = clock_hz;
    }
    if (clock_hz > calibration->clock_max_hz) {
        calibration->clock_max_hz = clock_hz;
    }

    for (channel = 0u; channel < PTP_CHANNEL_COUNT; ++channel) {
        uint32_t density;
        uint32_t transitions;

        if ((window->channel_mask & (1u << channel)) == 0u) {
            continue;
        }
        if ((calibration->channel_mask & (1u << channel)) == 0u) {
            continue;
        }

        density = (window->ones[channel] * 1000u) / window->clocks;
        transitions = (window->transitions[channel] * 1000u) / window->clocks;
        calibration->density_sum[channel] += density;
        calibration->transition_sum[channel] += transitions;

        if (density < calibration->density_min[channel]) {
            calibration->density_min[channel] = density;
        }
        if (density > calibration->density_max[channel]) {
            calibration->density_max[channel] = density;
        }
        if (transitions < calibration->transition_min[channel]) {
            calibration->transition_min[channel] = transitions;
        }
        if (transitions > calibration->transition_max[channel]) {
            calibration->transition_max[channel] = transitions;
        }
        if (window->ones[channel] == 0u ||
            window->ones[channel] == window->clocks) {
            calibration->stuck_windows[channel]++;
        }
        if (channel > 0u && (window->flags & (1u << channel)) != 0u) {
            calibration->duplicate_windows[channel]++;
        }
    }
    return true;
}

bool diagnostics_policy(const ptp_calibration_t *calibration,
                        ptp_policy_t *policy)
{
    uint32_t density_low = 1000u;
    uint32_t density_high = 0u;
    uint32_t transition_low = 1000u;
    uint32_t transition_high = 0u;
    unsigned channel;

    if (calibration == NULL || policy == NULL) {
        return false;
    }
    if (calibration->sample_windows < 8u) {
        return false;
    }

    for (channel = 0u; channel < PTP_CHANNEL_COUNT; ++channel) {
        if ((calibration->channel_mask & (1u << channel)) == 0u) {
            continue;
        }
        if (calibration->density_min[channel] < density_low) {
            density_low = calibration->density_min[channel];
        }
        if (calibration->density_max[channel] > density_high) {
            density_high = calibration->density_max[channel];
        }
        if (calibration->transition_min[channel] < transition_low) {
            transition_low = calibration->transition_min[channel];
        }
        if (calibration->transition_max[channel] > transition_high) {
            transition_high = calibration->transition_max[channel];
        }
    }

    policy->density_min_permille = (uint16_t)clamp_u32(
        density_low > 60u ? density_low - 60u : 10u,
        10u,
        450u);
    policy->density_max_permille = (uint16_t)clamp_u32(
        density_high + 60u,
        550u,
        990u);
    policy->transition_min_permille = (uint16_t)clamp_u32(
        transition_low > 30u ? transition_low - 30u : 5u,
        5u,
        400u);
    policy->transition_max_permille = (uint16_t)clamp_u32(
        transition_high + 50u,
        500u,
        990u);
    policy->clock_min_hz = clamp_u32(
        calibration->clock_min_hz - calibration->clock_min_hz / 20u,
        PTP_CLOCK_MIN_HZ,
        PTP_CLOCK_MAX_HZ - 1u);
    policy->clock_max_hz = clamp_u32(
        calibration->clock_max_hz + calibration->clock_max_hz / 20u,
        policy->clock_min_hz + 1u,
        PTP_CLOCK_MAX_HZ);
    policy->enabled_channels = calibration->channel_mask;
    return true;
}

ptp_health_report_t diagnostics_health(const ptp_calibration_t *calibration,
                                       uint8_t channel)
{
    ptp_health_report_t report;
    uint32_t density_span;
    uint32_t transition_average;

    memset(&report, 0, sizeof(report));
    report.channel = channel;
    report.level = HEALTH_UNKNOWN;

    if (calibration == NULL || channel >= PTP_CHANNEL_COUNT) {
        report.reason_mask = REASON_NO_DATA;
        return report;
    }
    if (calibration->sample_windows == 0u ||
        (calibration->channel_mask & (1u << channel)) == 0u) {
        report.reason_mask = REASON_NO_DATA;
        return report;
    }

    report.level = HEALTH_GOOD;
    report.score = 1000u;
    density_span = calibration->density_max[channel] -
                   calibration->density_min[channel];
    transition_average = average_u64(
        calibration->transition_sum[channel],
        calibration->sample_windows);

    if (calibration->stuck_windows[channel] != 0u) {
        report.reason_mask |= REASON_STUCK;
        report.score = report.score > 450u ? report.score - 450u : 0u;
        report.level = HEALTH_FAIL;
    }
    if (calibration->duplicate_windows[channel] >
        calibration->sample_windows / 2u) {
        report.reason_mask |= REASON_DUPLICATE;
        report.score = report.score > 250u ? report.score - 250u : 0u;
        if (report.level < HEALTH_WARN) {
            report.level = HEALTH_WARN;
        }
    }
    if (density_span > 700u) {
        report.reason_mask |= REASON_DENSITY_SPAN;
        report.score = report.score > 160u ? report.score - 160u : 0u;
        if (report.level < HEALTH_WARN) {
            report.level = HEALTH_WARN;
        }
    }
    if (transition_average < 25u || transition_average > 950u) {
        report.reason_mask |= REASON_TRANSITION;
        report.score = report.score > 300u ? report.score - 300u : 0u;
        report.level = HEALTH_FAIL;
    }
    if (calibration->clock_min_hz < PTP_CLOCK_MIN_HZ ||
        calibration->clock_max_hz > PTP_CLOCK_MAX_HZ) {
        report.reason_mask |= REASON_CLOCK;
        report.score = report.score > 400u ? report.score - 400u : 0u;
        report.level = HEALTH_FAIL;
    }
    return report;
}

uint32_t diagnostics_fingerprint(const ptp_calibration_t *calibration)
{
    uint32_t hash = 2166136261u;
    unsigned channel;

    if (calibration == NULL) {
        return 0u;
    }
    hash = mix_word(hash, calibration->sample_windows);
    hash = mix_word(hash, calibration->clock_min_hz);
    hash = mix_word(hash, calibration->clock_max_hz);
    hash = mix_word(hash, calibration->channel_mask);
    for (channel = 0u; channel < PTP_CHANNEL_COUNT; ++channel) {
        hash = mix_word(hash, calibration->density_min[channel]);
        hash = mix_word(hash, calibration->density_max[channel]);
        hash = mix_word(hash, calibration->transition_min[channel]);
        hash = mix_word(hash, calibration->transition_max[channel]);
        hash = mix_word(hash, calibration->stuck_windows[channel]);
        hash = mix_word(hash, calibration->duplicate_windows[channel]);
    }
    return hash;
}
