/*
 * safety.h - EDID Phantom safety and HPD controls
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_SAFETY_H
#define EDID_PHANTOM_SAFETY_H

#include "../board.h"

void safety_init(void);
void safety_apply_profile(const ep_profile_t *profile);
void safety_tick(uint32_t now_ms, ep_status_t *status);
void safety_pulse_hpd(ep_status_t *status);
void safety_build_report(char *buffer, size_t length, const ep_status_t *status);

#endif
