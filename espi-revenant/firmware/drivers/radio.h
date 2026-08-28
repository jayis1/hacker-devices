/*
 * radio.h - command framing for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ESPI_REVENANT_RADIO_H
#define ESPI_REVENANT_RADIO_H

#include "../board.h"

typedef struct {
    char last_command[ER_MAX_COMMAND_BYTES];
    char last_frame[ER_MAX_COMMAND_BYTES];
    uint32_t frame_counter;
} er_radio_t;

void er_radio_init(er_radio_t *radio);
void er_radio_accept_command(er_radio_t *radio, const char *command, er_event_t *event_out);
void er_radio_build_status_frame(er_radio_t *radio, const er_status_t *status, const char *trace_summary);
const char *er_radio_last_frame(const er_radio_t *radio);

#endif
