/*
 * lldp.h - LLDP / LLDP-MED helpers for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_LLDP_H
#define POE_WHISPER_LLDP_H

#include "../board.h"

typedef struct {
    char chassis_id[32];
    char port_id[32];
    char system_name[48];
    char platform[48];
    float requested_power_w;
    uint8_t priority;
} pw_lldp_profile_t;

size_t pw_lldp_build_advertisement(const pw_lldp_profile_t *profile, uint8_t *out, size_t out_len);
int pw_lldp_parse_summary(const uint8_t *buf, size_t len, pw_lldp_profile_t *out);
pw_lldp_profile_t pw_lldp_default_profile(const char *system_name, float requested_power_w);
void pw_lldp_format_summary(const pw_lldp_profile_t *profile, char *out, size_t out_len);

#endif
