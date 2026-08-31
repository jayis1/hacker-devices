/*
 * drivers/radio.h - MDIO Wraith companion link encoder
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MDIO_WRAITH_RADIO_H
#define MDIO_WRAITH_RADIO_H

#include "../board.h"

void mw_radio_build_heartbeat(const mw_system_t *sys, char *buffer, size_t length);
void mw_radio_build_capture_export(const mw_system_t *sys, char *buffer, size_t length, size_t max_frames);
void mw_radio_build_profile_frame(const mw_profile_t *profile, char *buffer, size_t length);

#endif
