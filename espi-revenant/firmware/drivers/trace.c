/*
 * trace.c - trace capture and anomaly scoring for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "espi_bus.h"

void er_trace_init(er_trace_log_t *log)
{
    memset(log, 0, sizeof(*log));
}

uint32_t er_trace_score_frame(const er_trace_frame_t *frame)
{
    uint32_t score = 0u;
    switch (frame->channel) {
        case ER_CH_VWIRE:
            if ((frame->data[0] & 0x03u) == 0x03u) {
                score += 15u;
            }
            if (frame->data[2] > 4u) {
                score += 5u;
            }
            break;
        case ER_CH_PERIPHERAL:
            if (frame->data[1] == 0xA5u) {
                score += 11u;
            }
            if (frame->data[3] == 0u) {
                score += 4u;
            }
            break;
        case ER_CH_OOB:
            if (frame->data[1] && frame->data[2] && frame->data[3]) {
                score += 9u;
            }
            break;
        case ER_CH_FLASH:
            if (frame->data[1] > 100u) {
                score += 14u;
            }
            if (frame->data[3] == 0u) {
                score += 2u;
            }
            break;
        default:
            score += 1u;
            break;
    }
    return score;
}

void er_trace_push(er_trace_log_t *log, const er_trace_frame_t *frame)
{
    log->frames[log->write_index] = *frame;
    log->write_index = (log->write_index + 1u) % ER_MAX_TRACE_FRAMES;
    if (log->count < ER_MAX_TRACE_FRAMES) {
        log->count++;
    }
    log->anomaly_score += er_trace_score_frame(frame);
}

static const er_trace_frame_t *latest_frame(const er_trace_log_t *log)
{
    if (log->count == 0u) {
        return NULL;
    }
    size_t idx = (log->write_index == 0u) ? (ER_MAX_TRACE_FRAMES - 1u) : (log->write_index - 1u);
    return &log->frames[idx];
}

void er_trace_summarize(const er_trace_log_t *log, char *buffer, size_t buffer_len)
{
    size_t vwire = 0u;
    size_t periph = 0u;
    size_t oob = 0u;
    size_t flash = 0u;
    for (size_t i = 0u; i < log->count; ++i) {
        switch (log->frames[i].channel) {
            case ER_CH_VWIRE:
                vwire++;
                break;
            case ER_CH_PERIPHERAL:
                periph++;
                break;
            case ER_CH_OOB:
                oob++;
                break;
            case ER_CH_FLASH:
                flash++;
                break;
            default:
                break;
        }
    }
    snprintf(buffer,
             buffer_len,
             "frames=%zu vwire=%zu periph=%zu oob=%zu flash=%zu anomaly=%u",
             log->count,
             vwire,
             periph,
             oob,
             flash,
             log->anomaly_score);
}

void er_trace_format_latest(const er_trace_log_t *log, char *buffer, size_t buffer_len)
{
    const er_trace_frame_t *frame = latest_frame(log);
    if (frame == NULL) {
        snprintf(buffer, buffer_len, "no-trace");
        return;
    }
    er_bus_describe_frame(frame, buffer, buffer_len);
}
