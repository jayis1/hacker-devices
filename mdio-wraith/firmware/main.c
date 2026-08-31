/*
 * main.c - MDIO Wraith firmware simulator
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "registers.h"
#include "drivers/mdio_bus.h"
#include "drivers/profile.h"
#include "drivers/telemetry.h"
#include "drivers/radio.h"
#include "drivers/safety.h"

static void mw_print_banner(void)
{
    printf("%s firmware simulator by %s\n", MW_DEVICE_NAME, MW_AUTHOR);
    printf("Version: %s\n", MW_FIRMWARE_VERSION);
    printf("Authorized use only. Hardware assessment and red-team research require explicit written permission.\n\n");
}

static void mw_log_event(mw_system_t *sys,
                         mw_event_code_t code,
                         mw_channel_t channel,
                         mw_risk_t risk,
                         const char *message)
{
    size_t index;

    if (sys == NULL || sys->event_count >= MW_MAX_EVENTS) {
        return;
    }

    index = sys->event_count++;
    sys->events[index].timestamp_ms = sys->tick_ms;
    sys->events[index].code = code;
    sys->events[index].channel = channel;
    sys->events[index].risk = risk;
    (void)snprintf(sys->events[index].message,
                   sizeof(sys->events[index].message),
                   "%s",
                   message != NULL ? message : "event");
}

static void mw_set_profile(mw_system_t *sys, const mw_profile_t *profile)
{
    char desc[MW_MAX_MESSAGE];

    if (sys == NULL || profile == NULL) {
        return;
    }

    sys->status.writes_applied = 0u;
    sys->status.trigger_seen = 0u;
    (void)snprintf(sys->status.active_profile, sizeof(sys->status.active_profile), "%s", profile->name);
    mw_profile_describe(profile, desc, sizeof(desc));
    mw_log_event(sys, MW_EVENT_PROFILE_LOAD, MW_CHANNEL_CONTROL, MW_RISK_LOW, desc);
}

static void mw_arm_if_allowed(mw_system_t *sys, const mw_profile_t *profile)
{
    char reason[64];
    char message[MW_MAX_MESSAGE];

    if (sys == NULL || profile == NULL) {
        return;
    }

    if (mw_safety_can_arm(sys, profile, reason, sizeof(reason))) {
        sys->status.armed = profile->require_arming ? 1u : 0u;
        sys->status.mode = profile->require_arming ? MW_MODE_GUARDED : MW_MODE_PASSIVE;
        (void)snprintf(message, sizeof(message), "arming-check:%s", reason);
        mw_log_event(sys, MW_EVENT_SAFETY, MW_CHANNEL_CONTROL, MW_RISK_LOW, message);
    } else {
        sys->status.armed = 0u;
        sys->status.mode = MW_MODE_PASSIVE;
        (void)snprintf(message, sizeof(message), "arming-blocked:%s", reason);
        mw_log_event(sys, MW_EVENT_GUARD_BLOCK, MW_CHANNEL_CONTROL, MW_RISK_MEDIUM, message);
    }
}

static void mw_run_profile_phy_fingerprint(mw_system_t *sys, const mw_profile_t *profile)
{
    size_t i;
    char summary[320];

    (void)profile;

    mw_mdio_scan(sys);
    for (i = 0u; i < sys->phy_count; ++i) {
        const mw_phy_t *phy = &sys->phys[i];
        sys->tick_ms += MW_TICK_MS;
        (void)mw_mdio_read(sys, phy->phy_addr, 0u, MW_REG_BMCR, 0u);
        sys->tick_ms += MW_TICK_MS;
        (void)mw_mdio_read(sys, phy->phy_addr, 0u, MW_REG_BMSR, 0u);
        sys->tick_ms += MW_TICK_MS;
        if (phy->has_clause45) {
            (void)mw_mdio_read(sys, phy->phy_addr, MW_DEVAD_AUTONEG, 0x8000u, 1u);
        }
        mw_mdio_snapshot_vendor(sys, phy->phy_addr);
    }

    mw_mdio_capture_summary(sys, summary, sizeof(summary));
    printf("[profile] %s\n", profile->name);
    printf("[capture] %s\n\n", summary);
}

static void mw_run_profile_isolated_link_drop(mw_system_t *sys, const mw_profile_t *profile)
{
    uint16_t bmcr;
    char summary[320];

    printf("[profile] %s\n", profile->name);
    mw_mdio_scan(sys);
    sys->tick_ms += MW_TICK_MS;
    (void)mw_mdio_read(sys, 0u, 0u, MW_REG_BMSR, 0u);
    sys->tick_ms += MW_TICK_MS;
    bmcr = mw_mdio_read(sys, 0u, 0u, MW_REG_BMCR, 0u);
    sys->tick_ms += MW_TICK_MS;
    (void)mw_mdio_write(sys, profile, 0u, 0u, MW_REG_BMCR, (uint16_t)(bmcr | MW_BMCR_ISOLATE), 0u);
    sys->status.mode = MW_MODE_ACTIVE;
    sys->tick_ms += profile->settle_time_ms;
    (void)mw_mdio_read(sys, 0u, 0u, MW_REG_BMSR, 0u);
    mw_mdio_capture_summary(sys, summary, sizeof(summary));
    printf("[capture] %s\n\n", summary);
}

static void mw_run_profile_loopback_diversion(mw_system_t *sys, const mw_profile_t *profile)
{
    uint16_t bmcr;
    char hb[256];

    printf("[profile] %s\n", profile->name);
    mw_mdio_scan(sys);
    sys->tick_ms += MW_TICK_MS;
    (void)mw_mdio_read(sys, 1u, 0u, MW_REG_ANER, 0u);
    sys->tick_ms += MW_TICK_MS;
    bmcr = mw_mdio_read(sys, 1u, 0u, MW_REG_BMCR, 0u);
    sys->tick_ms += MW_TICK_MS;
    (void)mw_mdio_write(sys,
                        profile,
                        1u,
                        0u,
                        MW_REG_BMCR,
                        (uint16_t)(bmcr | MW_BMCR_LOOPBACK | MW_BMCR_RESTART_AUTONEG),
                        0u);
    sys->status.mode = MW_MODE_ACTIVE;
    sys->tick_ms += profile->settle_time_ms;
    (void)mw_mdio_read(sys, 1u, MW_DEVAD_AUTONEG, 0x8000u, 1u);
    mw_radio_build_heartbeat(sys, hb, sizeof(hb));
    printf("[heartbeat] %s\n\n", hb);
}

static void mw_run_profile_strap_shadow(mw_system_t *sys, const mw_profile_t *profile)
{
    char export_frame[700];

    printf("[profile] %s\n", profile->name);
    mw_mdio_scan(sys);
    sys->tick_ms += MW_TICK_MS;
    (void)mw_mdio_read(sys, 2u, 0u, MW_REG_VENDOR_PAGE, 0u);
    sys->tick_ms += MW_TICK_MS;
    (void)mw_mdio_write(sys, profile, 2u, 0u, MW_REG_VENDOR_PAGE, 0x0044u, 0u);
    sys->status.mode = MW_MODE_ACTIVE;
    sys->tick_ms += profile->settle_time_ms;
    mw_mdio_snapshot_vendor(sys, 2u);
    mw_radio_build_capture_export(sys, export_frame, sizeof(export_frame), 8u);
    printf("[export] %s\n\n", export_frame);
}

static void mw_print_phys(const mw_system_t *sys)
{
    size_t i;

    if (sys == NULL) {
        return;
    }

    printf("[phys]\n");
    for (i = 0u; i < sys->phy_count; ++i) {
        const mw_phy_t *phy = &sys->phys[i];
        printf("  phy=%u label=%s clause45=%u link=%u bmcr=0x%04X bmsr=0x%04X\n",
               (unsigned int)phy->phy_addr,
               phy->phy_label,
               (unsigned int)phy->has_clause45,
               (unsigned int)phy->link_up,
               (unsigned int)phy->bmcr,
               (unsigned int)phy->bmsr);
    }
    printf("\n");
}

static void mw_print_events(const mw_system_t *sys)
{
    size_t i;

    if (sys == NULL) {
        return;
    }

    printf("[events] count=%u\n", (unsigned int)sys->event_count);
    for (i = 0u; i < sys->event_count; ++i) {
        const mw_event_t *event = &sys->events[i];
        printf("  t=%u code=%u channel=%u risk=%u %s\n",
               (unsigned int)event->timestamp_ms,
               (unsigned int)event->code,
               (unsigned int)event->channel,
               (unsigned int)event->risk,
               event->message);
    }
    printf("\n");
}

static void mw_print_status(const mw_system_t *sys)
{
    char telemetry[256];
    char json[1024];

    if (sys == NULL) {
        return;
    }

    mw_telemetry_report(sys, telemetry, sizeof(telemetry));
    mw_telemetry_render_json(sys, json, sizeof(json));

    printf("[status] %s\n", telemetry);
    printf("[json] %s\n\n", json);
}

static void mw_execute_profile(mw_system_t *sys, const mw_profile_t *profile)
{
    if (sys == NULL || profile == NULL) {
        return;
    }

    mw_set_profile(sys, profile);
    mw_arm_if_allowed(sys, profile);

    if (strcmp(profile->name, "phy-fingerprint") == 0) {
        mw_run_profile_phy_fingerprint(sys, profile);
    } else if (strcmp(profile->name, "isolated-link-drop") == 0) {
        mw_run_profile_isolated_link_drop(sys, profile);
    } else if (strcmp(profile->name, "loopback-diversion") == 0) {
        mw_run_profile_loopback_diversion(sys, profile);
    } else if (strcmp(profile->name, "strap-shadow") == 0) {
        mw_run_profile_strap_shadow(sys, profile);
    }

    mw_telemetry_update(sys, sys->tick_ms / MW_TICK_MS + sys->status.writes_applied);
    mw_safety_tick(sys);
    mw_print_status(sys);
    mw_print_events(sys);
}

int main(void)
{
    mw_system_t sys;
    size_t i;

    (void)memset(&sys, 0, sizeof(sys));
    mw_print_banner();
    mw_telemetry_seed(&sys);
    mw_safety_init(&sys, mw_profile_get(0u));
    mw_mdio_init(&sys);
    mw_print_phys(&sys);

    for (i = 0u; i < mw_profile_count(); ++i) {
        const mw_profile_t *profile = mw_profile_get(i);
        mw_execute_profile(&sys, profile);
        sys.tick_ms += 50u;
    }

    mw_safety_force_bypass(&sys, "simulation-complete");
    mw_print_status(&sys);
    printf("Simulation complete. Review captures before adapting to bench hardware.\n");
    return 0;
}
