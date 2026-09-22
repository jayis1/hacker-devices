/* Privacy-preserving PDM analyzer
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef PTP_ANALYZER_H
#define PTP_ANALYZER_H
#include "pdm_capture.h"
typedef enum { EVT_CLOCK=1, EVT_STUCK=2, EVT_DUPLICATE=3, EVT_DENSITY=4, EVT_TRANSITION=5, EVT_OVERFLOW=6, EVT_POLICY=7 } ptp_event_type_t;
typedef struct { uint64_t timestamp; uint32_t value; uint16_t detail; uint8_t type; uint8_t channel; } ptp_event_t;
typedef struct { uint16_t density_min_permille; uint16_t density_max_permille; uint16_t transition_min_permille; uint16_t transition_max_permille; uint32_t clock_min_hz; uint32_t clock_max_hz; uint8_t enabled_channels; } ptp_policy_t;
typedef struct { ptp_policy_t policy; ptp_event_t events[PTP_EVENT_CAPACITY]; size_t read_index; size_t write_index; size_t count; uint32_t windows_seen; uint32_t violations; uint32_t event_drops; } ptp_analyzer_t;
void analyzer_init(ptp_analyzer_t *analyzer);
bool analyzer_set_policy(ptp_analyzer_t *analyzer,const ptp_policy_t *policy);
void analyzer_consume(ptp_analyzer_t *analyzer,const ptp_window_t *window,uint32_t clock_hz);
bool analyzer_next_event(ptp_analyzer_t *analyzer,ptp_event_t *event);
#endif
