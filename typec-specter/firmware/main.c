/*
 * Type-C Specter reference firmware simulation
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/pd_phy.h"
#include "drivers/cc_mux.h"
#include "drivers/vbus_meter.h"
#include "drivers/script_engine.h"
#include "drivers/radio.h"

static ts_runtime_t g_runtime;
static ts_event_t g_log[TS_EVENT_LOG_CAPACITY];
static uint32_t g_log_head = 0;
static uint32_t g_now_ms = 0;

static void log_event(ts_event_type_t type, uint32_t param0, uint32_t param1, const char *text) {
    ts_event_t *slot = &g_log[g_log_head % TS_EVENT_LOG_CAPACITY];
    slot->timestamp_ms = g_now_ms;
    slot->type = type;
    slot->param0 = param0;
    slot->param1 = param1;
    snprintf(slot->text, sizeof(slot->text), "%s", text ? text : "");
    g_log_head++;
}

static void runtime_defaults(void) {
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.flags = TS_FLAG_PASSIVE_MONITOR | TS_FLAG_SAFE_MODE;
    snprintf(g_runtime.active_profile, sizeof(g_runtime.active_profile), "transparent-proxy");
    snprintf(g_runtime.scenario.name, sizeof(g_runtime.scenario.name), "idle");
    g_runtime.scenario.kind = TS_SCENARIO_IDLE;
}

static void board_init(void) {
    runtime_defaults();
    pd_phy_init();
    cc_mux_init();
    vbus_meter_init();
    radio_init();
    cc_mux_set_usb2_mode(TS_USB2_PASSTHROUGH);
    cc_mux_set_sbu_mode(TS_SBU_PASSTHROUGH);
    pd_phy_set_proxy_mode(true);
    pd_phy_set_mutation_enabled(false);
    log_event(TS_EVENT_BOOT, TS_FW_VERSION_MAJOR, TS_FW_VERSION_MINOR, "booted in passive mode");
}

static const char *role_name(ts_power_role_t role) {
    switch (role) {
        case TS_ROLE_SOURCE: return "source";
        case TS_ROLE_SINK: return "sink";
        case TS_ROLE_DRP: return "drp";
        case TS_ROLE_DEBUG_ACCESSORY: return "debug";
        default: return "none";
    }
}

static void print_status(void) {
    printf("[status] t=%ums flags=0x%02x target=%s peer=%s scenario=%s vbus=%umV ibus=%umA temp=%ldC usb2=%s sbu=%s\n",
           g_now_ms,
           g_runtime.flags,
           role_name(g_runtime.target_role),
           role_name(g_runtime.peer_role),
           g_runtime.scenario.name,
           g_runtime.power.target_vbus_mv,
           g_runtime.power.target_ibus_ma,
           (long)g_runtime.power.board_temp_c,
           cc_mux_usb2_mode_name(),
           cc_mux_sbu_mode_name());
}

static void update_attach_state(void) {
    const ts_attach_state_t attach = pd_phy_poll_attach();
    uint32_t prev_flags = g_runtime.flags;

    if (attach.target_present) {
        g_runtime.flags |= TS_FLAG_TARGET_PRESENT;
        g_runtime.target_role = attach.target_role;
    } else {
        g_runtime.flags &= ~TS_FLAG_TARGET_PRESENT;
        g_runtime.target_role = TS_ROLE_NONE;
    }

    if (attach.peer_present) {
        g_runtime.flags |= TS_FLAG_PEER_PRESENT;
        g_runtime.peer_role = attach.peer_role;
    } else {
        g_runtime.flags &= ~TS_FLAG_PEER_PRESENT;
        g_runtime.peer_role = TS_ROLE_NONE;
    }

    g_runtime.data_role = attach.data_role;

    if (((prev_flags ^ g_runtime.flags) & TS_FLAG_TARGET_PRESENT) != 0u) {
        log_event(attach.target_present ? TS_EVENT_ATTACH : TS_EVENT_DETACH,
                  attach.target_present,
                  g_runtime.target_role,
                  attach.target_present ? "target attach changed" : "target detached");
    }
    if (((prev_flags ^ g_runtime.flags) & TS_FLAG_PEER_PRESENT) != 0u) {
        log_event(attach.peer_present ? TS_EVENT_ATTACH : TS_EVENT_DETACH,
                  attach.peer_present,
                  g_runtime.peer_role,
                  attach.peer_present ? "peer attach changed" : "peer detached");
    }
}

static void update_power(void) {
    g_runtime.power = vbus_meter_sample();
    if (g_runtime.power.trip) {
        g_runtime.flags |= TS_FLAG_FAULTED | TS_FLAG_SAFE_MODE;
        g_runtime.flags &= ~TS_FLAG_MUTATION_ARMED;
        pd_phy_set_mutation_enabled(false);
        cc_mux_force_safe_path();
        script_engine_abort("power safety trip");
        log_event(TS_EVENT_FAULT, g_runtime.power.target_ibus_ma, (uint32_t)g_runtime.power.board_temp_c, "power safety trip");
    }
}

static void apply_script_actions(void) {
    ts_script_action_t action;
    while (script_engine_next_action(&g_runtime, g_now_ms, &action)) {
        switch (action.kind) {
            case TS_ACTION_ENABLE_MUTATION:
                g_runtime.flags |= TS_FLAG_MUTATION_ARMED;
                g_runtime.flags &= ~TS_FLAG_SAFE_MODE;
                pd_phy_set_mutation_enabled(true);
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, action.value0, "mutation enabled");
                break;
            case TS_ACTION_DISABLE_MUTATION:
                g_runtime.flags &= ~TS_FLAG_MUTATION_ARMED;
                pd_phy_set_mutation_enabled(false);
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, action.value0, "mutation disabled");
                break;
            case TS_ACTION_IDENTITY_PROFILE:
                pd_phy_load_identity_profile(action.text);
                snprintf(g_runtime.active_profile, sizeof(g_runtime.active_profile), "%.31s", action.text);
                log_event(TS_EVENT_PROFILE_LOAD, action.kind, 0, action.text);
                break;
            case TS_ACTION_REQUEST_ROLE_SWAP:
                pd_phy_request_role_swap((ts_power_role_t)action.value0);
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, action.value0, "role swap requested");
                break;
            case TS_ACTION_ASSERT_DEBUG_ACCESSORY:
                pd_phy_assert_debug_accessory(action.value0 != 0u);
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, action.value0, "debug accessory toggled");
                break;
            case TS_ACTION_LIMIT_CURRENT:
                vbus_meter_set_current_limit(action.value0);
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, action.value0, "current limit updated");
                break;
            case TS_ACTION_USB2_ISOLATE:
                cc_mux_set_usb2_mode(action.value0 ? TS_USB2_ISOLATE : TS_USB2_PASSTHROUGH);
                if (action.value0) g_runtime.flags |= TS_FLAG_USB2_ISOLATED; else g_runtime.flags &= ~TS_FLAG_USB2_ISOLATED;
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, action.value0, "usb2 isolation toggled");
                break;
            case TS_ACTION_SBU_ISOLATE:
                cc_mux_set_sbu_mode(action.value0 ? TS_SBU_ISOLATE : TS_SBU_PASSTHROUGH);
                if (action.value0) g_runtime.flags |= TS_FLAG_SBU_ISOLATED; else g_runtime.flags &= ~TS_FLAG_SBU_ISOLATED;
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, action.value0, "sbu isolation toggled");
                break;
            case TS_ACTION_HARD_RESET:
                pd_phy_send_hard_reset();
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, 0, "pd hard reset");
                break;
            case TS_ACTION_COMPLETE:
                g_runtime.scenario.active = false;
                g_runtime.scenario.completed = true;
                g_runtime.flags &= ~TS_FLAG_MUTATION_ARMED;
                pd_phy_set_mutation_enabled(false);
                log_event(TS_EVENT_SCENARIO_ACTION, action.kind, 0, "scenario complete");
                break;
            case TS_ACTION_NONE:
            default:
                break;
        }
    }
}

static void pump_pd_trace(void) {
    ts_pd_packet_t packet;
    while (pd_phy_receive_packet(&packet)) {
        char line[TS_STATUS_TEXT_LEN];
        snprintf(line, sizeof(line), "pd seq=%u hdr=0x%04x mutated=%u words=%u",
                 packet.seq, packet.header, packet.mutated ? 1u : 0u, packet.payload_len);
        log_event(TS_EVENT_PD_MESSAGE, packet.header, packet.payload_len, line);
    }
}

static void service_remote(void) {
    ts_radio_command_t cmd;
    while (radio_poll_command(&cmd)) {
        log_event(TS_EVENT_REMOTE_COMMAND, cmd.kind, cmd.arg0, cmd.text);
        switch (cmd.kind) {
            case TS_RADIO_CMD_GET_STATUS:
                radio_publish_status(&g_runtime);
                break;
            case TS_RADIO_CMD_ARM_SCENARIO:
                if (script_engine_arm((ts_scenario_kind_t)cmd.arg0, cmd.text, &g_runtime.scenario)) {
                    g_runtime.scenario.armed = true;
                    g_runtime.scenario.active = true;
                    log_event(TS_EVENT_SCENARIO_ARMED, cmd.arg0, 0, g_runtime.scenario.name);
                }
                break;
            case TS_RADIO_CMD_ABORT_SCENARIO:
                script_engine_abort("remote abort");
                g_runtime.scenario.active = false;
                g_runtime.flags &= ~TS_FLAG_MUTATION_ARMED;
                pd_phy_set_mutation_enabled(false);
                log_event(TS_EVENT_SCENARIO_ABORT, 0, 0, "remote abort");
                break;
            case TS_RADIO_CMD_LOAD_PROFILE:
                pd_phy_load_identity_profile(cmd.text);
                snprintf(g_runtime.active_profile, sizeof(g_runtime.active_profile), "%.31s", cmd.text);
                log_event(TS_EVENT_PROFILE_LOAD, 0, 0, cmd.text);
                break;
            case TS_RADIO_CMD_SET_LIMITS:
                vbus_meter_set_current_limit(cmd.arg0);
                log_event(TS_EVENT_REMOTE_COMMAND, cmd.arg0, 0, "set current limit");
                break;
            case TS_RADIO_CMD_NONE:
            default:
                break;
        }
    }
}

static void inject_demo_commands(void) {
    radio_queue_demo_command(TS_RADIO_CMD_GET_STATUS, 0, "status");
    radio_queue_demo_command(TS_RADIO_CMD_ARM_SCENARIO, TS_SCENARIO_DOCK_IDENTITY_FLIP, "dock_identity_flip");
}

static void dump_recent_events(uint32_t count) {
    uint32_t available = g_log_head < TS_EVENT_LOG_CAPACITY ? g_log_head : TS_EVENT_LOG_CAPACITY;
    uint32_t start = (available > count) ? (available - count) : 0;
    for (uint32_t i = start; i < available; ++i) {
        ts_event_t *e = &g_log[i % TS_EVENT_LOG_CAPACITY];
        printf("[event] t=%u type=%u p0=%u p1=%u %s\n", e->timestamp_ms, e->type, e->param0, e->param1, e->text);
    }
}

int main(void) {
    board_init();
    inject_demo_commands();
    for (g_now_ms = 0; g_now_ms <= 6000; g_now_ms += 100) {
        update_attach_state();
        update_power();
        service_remote();
        apply_script_actions();
        pump_pd_trace();
        if ((g_now_ms % 500u) == 0u) {
            print_status();
        }
    }
    dump_recent_events(32);
    return 0;
}
