/*
 * cec.h - EDID Phantom CEC subsystem
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_CEC_H
#define EDID_PHANTOM_CEC_H

#include "../board.h"

void cec_init(void);
void cec_set_profile(const ep_profile_t *profile);
void cec_service(uint32_t now_ms, ep_status_t *status);
void cec_inject_guarded_ping(ep_status_t *status);
void cec_build_report(char *buffer, size_t length);
const ep_cec_frame_t *cec_frame(size_t index);
size_t cec_frame_count(void);

#endif
