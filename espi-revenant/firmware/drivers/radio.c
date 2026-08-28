/*
 * radio.c - command framing for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "radio.h"

void er_radio_init(er_radio_t *radio)
{
    memset(radio, 0, sizeof(*radio));
}

void er_radio_accept_command(er_radio_t *radio, const char *command, er_event_t *event_out)
{
    snprintf(radio->last_command, sizeof(radio->last_command), "%s", command);
    event_out->code = ER_EVENT_COMMAND;
    event_out->channel = ER_CH_GPIO;
    event_out->risk = ER_RISK_LOW;
    snprintf(event_out->message, sizeof(event_out->message), "Command accepted: %s", command);
}

void er_radio_build_status_frame(er_radio_t *radio, const er_status_t *status, const char *trace_summary)
{
    radio->frame_counter++;
    snprintf(radio->last_frame,
             sizeof(radio->last_frame),
             "{id:%u,device:'%s',profile:'%s',mode:%u,armed:%u,bypass:%u,temp:%.1f,current:%.1f,trace:'%s'}",
             radio->frame_counter,
             ER_DEVICE_NAME,
             status->active_profile,
             status->mode,
             status->armed,
             status->bypass_enabled,
             status->board_temp_c,
             status->ec_current_ma,
             trace_summary);
}

const char *er_radio_last_frame(const er_radio_t *radio)
{
    return radio->last_frame;
}
