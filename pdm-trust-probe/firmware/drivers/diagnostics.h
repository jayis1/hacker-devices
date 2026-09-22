/* PDM Trust Probe diagnostics and calibration engine
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef PTP_DIAGNOSTICS_H
#define PTP_DIAGNOSTICS_H
#include "analyzer.h"
typedef enum {
    HEALTH_UNKNOWN = 0,
    HEALTH_GOOD = 1,
    HEALTH_WARN = 2,
    HEALTH_FAIL = 3
} ptp_health_level_t;
typedef struct {
    uint32_t sample_windows;
    uint32_t clock_sum_hz;
    uint32_t clock_min_hz;
    uint32_t clock_max_hz;
    uint64_t density_sum[PTP_CHANNEL_COUNT];
    uint32_t density_min[PTP_CHANNEL_COUNT];
    uint32_t density_max[PTP_CHANNEL_COUNT];
    uint64_t transition_sum[PTP_CHANNEL_COUNT];
    uint32_t transition_min[PTP_CHANNEL_COUNT];
    uint32_t transition_max[PTP_CHANNEL_COUNT];
    uint32_t stuck_windows[PTP_CHANNEL_COUNT];
    uint32_t duplicate_windows[PTP_CHANNEL_COUNT];
    uint8_t channel_mask;
} ptp_calibration_t;
typedef struct {
    ptp_health_level_t level;
    uint8_t channel;
    uint16_t score;
    uint32_t reason_mask;
} ptp_health_report_t;
void diagnostics_init(ptp_calibration_t *calibration, uint8_t channel_mask);
bool diagnostics_observe(ptp_calibration_t *calibration,
                         const ptp_window_t *window,
                         uint32_t clock_hz);
bool diagnostics_policy(const ptp_calibration_t *calibration,
                        ptp_policy_t *policy);
ptp_health_report_t diagnostics_health(const ptp_calibration_t *calibration,
                                       uint8_t channel);
uint32_t diagnostics_fingerprint(const ptp_calibration_t *calibration);
#endif
