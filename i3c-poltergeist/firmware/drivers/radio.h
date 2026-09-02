/*
 * radio.h - I3C Poltergeist operator-link helpers
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef I3C_POLTERGEIST_RADIO_H
#define I3C_POLTERGEIST_RADIO_H

#include "../board.h"

void ip_radio_init(ip_system_t *sys);
void ip_radio_heartbeat(const ip_system_t *sys, char *buffer, size_t buffer_size);
void ip_radio_export_frame(const ip_system_t *sys, char *buffer, size_t buffer_size, size_t event_limit);

#endif
