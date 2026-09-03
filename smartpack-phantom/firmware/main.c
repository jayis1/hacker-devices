/*
 * SmartPack Phantom reference firmware simulation
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/smbus_bus.h"
#include "drivers/battery_profile.h"
#include "drivers/auth_chip.h"
#include "drivers/telemetry.h"
#include "drivers/radio.h"

static sp_runtime_t g_runtime;
static sp_event_t g_events[SP_EVENT_LOG_CAPACITY];
static uint32_t g_event_head;
static uint32_t g_now_ms;

static void log_event(sp_event_type_t type, uint32_t p0, uint32_t p1, const char *text) {
    sp_event_t *slot = &g_events[g_event_head % SP_EVENT_LOG_CAPACITY];
    slot->timestamp_ms = g_now_ms;
    slot->type = type;
    slot->param0 = p0;
    slot->param1 = p1;
    snprintf(slot->text, sizeof(slot->text), "%.79s", text != NULL ? text : "");
    g_event_head++;
}

static const char *scenario_name(sp_scenario_kind_t kind) {
    switch (kind) {
        case SP_SCENARIO_MAINTENANCE_MASK: return "maintenance-mask";
        case SP_SCENARIO_LOW_SOC_BAIT: return "low-soc-bait";
        case SP_SCENARIO_STALE_AUTH_REPLAY: return "stale-auth-replay";
        case SP_SCENARIO_SHIPPING_CONFUSION: return "shipping-confusion";
        case SP_SCENARIO_THERMAL_TRIP_SPOOF: return "thermal-trip-spoof";
        case SP_SCENARIO_CHARGER_LIMIT_SWING: return "charger-limit-swing";
        case SP_SCENARIO_IDLE:
        default: return "idle";
    }
}

static void runtime_defaults(void) {
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.flags = SP_FLAG_PASSIVE_PROXY | SP_FLAG_PACK_AUTH_OK;
    g_runtime.passive_mode = true;
    g_runtime.authorized_mode = true;
    g_runtime.pack_present = true;
    g_runtime.host_present = true;
    battery_profile_init(&g_runtime.profile, SP_PROFILE_ENTERPRISE_LAPTOP);
    snprintf(g_runtime.scenario.name, sizeof(g_runtime.scenario.name), "idle");
    g_runtime.scenario.kind = SP_SCENARIO_IDLE;
    g_runtime.mutation_enabled = false;
}

static void board_init(void) {
    runtime_defaults();
    smbus_bus_init();
    smbus_bus_reset_stats(&g_runtime.bus);
    auth_chip_init(&g_runtime.auth);
    telemetry_init(&g_runtime.telemetry);
    telemetry_apply_profile(&g_runtime);
    radio_init(&g_runtime.radio);
    log_event(SP_EVENT_BOOT, SP_FW_VERSION_MAJOR, SP_FW_VERSION_MINOR, "booted in passive proxy mode");
}

static void print_status(void) {
    printf("[status] t=%ums scenario=%s profile=%s soc=%u%% temp=%dC auth=%s health=%s flags=0x%02x mutations=%u alerts=%u\n",
           g_now_ms,
           g_runtime.scenario.name,
           battery_profile_name(g_runtime.profile.kind),
           battery_profile_relative_soc(&g_runtime.profile),
           g_runtime.profile.temperature_c,
           auth_chip_mode_name(g_runtime.auth.mode),
           telemetry_health_string(&g_runtime),
           g_runtime.flags,
           g_runtime.bus.mutated_replies,
           g_runtime.bus.alert_assertions);
}

static void sync_runtime_flags(void) {
    if (g_runtime.mutation_enabled) {
        g_runtime.flags |= SP_FLAG_MUTATION_ARMED;
        g_runtime.flags &= ~SP_FLAG_PASSIVE_PROXY;
        g_runtime.passive_mode = false;
    } else {
        g_runtime.flags &= ~SP_FLAG_MUTATION_ARMED;
        g_runtime.flags |= SP_FLAG_PASSIVE_PROXY;
        g_runtime.passive_mode = true;
    }

    if (g_runtime.profile.shipping_mode) {
        g_runtime.flags |= SP_FLAG_SHIPPING_MODE;
    } else {
        g_runtime.flags &= ~SP_FLAG_SHIPPING_MODE;
    }

    if (g_runtime.bus.alert_line) {
        g_runtime.flags |= SP_FLAG_ALERT_ACTIVE;
    } else {
        g_runtime.flags &= ~SP_FLAG_ALERT_ACTIVE;
    }
}

static void apply_action(const sp_script_action_t *action) {
    if (action == NULL) {
        return;
    }
    switch (action->kind) {
        case SP_ACTION_ENABLE_MUTATION:
            g_runtime.mutation_enabled = true;
            smbus_bus_set_mutation_enabled(true);
            log_event(SP_EVENT_SCENARIO_TRIGGER, 1u, 0u, "mutation enabled");
            break;
        case SP_ACTION_DISABLE_MUTATION:
            g_runtime.mutation_enabled = false;
            smbus_bus_set_mutation_enabled(false);
            log_event(SP_EVENT_SCENARIO_COMPLETE, 0u, 0u, "mutation disabled");
            break;
        case SP_ACTION_SET_PROFILE:
            if (strcmp(action->text, "service-pack") == 0) {
                battery_profile_init(&g_runtime.profile, SP_PROFILE_SERVICE_PACK);
            } else if (strcmp(action->text, "counterfeit-clone") == 0) {
                battery_profile_init(&g_runtime.profile, SP_PROFILE_COUNTERFEIT_CLONE);
            }
            log_event(SP_EVENT_PROFILE, g_runtime.profile.kind, 0u, battery_profile_name(g_runtime.profile.kind));
            break;
        case SP_ACTION_SET_SOC:
            battery_profile_set_soc_percent(&g_runtime.profile, (uint8_t)action->value0);
            log_event(SP_EVENT_PROFILE, action->value0, 0u, "soc updated");
            break;
        case SP_ACTION_SET_TEMP_C:
            battery_profile_set_temperature(&g_runtime.profile, (int16_t)action->value0);
            log_event(SP_EVENT_PROFILE, action->value0, 0u, "temperature updated");
            break;
        case SP_ACTION_SET_STATUS_WORD:
            battery_profile_apply_status(&g_runtime.profile, (uint16_t)action->value0);
            log_event(SP_EVENT_PROFILE, action->value0, 0u, "status word updated");
            break;
        case SP_ACTION_SET_AUTH_MODE:
            auth_chip_set_mode(&g_runtime.auth, (sp_auth_mode_t)action->value0);
            log_event(SP_EVENT_AUTH, action->value0, 0u, auth_chip_mode_name(g_runtime.auth.mode));
            break;
        case SP_ACTION_ASSERT_ALERT:
            smbus_bus_set_alert(action->value0 != 0u, &g_runtime.bus);
            log_event(SP_EVENT_ALERT, action->value0, 0u, action->value0 ? "alert asserted" : "alert cleared");
            break;
        case SP_ACTION_SET_CHARGE_LIMIT:
            battery_profile_set_charge_limit(&g_runtime.profile, (uint16_t)action->value0);
            log_event(SP_EVENT_PROFILE, action->value0, 0u, "charge limit updated");
            break;
        case SP_ACTION_COMPLETE:
            g_runtime.scenario.active = false;
            g_runtime.scenario.completed = true;
            g_runtime.scenario.triggered = false;
            g_runtime.flags &= ~SP_FLAG_TRIGGERED;
            g_runtime.mutation_enabled = false;
            smbus_bus_set_mutation_enabled(false);
            log_event(SP_EVENT_SCENARIO_COMPLETE, g_runtime.scenario.kind, 0u, g_runtime.scenario.name);
            break;
        case SP_ACTION_NONE:
        default:
            break;
    }
    battery_profile_apply_named_overlay(&g_runtime.profile, g_runtime.scenario.name);
    telemetry_apply_profile(&g_runtime);
    sync_runtime_flags();
}

static bool schedule_next_action(sp_script_action_t *action) {
    uint32_t elapsed;
    if (action == NULL || !g_runtime.scenario.active || !g_runtime.scenario.triggered) {
        return false;
    }
    elapsed = g_now_ms - g_runtime.scenario.start_ms;
    memset(action, 0, sizeof(*action));
    switch (g_runtime.scenario.kind) {
        case SP_SCENARIO_MAINTENANCE_MASK:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = SP_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = SP_ACTION_SET_PROFILE; snprintf(action->text, sizeof(action->text), "service-pack"); action->deadline_ms = 160u; break;
                case 2u: action->kind = SP_ACTION_ASSERT_ALERT; action->value0 = 1u; action->deadline_ms = 260u; break;
                case 3u: action->kind = SP_ACTION_COMPLETE; action->deadline_ms = 1000u; break;
                default: return false;
            }
            break;
        case SP_SCENARIO_LOW_SOC_BAIT:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = SP_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = SP_ACTION_SET_SOC; action->value0 = 2u; action->deadline_ms = 220u; break;
                case 2u: action->kind = SP_ACTION_SET_STATUS_WORD; action->value0 = 0x4000u; action->deadline_ms = 300u; break;
                case 3u: action->kind = SP_ACTION_COMPLETE; action->deadline_ms = 950u; break;
                default: return false;
            }
            break;
        case SP_SCENARIO_STALE_AUTH_REPLAY:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = SP_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = SP_ACTION_SET_AUTH_MODE; action->value0 = SP_AUTH_REPLAY_LAST; action->deadline_ms = 150u; break;
                case 2u: action->kind = SP_ACTION_ASSERT_ALERT; action->value0 = 1u; action->deadline_ms = 240u; break;
                case 3u: action->kind = SP_ACTION_COMPLETE; action->deadline_ms = 900u; break;
                default: return false;
            }
            break;
        case SP_SCENARIO_SHIPPING_CONFUSION:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = SP_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = SP_ACTION_SET_STATUS_WORD; action->value0 = 0x0200u; action->deadline_ms = 180u; break;
                case 2u: action->kind = SP_ACTION_SET_SOC; action->value0 = 88u; action->deadline_ms = 220u; break;
                case 3u: action->kind = SP_ACTION_COMPLETE; action->deadline_ms = 850u; break;
                default: return false;
            }
            break;
        case SP_SCENARIO_THERMAL_TRIP_SPOOF:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = SP_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = SP_ACTION_SET_TEMP_C; action->value0 = 78u; action->deadline_ms = 180u; break;
                case 2u: action->kind = SP_ACTION_ASSERT_ALERT; action->value0 = 1u; action->deadline_ms = 220u; break;
                case 3u: action->kind = SP_ACTION_COMPLETE; action->deadline_ms = 900u; break;
                default: return false;
            }
            break;
        case SP_SCENARIO_CHARGER_LIMIT_SWING:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = SP_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = SP_ACTION_SET_CHARGE_LIMIT; action->value0 = 500u; action->deadline_ms = 180u; break;
                case 2u: action->kind = SP_ACTION_SET_STATUS_WORD; action->value0 = 0x0800u; action->deadline_ms = 260u; break;
                case 3u: action->kind = SP_ACTION_COMPLETE; action->deadline_ms = 900u; break;
                default: return false;
            }
            break;
        case SP_SCENARIO_IDLE:
        default:
            return false;
    }

    if (elapsed >= action->deadline_ms) {
        g_runtime.scenario.step_index++;
        return true;
    }
    return false;
}

static void service_script(void) {
    sp_script_action_t action;
    while (schedule_next_action(&action)) {
        apply_action(&action);
    }
}

static void arm_scenario(sp_scenario_kind_t kind) {
    battery_profile_init(&g_runtime.profile, SP_PROFILE_ENTERPRISE_LAPTOP);
    auth_chip_set_mode(&g_runtime.auth, SP_AUTH_PASSTHROUGH);
    smbus_bus_set_alert(false, &g_runtime.bus);
    g_runtime.mutation_enabled = false;
    smbus_bus_set_mutation_enabled(false);
    g_runtime.scenario.active = kind != SP_SCENARIO_IDLE;
    g_runtime.scenario.triggered = false;
    g_runtime.scenario.completed = false;
    g_runtime.scenario.step_index = 0u;
    g_runtime.scenario.kind = kind;
    snprintf(g_runtime.scenario.name, sizeof(g_runtime.scenario.name), "%s", scenario_name(kind));
    telemetry_apply_profile(&g_runtime);
    sync_runtime_flags();
    log_event(SP_EVENT_SCENARIO_ARM, kind, 0u, g_runtime.scenario.name);
}

static void trigger_scenario(void) {
    if (!g_runtime.scenario.active) {
        return;
    }
    g_runtime.scenario.triggered = true;
    g_runtime.scenario.start_ms = g_now_ms;
    g_runtime.flags |= SP_FLAG_TRIGGERED;
    log_event(SP_EVENT_SCENARIO_TRIGGER, g_runtime.scenario.kind, 0u, g_runtime.scenario.name);
}

static void query_command(uint8_t command) {
    sp_bus_reply_t reply;
    char line[160];
    smbus_bus_prepare_reply(&g_runtime, command, &reply);
    smbus_bus_record_exchange(&g_runtime.bus, &reply);
    smbus_bus_format_reply(&reply, line, sizeof(line));
    printf("[bus] %s\n", line);
    log_event(SP_EVENT_BUS, command, reply.word, line);
}

static void run_auth_exchange(void) {
    uint32_t challenge;
    bool accepted = false;
    challenge = auth_chip_issue_challenge(&g_runtime.auth, g_now_ms ^ 0x5a5aa5a5u);
    g_runtime.auth.last_challenge = challenge;
    log_event(SP_EVENT_AUTH, challenge, 0u, "challenge issued");
    query_command(SP_VENDOR_AUTH_CHALLENGE);
    (void)auth_chip_respond(&g_runtime.auth, challenge, &accepted);
    query_command(SP_VENDOR_AUTH_RESPONSE);
    if (accepted) {
        g_runtime.flags |= SP_FLAG_PACK_AUTH_OK;
    } else {
        g_runtime.flags &= ~SP_FLAG_PACK_AUTH_OK;
    }
}

static void service_host_poll(void) {
    static const uint8_t commands[] = {
        SP_SBS_VOLTAGE,
        SP_SBS_CURRENT,
        SP_SBS_RELATIVE_SOC,
        SP_SBS_STATUS,
        SP_SBS_MANUFACTURER_NAME,
        SP_SBS_DEVICE_NAME,
        SP_VENDOR_SERVICE_FLAGS,
        SP_VENDOR_CHARGE_POLICY
    };
    size_t i;
    for (i = 0u; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        query_command(commands[i]);
    }
}

static void tick_once(uint32_t step_ms) {
    char frame[192];
    g_now_ms += step_ms;
    telemetry_tick(&g_runtime, g_now_ms);
    service_script();
    if ((g_now_ms % 250u) == 0u) {
        service_host_poll();
    }
    if ((g_now_ms % 500u) == 0u) {
        run_auth_exchange();
    }
    if ((g_now_ms % 1000u) == 0u) {
        radio_emit_status(&g_runtime, frame, sizeof(frame));
        printf("[radio] %s\n", frame);
        log_event(SP_EVENT_EXPORT, g_runtime.radio.frames_sent, 0u, frame);
    }
}

static void dump_recent_events(void) {
    uint32_t count = g_event_head < 8u ? g_event_head : 8u;
    uint32_t start = g_event_head > count ? g_event_head - count : 0u;
    uint32_t i;
    printf("[events] recent=%u\n", count);
    for (i = start; i < g_event_head; ++i) {
        const sp_event_t *event = &g_events[i % SP_EVENT_LOG_CAPACITY];
        printf("  - t=%u type=%u p0=%u p1=%u text=%s\n",
               event->timestamp_ms,
               (unsigned)event->type,
               event->param0,
               event->param1,
               event->text);
    }
}

static void export_bundle(void) {
    char bundle[512];
    uint32_t count = g_event_head < 8u ? g_event_head : 8u;
    uint32_t start = g_event_head > count ? g_event_head - count : 0u;
    uint32_t i;
    sp_event_t slice[8];
    for (i = 0u; i < count; ++i) {
        slice[i] = g_events[(start + i) % SP_EVENT_LOG_CAPACITY];
    }
    radio_emit_event_bundle(slice, count, &g_runtime, bundle, sizeof(bundle));
    printf("[export] %s\n", bundle);
}

static void simulate_scenario(sp_scenario_kind_t kind) {
    uint32_t i;
    printf("\n=== scenario: %s ===\n", scenario_name(kind));
    arm_scenario(kind);
    trigger_scenario();
    for (i = 0u; i < 12u; ++i) {
        tick_once(100u);
    }
    print_status();
    dump_recent_events();
}

int main(void) {
    size_t i;
    const sp_scenario_kind_t scenarios[] = {
        SP_SCENARIO_MAINTENANCE_MASK,
        SP_SCENARIO_LOW_SOC_BAIT,
        SP_SCENARIO_STALE_AUTH_REPLAY,
        SP_SCENARIO_SHIPPING_CONFUSION,
        SP_SCENARIO_THERMAL_TRIP_SPOOF,
        SP_SCENARIO_CHARGER_LIMIT_SWING
    };

    board_init();
    print_status();
    for (i = 0u; i < sizeof(scenarios) / sizeof(scenarios[0]); ++i) {
        simulate_scenario(scenarios[i]);
    }
    export_bundle();
    printf("[summary] events=%u frames=%u exports=%u hostQueries=%u mutatedReplies=%u\n",
           g_event_head,
           g_runtime.radio.frames_sent,
           g_runtime.radio.exports_generated,
           g_runtime.bus.host_queries,
           g_runtime.bus.mutated_replies);
    return 0;
}
