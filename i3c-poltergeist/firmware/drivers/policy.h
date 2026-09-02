/*
 * policy.h - I3C Poltergeist safety and write policy
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef I3C_POLTERGEIST_POLICY_H
#define I3C_POLTERGEIST_POLICY_H

#include "../board.h"

const ip_profile_t *ip_policy_catalog(size_t *count);
void ip_policy_load(ip_system_t *sys, const ip_profile_t *profile);
int ip_policy_arm(ip_system_t *sys, const ip_profile_t *profile, char *reason, size_t reason_size);
int ip_policy_check_write(ip_system_t *sys, const ip_profile_t *profile, const char *action, char *reason, size_t reason_size);
void ip_policy_consume_write(ip_system_t *sys);
void ip_policy_refresh_telemetry(ip_system_t *sys);
int ip_policy_should_rollback(ip_system_t *sys, char *reason, size_t reason_size);
void ip_policy_force_bypass(ip_system_t *sys, const char *reason);

#endif
