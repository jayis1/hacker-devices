/* PDM capture driver
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef PTP_PDM_CAPTURE_H
#define PTP_PDM_CAPTURE_H
#include "../board.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint64_t timestamp;
    uint32_t clocks;
    uint32_t ones[PTP_CHANNEL_COUNT];
    uint32_t transitions[PTP_CHANNEL_COUNT];
    uint32_t fingerprints[PTP_CHANNEL_COUNT];
    uint8_t channel_mask;
    uint8_t flags;
} ptp_window_t;
typedef struct { ptp_window_t windows[64]; size_t read_index; size_t write_index; size_t count; uint32_t clock_hz; uint32_t dropped; bool running; } ptp_capture_t;
void capture_init(ptp_capture_t *capture);
bool capture_configure(ptp_capture_t *capture, uint32_t clock_hz);
void capture_start(ptp_capture_t *capture);
void capture_stop(ptp_capture_t *capture);
bool capture_push_bits(ptp_capture_t *capture, const uint8_t *interleaved, size_t clocks, uint8_t channel_mask, uint64_t timestamp);
bool capture_next(ptp_capture_t *capture, ptp_window_t *window);
#endif
