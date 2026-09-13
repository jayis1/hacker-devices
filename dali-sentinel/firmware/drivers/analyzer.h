/* DALI security analyzer API. Author: jayis1. */
#ifndef DALI_ANALYZER_H
#define DALI_ANALYZER_H
#include "dali_phy.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum { SEV_INFO=0, SEV_LOW=1, SEV_MEDIUM=2, SEV_HIGH=3, SEV_CRITICAL=4 } finding_severity_t;
typedef enum {
 FIND_NONE=0, FIND_BAD_FRAMING, FIND_COLLISION, FIND_BROADCAST_OFF,
 FIND_BROADCAST_MAX, FIND_COMMISSIONING_START, FIND_COMMISSIONING_SEQUENCE,
 FIND_COMMISSIONING_OUTSIDE_WINDOW, FIND_ADDRESS_PROGRAMMED, FIND_RATE_HIGH,
 FIND_STUCK_LOW, FIND_DUPLICATE_REPLY, FIND_CAPTURE_LOSS, FIND_SAFETY_FAULT
} finding_code_t;
typedef struct {
 uint32_t timestamp_ms;
 finding_code_t code;
 finding_severity_t severity;
 uint32_t evidence_raw;
 uint16_t context;
 bus_side_t side;
} finding_t;
typedef struct {
 uint32_t frames_total, frames_bad, collisions, broadcasts, commissioning_commands;
 uint32_t window_started_ms, frames_in_window, last_forward_ms, last_backward_ms;
 uint8_t commissioning_state;
 uint8_t last_query_address;
 uint32_t maintenance_start_minute, maintenance_end_minute;
} analyzer_stats_t;
void analyzer_init(uint32_t maintenance_start_minute, uint32_t maintenance_end_minute);
void analyzer_consume(const dali_frame_t *frame, uint32_t now_ms, uint32_t minute_of_day);
void analyzer_note_bus(bool low, uint32_t now_ms, bus_side_t side);
void analyzer_note_capture_loss(uint32_t count, uint32_t now_ms);
bool analyzer_next_finding(finding_t *finding);
analyzer_stats_t analyzer_stats(void);
const char *analyzer_finding_name(finding_code_t code);
bool analyzer_run_self_test(void);
#endif
