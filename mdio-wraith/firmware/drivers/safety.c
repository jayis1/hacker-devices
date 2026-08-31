/*
 * drivers/safety.c - MDIO Wraith safety controls
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "safety.h"

#include <stdio.h>
#include <string.h>

#include "../registers.h"

void mw_safety_init(mw_system_t *sys, const mw_profile_t *profile)
{
    if (sys == NULL) {
        return;
    }

    sys->status.mode = MW_MODE_PASSIVE;
    sys->status.armed = 0u;
    sys->status.watchdog_ok = 1u;
    sys->status.target_present = 1u;
    sys->status.fail_safe_engaged = 0u;
    sys->status.trigger_seen = 0u;
    sys->status.writes_applied = 0u;
    sys->status.anomalies = 0u;
    sys->status.discovered_phys = 0u;
    (void)snprintf(sys->status.active_profile, sizeof(sys->status.active_profile), "%s", profile != NULL ? profile->name : "none");
}

int mw_safety_can_arm(const mw_system_t *sys, const mw_profile_t *profile, char *reason, size_t length)
{
    if (reason != NULL && length > 0u) {
        reason[0] = '\0';
    }

    if (sys == NULL || profile == NULL) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "missing-system-or-profile");
        }
        return 0;
    }

    if (!profile->require_arming) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "profile-does-not-require-arming");
        }
        return 1;
    }

    if (!sys->status.target_present) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "target-not-present");
        }
        return 0;
    }

    if (sys->status.board_temp_c > MW_TEMP_LIMIT_C) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "thermal-limit");
        }
        return 0;
    }

    if (sys->status.target_current_ma > MW_CURRENT_LIMIT_MA) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "current-limit");
        }
        return 0;
    }

    if (sys->status.target_voltage_v < MW_VOLTAGE_MIN_V || sys->status.target_voltage_v > MW_VOLTAGE_MAX_V) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "voltage-out-of-range");
        }
        return 0;
    }

    if (!sys->status.watchdog_ok) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "watchdog-degraded");
        }
        return 0;
    }

    if (reason != NULL && length > 0u) {
        (void)snprintf(reason, length, "armed-ok");
    }
    return 1;
}

int mw_safety_can_write(mw_system_t *sys, const mw_profile_t *profile, uint16_t reg, uint16_t value, char *reason, size_t length)
{
    if (reason != NULL && length > 0u) {
        reason[0] = '\0';
    }

    if (sys == NULL || profile == NULL) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "missing-context");
        }
        return 0;
    }

    if (profile->require_arming && !sys->status.armed) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "profile-not-armed");
        }
        return 0;
    }

    if (sys->status.writes_applied >= profile->write_budget) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "write-budget-exhausted");
        }
        return 0;
    }

    if ((reg == MW_REG_BMCR) && ((value & MW_BMCR_POWER_DOWN) != 0u)) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "power-down-blocked");
        }
        return 0;
    }

    if ((reg == MW_REG_BMCR) && ((value & MW_BMCR_ISOLATE) != 0u) && !profile->allow_isolate_write) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "isolate-write-disallowed");
        }
        return 0;
    }

    if ((reg == MW_REG_BMCR) && ((value & MW_BMCR_LOOPBACK) != 0u) && !profile->allow_loopback_write) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "loopback-write-disallowed");
        }
        return 0;
    }

    if (sys->status.board_temp_c > MW_TEMP_LIMIT_C) {
        if (reason != NULL && length > 0u) {
            (void)snprintf(reason, length, "thermal-block");
        }
        return 0;
    }

    if (reason != NULL && length > 0u) {
        (void)snprintf(reason, length, "write-approved");
    }
    return 1;
}

void mw_safety_tick(mw_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    if (sys->status.board_temp_c > MW_TEMP_LIMIT_C ||
        sys->status.target_current_ma > MW_CURRENT_LIMIT_MA ||
        sys->status.target_voltage_v < MW_VOLTAGE_MIN_V) {
        sys->status.fail_safe_engaged = 1u;
        sys->status.armed = 0u;
        sys->status.mode = MW_MODE_BYPASS;
        sys->status.watchdog_ok = 0u;
        ++sys->status.anomalies;
    }
}

void mw_safety_force_bypass(mw_system_t *sys, const char *reason)
{
    size_t index;

    if (sys == NULL) {
        return;
    }

    sys->status.fail_safe_engaged = 1u;
    sys->status.armed = 0u;
    sys->status.mode = MW_MODE_BYPASS;
    sys->status.watchdog_ok = 0u;

    if (sys->event_count < MW_MAX_EVENTS) {
        index = sys->event_count++;
        sys->events[index].timestamp_ms = sys->tick_ms;
        sys->events[index].code = MW_EVENT_ROLLBACK;
        sys->events[index].channel = MW_CHANNEL_CONTROL;
        sys->events[index].risk = MW_RISK_HIGH;
        (void)snprintf(sys->events[index].message,
                       sizeof(sys->events[index].message),
                       "rollback:%s",
                       reason != NULL ? reason : "operator");
    }
}
