/*
 * timing_fabric.h - I3C Poltergeist timing fabric controls
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef I3C_POLTERGEIST_TIMING_FABRIC_H
#define I3C_POLTERGEIST_TIMING_FABRIC_H

#include "../board.h"

void ip_fabric_init(ip_system_t *sys);
void ip_fabric_apply_profile(ip_system_t *sys, const ip_profile_t *profile);
int ip_fabric_match_trigger(const ip_profile_t *profile, uint16_t ccc);
void ip_fabric_record_health(ip_system_t *sys, char *buffer, size_t buffer_size);
uint32_t ip_fabric_estimate_delay_ns(const ip_profile_t *profile, uint16_t ccc, uint8_t target_index);
int ip_fabric_should_mirror_ccc(const ip_profile_t *profile, uint16_t ccc, uint8_t target_index);

#endif
