/*
 * telemetry.h - I3C Poltergeist telemetry renderers
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef I3C_POLTERGEIST_TELEMETRY_H
#define I3C_POLTERGEIST_TELEMETRY_H

#include "../board.h"

void ip_telemetry_status_line(const ip_system_t *sys, char *buffer, size_t buffer_size);
void ip_telemetry_render_json(const ip_system_t *sys, char *buffer, size_t buffer_size);
void ip_telemetry_render_events(const ip_system_t *sys, char *buffer, size_t buffer_size);

#endif
