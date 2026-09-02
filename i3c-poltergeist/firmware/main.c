/*
 * main.c - I3C Poltergeist firmware simulator
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "registers.h"
#include "drivers/i3c_bus.h"
#include "drivers/timing_fabric.h"
#include "drivers/policy.h"
#include "drivers/telemetry.h"
#include "drivers/radio.h"

static void ip_log_event(ip_system_t *sys, ip_event_code_t code, ip_risk_t risk, const char *message)
{
    size_t slot;

    if (sys == NULL || sys->event_count >= IP_MAX_EVENTS) {
        return;
    }

    slot = sys->event_count++;
    sys->events[slot].timestamp_ms = sys->tick_ms;
    sys->events[slot].code = code;
    sys->events[slot].risk = risk;
    (void)snprintf(sys->events[slot].message,
                   sizeof(sys->events[slot].message),
                   "%s",
                   message != NULL ? message : "event");
}

static void ip_banner(void)
{
    printf("%s firmware simulator by %s\n", IP_DEVICE_NAME, IP_AUTHOR);
    printf("Version: %s\n", IP_FIRMWARE_VERSION);
    printf("Authorized use only. Hardware security assessment requires explicit written permission.\n\n");
}

static void ip_boot(ip_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    (void)memset(sys, 0, sizeof(*sys));
    sys->status.mode = IP_MODE_OBSERVE;
    sys->status.board_temp_c = 38.4f;
    sys->status.target_current_ma = 71.0f;
    sys->status.target_voltage_v = 1.80f;
    ip_i3c_init_targets(sys);
    ip_fabric_init(sys);
    ip_radio_init(sys);
    ip_policy_refresh_telemetry(sys);
    ip_log_event(sys, IP_EVENT_BOOT, IP_RISK_LOW, "boot-complete");
}

static void ip_print_events(const ip_system_t *sys)
{
    size_t i;

    if (sys == NULL) {
        return;
    }

    printf("[events] count=%u\n", (unsigned int)sys->event_count);
    for (i = 0u; i < sys->event_count; ++i) {
        printf("  t=%u code=%u risk=%u msg=%s\n",
               (unsigned int)sys->events[i].timestamp_ms,
               (unsigned int)sys->events[i].code,
               (unsigned int)sys->events[i].risk,
               sys->events[i].message);
    }
    printf("\n");
}

static void ip_print_status(const ip_system_t *sys)
{
    char status_line[256];
    char status_json[768];
    char fabric_health[256];
    char heartbeat[256];

    if (sys == NULL) {
        return;
    }

    ip_telemetry_status_line(sys, status_line, sizeof(status_line));
    ip_telemetry_render_json(sys, status_json, sizeof(status_json));
    ip_fabric_record_health((ip_system_t *)sys, fabric_health, sizeof(fabric_health));
    ip_radio_heartbeat(sys, heartbeat, sizeof(heartbeat));

    printf("[status] %s\n", status_line);
    printf("[fabric] %s\n", fabric_health);
    printf("[heartbeat] %s\n", heartbeat);
    printf("[json] %s\n\n", status_json);
}

static void ip_passive_inventory(ip_system_t *sys, const ip_profile_t *profile)
{
    char capture_summary[256];

    if (sys == NULL || profile == NULL) {
        return;
    }

    ip_policy_load(sys, profile);
    ip_log_event(sys, IP_EVENT_PROFILE_LOAD, IP_RISK_LOW, profile->name);
    ip_i3c_capture_boot_sequence(sys);
    ip_log_event(sys, IP_EVENT_DISCOVERY, IP_RISK_LOW, "boot-sequence-captured");
    ip_i3c_run_inventory(sys);
    ip_log_event(sys, IP_EVENT_DISCOVERY, IP_RISK_LOW, "inventory-finished");
    ip_policy_refresh_telemetry(sys);
    ip_i3c_describe_capture(sys, capture_summary, sizeof(capture_summary));
    printf("[profile] %s\n", profile->name);
    printf("[capture] %s\n\n", capture_summary);
}

static void ip_run_downgrade_probe(ip_system_t *sys, const ip_profile_t *profile)
{
    char reason[64];
    uint16_t id0;
    uint16_t id3;

    if (sys == NULL || profile == NULL) {
        return;
    }

    ip_policy_load(sys, profile);
    ip_fabric_apply_profile(sys, profile);
    ip_log_event(sys, IP_EVENT_PROFILE_LOAD, IP_RISK_LOW, profile->name);

    if (!ip_policy_arm(sys, profile, reason, sizeof(reason))) {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
        return;
    }
    ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_LOW, reason);

    id0 = ip_i3c_legacy_read(sys, 0u, IP_REG_DEVICE_ID);
    id3 = ip_i3c_legacy_read(sys, 3u, IP_REG_DEVICE_ID);
    ip_log_event(sys, IP_EVENT_DOWNGRADE, IP_RISK_MEDIUM, "legacy-read-window-measured");

    if (ip_policy_check_write(sys, profile, "legacy-write-touch-bridge", reason, sizeof(reason))) {
        if (ip_i3c_legacy_write(sys, profile, 1u, IP_REG_MAILBOX, 0x51A5u)) {
            ip_policy_consume_write(sys);
            ip_log_event(sys, IP_EVENT_WRITE, IP_RISK_MEDIUM, "touch-bridge-mailbox-write");
        }
    } else {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
    }

    if (ip_policy_check_write(sys, profile, "legacy-write-ec", reason, sizeof(reason))) {
        if (ip_i3c_legacy_write(sys, profile, 3u, IP_REG_DEBUG_WINDOW, 0x2201u)) {
            ip_policy_consume_write(sys);
            ip_log_event(sys, IP_EVENT_WRITE, IP_RISK_HIGH, "ec-debug-window-write");
        }
    } else {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
    }

    printf("[profile] %s\n", profile->name);
    printf("[downgrade] imu-id=0x%04X ec-id=0x%04X remaining-budget=%u\n\n",
           (unsigned int)id0,
           (unsigned int)id3,
           (unsigned int)sys->status.remaining_write_budget);
}

static void ip_run_hotjoin_ghost(ip_system_t *sys, const ip_profile_t *profile)
{
    char reason[64];
    uint32_t delay_ns;

    if (sys == NULL || profile == NULL) {
        return;
    }

    ip_policy_load(sys, profile);
    ip_fabric_apply_profile(sys, profile);
    ip_log_event(sys, IP_EVENT_PROFILE_LOAD, IP_RISK_LOW, profile->name);

    if (!ip_policy_arm(sys, profile, reason, sizeof(reason))) {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
        return;
    }

    delay_ns = ip_fabric_estimate_delay_ns(profile, IP_CCC_ENTDAA, profile->target_selector);
    ip_log_event(sys, IP_EVENT_FABRIC, IP_RISK_LOW, "fabric-jitter-applied");

    if (ip_policy_check_write(sys, profile, "inject-hotjoin", reason, sizeof(reason)) &&
        ip_i3c_inject_hotjoin(sys, profile, profile->target_selector)) {
        ip_policy_consume_write(sys);
        ip_log_event(sys, IP_EVENT_HOTJOIN, IP_RISK_HIGH, "synthetic-hotjoin-emitted");
    } else {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
    }

    printf("[profile] %s\n", profile->name);
    printf("[hotjoin] target=%u delay=%uns seen=%u\n\n",
           (unsigned int)profile->target_selector,
           (unsigned int)delay_ns,
           (unsigned int)sys->status.hotjoin_seen);
}

static void ip_run_ibi_shadow(ip_system_t *sys, const ip_profile_t *profile)
{
    char reason[64];
    uint16_t vector;

    if (sys == NULL || profile == NULL) {
        return;
    }

    ip_policy_load(sys, profile);
    ip_fabric_apply_profile(sys, profile);
    ip_log_event(sys, IP_EVENT_PROFILE_LOAD, IP_RISK_LOW, profile->name);

    if (!ip_policy_arm(sys, profile, reason, sizeof(reason))) {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
        return;
    }

    vector = (uint16_t)(0x9000u | profile->target_selector);
    if (ip_policy_check_write(sys, profile, "replay-ibi", reason, sizeof(reason)) &&
        ip_i3c_replay_ibi(sys, profile, profile->target_selector, vector)) {
        ip_policy_consume_write(sys);
        ip_log_event(sys, IP_EVENT_IBI, IP_RISK_HIGH, "ibi-replay-emitted");
    } else {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
    }

    printf("[profile] %s\n", profile->name);
    printf("[ibi] target=%u vector=0x%04X seen=%u\n\n",
           (unsigned int)profile->target_selector,
           (unsigned int)vector,
           (unsigned int)sys->status.ibi_seen);
}

static void ip_run_ccc_eclipse(ip_system_t *sys, const ip_profile_t *profile)
{
    char reason[64];

    if (sys == NULL || profile == NULL) {
        return;
    }

    ip_policy_load(sys, profile);
    ip_fabric_apply_profile(sys, profile);
    ip_log_event(sys, IP_EVENT_PROFILE_LOAD, IP_RISK_LOW, profile->name);

    if (!ip_policy_arm(sys, profile, reason, sizeof(reason))) {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
        return;
    }

    if (ip_fabric_should_mirror_ccc(profile, profile->trigger_ccc, 0u)) {
        ip_log_event(sys, IP_EVENT_FABRIC, IP_RISK_MEDIUM, "ccc-mirror-window-open");
    }

    if (ip_policy_check_write(sys, profile, "suppress-ccc", reason, sizeof(reason)) &&
        ip_i3c_suppress_ccc(sys, profile, profile->trigger_ccc)) {
        ip_policy_consume_write(sys);
        ip_log_event(sys, IP_EVENT_CCC, IP_RISK_HIGH, "ccc-suppressed-once");
    } else {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_MEDIUM, reason);
    }

    printf("[profile] %s\n", profile->name);
    printf("[ccc] suppressed=%s remaining-budget=%u\n\n",
           ip_i3c_ccc_name(profile->trigger_ccc),
           (unsigned int)sys->status.remaining_write_budget);
}

static void ip_rollback_if_needed(ip_system_t *sys)
{
    char reason[64];

    if (sys == NULL) {
        return;
    }

    ip_policy_refresh_telemetry(sys);
    if (ip_policy_should_rollback(sys, reason, sizeof(reason))) {
        ip_policy_force_bypass(sys, reason);
        ip_log_event(sys, IP_EVENT_ROLLBACK, IP_RISK_HIGH, reason);
    } else {
        ip_log_event(sys, IP_EVENT_POLICY, IP_RISK_LOW, reason);
    }
}

static void ip_export(const ip_system_t *sys)
{
    char events_json[2048];
    char frame[IP_MAX_EXPORT];

    if (sys == NULL) {
        return;
    }

    ip_telemetry_render_events(sys, events_json, sizeof(events_json));
    ip_radio_export_frame(sys, frame, sizeof(frame), 8u);
    printf("[events-json] %s\n", events_json);
    printf("[export] %s\n\n", frame);
}

int main(void)
{
    ip_system_t sys;
    size_t count;
    const ip_profile_t *profiles;

    ip_banner();
    ip_boot(&sys);
    ip_i3c_print_inventory(&sys);

    profiles = ip_policy_catalog(&count);
    if (count < 5u) {
        printf("profile-catalog-incomplete\n");
        return 1;
    }

    ip_passive_inventory(&sys, &profiles[0]);
    ip_run_downgrade_probe(&sys, &profiles[1]);
    ip_run_hotjoin_ghost(&sys, &profiles[2]);
    ip_run_ibi_shadow(&sys, &profiles[3]);
    ip_run_ccc_eclipse(&sys, &profiles[4]);
    ip_rollback_if_needed(&sys);
    ip_print_status(&sys);
    ip_print_events(&sys);
    ip_export(&sys);

    return 0;
}
