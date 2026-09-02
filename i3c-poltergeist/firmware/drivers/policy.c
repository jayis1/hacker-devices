/*
 * policy.c - I3C Poltergeist safety and write policy
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "policy.h"
#include "../registers.h"

static const ip_profile_t g_profiles[] = {
    {
        "inventory-passive", 0u, 0u, 0u, 0u, 0u, 0u, IP_TARGET_ANY, 2u, 40u, IP_TRIGGER_ANY_CCC
    },
    {
        "downgrade-probe", 1u, 0u, 0u, 0u, 1u, 2u, 3u, 6u, 80u, IP_CCC_GETPID
    },
    {
        "hotjoin-ghost", 0u, 1u, 0u, 0u, 1u, 1u, 4u, 8u, 100u, IP_CCC_ENTDAA
    },
    {
        "ibi-shadow", 0u, 0u, 1u, 0u, 1u, 1u, 1u, 10u, 80u, IP_CCC_ENEC
    },
    {
        "ccc-eclipse", 0u, 0u, 0u, 1u, 1u, 1u, IP_TARGET_ANY, 14u, 60u, IP_CCC_SETMRL
    }
};

const ip_profile_t *ip_policy_catalog(size_t *count)
{
    if (count != NULL) {
        *count = sizeof(g_profiles) / sizeof(g_profiles[0]);
    }
    return g_profiles;
}

void ip_policy_load(ip_system_t *sys, const ip_profile_t *profile)
{
    if (sys == NULL || profile == NULL) {
        return;
    }

    (void)snprintf(sys->status.active_profile,
                   sizeof(sys->status.active_profile),
                   "%s",
                   profile->name);
    sys->status.remaining_write_budget = profile->write_budget;
    sys->status.armed = 0u;
    sys->status.mode = IP_MODE_OBSERVE;
    sys->status.bypass_engaged = 0u;
}

int ip_policy_arm(ip_system_t *sys, const ip_profile_t *profile, char *reason, size_t reason_size)
{
    if (reason != NULL && reason_size > 0u) {
        reason[0] = '\0';
    }

    if (sys == NULL || profile == NULL) {
        if (reason != NULL && reason_size > 0u) {
            (void)snprintf(reason, reason_size, "%s", "invalid-context");
        }
        return 0;
    }

    if (sys->status.bypass_engaged) {
        (void)snprintf(reason, reason_size, "%s", "bypass-engaged");
        return 0;
    }
    if (!sys->status.target_present) {
        (void)snprintf(reason, reason_size, "%s", "no-target");
        return 0;
    }
    if (sys->status.board_temp_c > IP_TEMP_LIMIT_C) {
        (void)snprintf(reason, reason_size, "%s", "over-temp");
        return 0;
    }
    if (sys->status.target_current_ma > IP_CURRENT_LIMIT_MA) {
        (void)snprintf(reason, reason_size, "%s", "over-current");
        return 0;
    }
    if (sys->status.target_voltage_v < IP_VOLTAGE_MIN_V || sys->status.target_voltage_v > IP_VOLTAGE_MAX_V) {
        (void)snprintf(reason, reason_size, "%s", "rail-out-of-range");
        return 0;
    }

    sys->status.armed = profile->require_arming ? 1u : 0u;
    sys->status.mode = profile->require_arming ? IP_MODE_GUARDED : IP_MODE_OBSERVE;
    (void)snprintf(reason, reason_size, "%s", profile->require_arming ? "armed" : "observe-only");
    return 1;
}

int ip_policy_check_write(ip_system_t *sys, const ip_profile_t *profile, const char *action, char *reason, size_t reason_size)
{
    (void)action;

    if (reason != NULL && reason_size > 0u) {
        reason[0] = '\0';
    }

    if (sys == NULL || profile == NULL) {
        (void)snprintf(reason, reason_size, "%s", "invalid-context");
        return 0;
    }
    if (sys->status.bypass_engaged) {
        (void)snprintf(reason, reason_size, "%s", "bypass-engaged");
        return 0;
    }
    if (profile->require_arming && !sys->status.armed) {
        (void)snprintf(reason, reason_size, "%s", "not-armed");
        return 0;
    }
    if (sys->status.remaining_write_budget == 0u) {
        (void)snprintf(reason, reason_size, "%s", "write-budget-exhausted");
        return 0;
    }
    if (sys->status.board_temp_c > IP_TEMP_LIMIT_C) {
        (void)snprintf(reason, reason_size, "%s", "over-temp");
        return 0;
    }
    if (sys->status.target_current_ma > IP_CURRENT_LIMIT_MA) {
        (void)snprintf(reason, reason_size, "%s", "over-current");
        return 0;
    }
    if (sys->status.target_voltage_v < IP_VOLTAGE_MIN_V || sys->status.target_voltage_v > IP_VOLTAGE_MAX_V) {
        (void)snprintf(reason, reason_size, "%s", "rail-out-of-range");
        return 0;
    }

    sys->status.mode = IP_MODE_ACTIVE;
    (void)snprintf(reason, reason_size, "%s", "write-approved");
    return 1;
}

void ip_policy_consume_write(ip_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    if (sys->status.remaining_write_budget > 0u) {
        sys->status.remaining_write_budget--;
    }
}

void ip_policy_refresh_telemetry(ip_system_t *sys)
{
    static const float temps[] = {38.4f, 40.1f, 42.7f, 45.2f, 47.8f, 49.1f};
    static const float currents[] = {71.0f, 86.4f, 97.8f, 122.2f, 138.5f, 149.7f};
    static const float volts[] = {1.80f, 1.79f, 1.81f, 1.78f, 1.80f, 1.79f};
    size_t slot;

    if (sys == NULL) {
        return;
    }

    slot = (sys->tick_ms / IP_TICK_MS) % (sizeof(temps) / sizeof(temps[0]));
    sys->status.board_temp_c = temps[slot] + (float)sys->status.anomalies * 0.4f;
    sys->status.target_current_ma = currents[slot] + (float)(sys->capture_count % 5u) * 2.0f;
    sys->status.target_voltage_v = volts[slot];
}

int ip_policy_should_rollback(ip_system_t *sys, char *reason, size_t reason_size)
{
    if (reason != NULL && reason_size > 0u) {
        reason[0] = '\0';
    }

    if (sys == NULL) {
        (void)snprintf(reason, reason_size, "%s", "invalid-context");
        return 1;
    }

    if (sys->status.board_temp_c > IP_TEMP_LIMIT_C) {
        (void)snprintf(reason, reason_size, "%s", "rollback-over-temp");
        return 1;
    }
    if (sys->status.target_current_ma > IP_CURRENT_LIMIT_MA) {
        (void)snprintf(reason, reason_size, "%s", "rollback-over-current");
        return 1;
    }
    if (sys->status.target_voltage_v < IP_VOLTAGE_MIN_V || sys->status.target_voltage_v > IP_VOLTAGE_MAX_V) {
        (void)snprintf(reason, reason_size, "%s", "rollback-rail-out-of-range");
        return 1;
    }
    if (!sys->status.fabric_locked) {
        (void)snprintf(reason, reason_size, "%s", "rollback-fabric-drift");
        return 1;
    }
    if (sys->status.anomalies > 5u) {
        (void)snprintf(reason, reason_size, "%s", "rollback-anomaly-budget");
        return 1;
    }

    (void)snprintf(reason, reason_size, "%s", "stable");
    return 0;
}

void ip_policy_force_bypass(ip_system_t *sys, const char *reason)
{
    if (sys == NULL) {
        return;
    }

    sys->status.mode = IP_MODE_BYPASS;
    sys->status.bypass_engaged = 1u;
    sys->status.armed = 0u;
    if (reason != NULL && reason[0] != '\0') {
        sys->status.anomalies++;
    }
}
