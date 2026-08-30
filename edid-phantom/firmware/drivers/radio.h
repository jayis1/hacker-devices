/*
 * radio.h - EDID Phantom radio/control plane simulator
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_RADIO_H
#define EDID_PHANTOM_RADIO_H

#include "../board.h"

void radio_init(void);
void radio_set_profile(const ep_profile_t *profile);
void radio_service(uint32_t now_ms, ep_status_t *status);
void radio_emit_status(const ep_status_t *status);
void radio_emit_transcript(void);

#endif
