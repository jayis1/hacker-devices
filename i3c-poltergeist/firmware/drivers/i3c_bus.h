/*
 * i3c_bus.h - I3C Poltergeist bus simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef I3C_POLTERGEIST_I3C_BUS_H
#define I3C_POLTERGEIST_I3C_BUS_H

#include "../board.h"

void ip_i3c_init_targets(ip_system_t *sys);
void ip_i3c_run_inventory(ip_system_t *sys);
void ip_i3c_print_inventory(const ip_system_t *sys);
void ip_i3c_capture_boot_sequence(ip_system_t *sys);
uint16_t ip_i3c_legacy_read(ip_system_t *sys, uint8_t target_index, uint16_t reg);
int ip_i3c_legacy_write(ip_system_t *sys, const ip_profile_t *profile, uint8_t target_index, uint16_t reg, uint16_t value);
int ip_i3c_inject_hotjoin(ip_system_t *sys, const ip_profile_t *profile, uint8_t target_index);
int ip_i3c_replay_ibi(ip_system_t *sys, const ip_profile_t *profile, uint8_t target_index, uint16_t vector);
int ip_i3c_suppress_ccc(ip_system_t *sys, const ip_profile_t *profile, uint16_t ccc);
void ip_i3c_describe_capture(const ip_system_t *sys, char *buffer, size_t buffer_size);
const char *ip_i3c_ccc_name(uint16_t ccc);

#endif
