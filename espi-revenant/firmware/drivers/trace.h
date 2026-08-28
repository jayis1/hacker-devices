/*
 * trace.h - trace capture and anomaly scoring for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ESPI_REVENANT_TRACE_H
#define ESPI_REVENANT_TRACE_H

#include "../board.h"

typedef struct {
    er_trace_frame_t frames[ER_MAX_TRACE_FRAMES];
    size_t count;
    size_t write_index;
    uint32_t anomaly_score;
    uint32_t exported;
} er_trace_log_t;

void er_trace_init(er_trace_log_t *log);
void er_trace_push(er_trace_log_t *log, const er_trace_frame_t *frame);
uint32_t er_trace_score_frame(const er_trace_frame_t *frame);
void er_trace_summarize(const er_trace_log_t *log, char *buffer, size_t buffer_len);
void er_trace_format_latest(const er_trace_log_t *log, char *buffer, size_t buffer_len);

#endif
