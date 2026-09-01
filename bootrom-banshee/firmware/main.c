/*
 * BootROM Banshee reference firmware simulation
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/spi_link.h"
#include "drivers/overlay_store.h"
#include "drivers/glitch_engine.h"
#include "drivers/telemetry.h"
#include "drivers/radio.h"

static bb_runtime_t g_runtime;
static bb_event_t g_events[BB_EVENT_LOG_CAPACITY];
static uint32_t g_event_head;
static uint32_t g_now_ms;

static void log_event(bb_event_type_t type, uint32_t p0, uint32_t p1, const char *text) {
    bb_event_t *slot = &g_events[g_event_head % BB_EVENT_LOG_CAPACITY];
    slot->timestamp_ms = g_now_ms;
    slot->type = type;
    slot->param0 = p0;
    slot->param1 = p1;
    snprintf(slot->text, sizeof(slot->text), "%s", text ? text : "");
    g_event_head++;
}

static const char *scenario_name(bb_scenario_kind_t kind) {
    switch (kind) {
        case BB_SCENARIO_ROLLBACK_SHADOW: return "rollback-shadow";
        case BB_SCENARIO_JEDEC_MASQUERADE: return "jedec-masquerade";
        case BB_SCENARIO_LATE_READY_STALL: return "late-ready-stall";
        case BB_SCENARIO_HEADER_GHOST: return "header-ghost";
        case BB_SCENARIO_STRAP_SIREN: return "strap-siren";
        case BB_SCENARIO_CS_WHISPER: return "cs-whisper";
        case BB_SCENARIO_IDLE:
        default: return "idle";
    }
}

static const char *bus_mode_name(void) {
    return spi_link_bus_mode_name(g_runtime.bus_mode);
}

static void runtime_defaults(void) {
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.flags = BB_FLAG_PASSIVE_PROXY | BB_FLAG_TARGET_PRESENT | BB_FLAG_FLASH_PRESENT | BB_FLAG_CAPTURE_ACTIVE;
    g_runtime.bus_mode = BB_BUS_MODE_QSPI;
    snprintf(g_runtime.scenario.name, sizeof(g_runtime.scenario.name), "idle");
    g_runtime.scenario.kind = BB_SCENARIO_IDLE;
    snprintf(g_runtime.active_overlay, sizeof(g_runtime.active_overlay), "transparent");
    g_runtime.glitch_delay_us = 0u;
    g_runtime.truncation_len = 0u;
    g_runtime.strap_mask = 0u;
    g_runtime.mutation_enabled = false;
}

static void board_init(void) {
    runtime_defaults();
    spi_link_init();
    overlay_store_init();
    glitch_engine_init();
    telemetry_init();
    radio_init();
    spi_link_set_bus_mode(g_runtime.bus_mode);
    spi_link_set_overlay_name(g_runtime.active_overlay);
    log_event(BB_EVENT_BOOT, BB_FW_VERSION_MAJOR, BB_FW_VERSION_MINOR, "booted in passive proxy mode");
}

static void print_status(void) {
    printf("[status] t=%ums scenario=%s bus=%s overlay=%s flags=0x%02x delay=%uus trunc=%u strap=0x%02x current=%umA temp=%ldC\n",
           g_now_ms,
           g_runtime.scenario.name,
           bus_mode_name(),
           g_runtime.active_overlay,
           g_runtime.flags,
           g_runtime.glitch_delay_us,
           g_runtime.truncation_len,
           g_runtime.strap_mask,
           g_runtime.telemetry.rail_current_ma,
           (long)g_runtime.telemetry.board_temp_c);
}

static void set_overlay(const char *name) {
    snprintf(g_runtime.active_overlay, sizeof(g_runtime.active_overlay), "%.31s", name ? name : "transparent");
    spi_link_set_overlay_name(g_runtime.active_overlay);
}

static void apply_action(const bb_script_action_t *action) {
    if (action == NULL) {
        return;
    }
    switch (action->kind) {
        case BB_ACTION_ENABLE_MUTATION:
            g_runtime.mutation_enabled = true;
            g_runtime.flags |= BB_FLAG_MUTATION_ARMED;
            g_runtime.flags &= ~BB_FLAG_PASSIVE_PROXY;
            spi_link_set_mutation_enabled(true);
            log_event(BB_EVENT_GLITCH, action->kind, 1u, "mutation enabled");
            break;
        case BB_ACTION_DISABLE_MUTATION:
            g_runtime.mutation_enabled = false;
            g_runtime.flags &= ~BB_FLAG_MUTATION_ARMED;
            g_runtime.flags |= BB_FLAG_PASSIVE_PROXY;
            spi_link_set_mutation_enabled(false);
            log_event(BB_EVENT_GLITCH, action->kind, 0u, "mutation disabled");
            break;
        case BB_ACTION_SELECT_OVERLAY:
            set_overlay(action->text);
            log_event(BB_EVENT_OVERLAY, action->value0, 0u, g_runtime.active_overlay);
            break;
        case BB_ACTION_SET_DELAY_US:
            g_runtime.glitch_delay_us = action->value0;
            glitch_engine_set_delay_us(action->value0);
            spi_link_set_delay_us(action->value0);
            log_event(BB_EVENT_GLITCH, action->kind, action->value0, "delay updated");
            break;
        case BB_ACTION_SET_STRAP_MASK:
            g_runtime.strap_mask = action->value0;
            glitch_engine_set_strap_mask(action->value0);
            log_event(BB_EVENT_GLITCH, action->kind, action->value0, "strap mask updated");
            break;
        case BB_ACTION_SET_TRUNCATION:
            g_runtime.truncation_len = action->value0;
            glitch_engine_set_truncation(action->value0);
            spi_link_set_truncation(action->value0);
            log_event(BB_EVENT_GLITCH, action->kind, action->value0, "truncation updated");
            break;
        case BB_ACTION_FORCE_JEDEC:
            spi_link_force_jedec_id(action->value0);
            log_event(BB_EVENT_GLITCH, action->kind, action->value0, "forced jedec updated");
            break;
        case BB_ACTION_COMPLETE:
            g_runtime.scenario.active = false;
            g_runtime.scenario.completed = true;
            g_runtime.flags &= ~BB_FLAG_TRIGGERED;
            g_runtime.flags &= ~BB_FLAG_MUTATION_ARMED;
            g_runtime.flags |= BB_FLAG_PASSIVE_PROXY;
            g_runtime.mutation_enabled = false;
            spi_link_set_mutation_enabled(false);
            log_event(BB_EVENT_SCENARIO_COMPLETE, g_runtime.scenario.kind, 0u, g_runtime.scenario.name);
            break;
        case BB_ACTION_NONE:
        default:
            break;
    }
}

static bool schedule_next_action(bb_script_action_t *action) {
    if (action == NULL || !g_runtime.scenario.active || !g_runtime.scenario.triggered) {
        return false;
    }

    const uint32_t elapsed = g_now_ms - g_runtime.scenario.start_ms;
    memset(action, 0, sizeof(*action));
    switch (g_runtime.scenario.kind) {
        case BB_SCENARIO_ROLLBACK_SHADOW:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = BB_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = BB_ACTION_SELECT_OVERLAY; snprintf(action->text, sizeof(action->text), "rollback-shadow"); action->deadline_ms = 200u; break;
                case 2u: action->kind = BB_ACTION_SET_DELAY_US; action->value0 = 40u; action->deadline_ms = 300u; break;
                case 3u: action->kind = BB_ACTION_COMPLETE; action->deadline_ms = 1200u; break;
                default: return false;
            }
            break;
        case BB_SCENARIO_JEDEC_MASQUERADE:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = BB_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = BB_ACTION_FORCE_JEDEC; action->value0 = 0xEF4018u; action->deadline_ms = 150u; break;
                case 2u: action->kind = BB_ACTION_SET_DELAY_US; action->value0 = 25u; action->deadline_ms = 250u; break;
                case 3u: action->kind = BB_ACTION_COMPLETE; action->deadline_ms = 1000u; break;
                default: return false;
            }
            break;
        case BB_SCENARIO_LATE_READY_STALL:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = BB_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = BB_ACTION_SET_DELAY_US; action->value0 = 280u; action->deadline_ms = 200u; break;
                case 2u: action->kind = BB_ACTION_SET_TRUNCATION; action->value0 = 8u; action->deadline_ms = 350u; break;
                case 3u: action->kind = BB_ACTION_COMPLETE; action->deadline_ms = 1300u; break;
                default: return false;
            }
            break;
        case BB_SCENARIO_HEADER_GHOST:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = BB_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = BB_ACTION_SELECT_OVERLAY; snprintf(action->text, sizeof(action->text), "header-ghost"); action->deadline_ms = 200u; break;
                case 2u: action->kind = BB_ACTION_SET_TRUNCATION; action->value0 = 16u; action->deadline_ms = 300u; break;
                case 3u: action->kind = BB_ACTION_COMPLETE; action->deadline_ms = 1200u; break;
                default: return false;
            }
            break;
        case BB_SCENARIO_STRAP_SIREN:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = BB_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = BB_ACTION_SET_STRAP_MASK; action->value0 = 2u; action->deadline_ms = 150u; break;
                case 2u: action->kind = BB_ACTION_SET_DELAY_US; action->value0 = 75u; action->deadline_ms = 250u; break;
                case 3u: action->kind = BB_ACTION_COMPLETE; action->deadline_ms = 950u; break;
                default: return false;
            }
            break;
        case BB_SCENARIO_CS_WHISPER:
            switch (g_runtime.scenario.step_index) {
                case 0u: action->kind = BB_ACTION_ENABLE_MUTATION; action->deadline_ms = 100u; break;
                case 1u: action->kind = BB_ACTION_SET_TRUNCATION; action->value0 = 4u; action->deadline_ms = 180u; break;
                case 2u: action->kind = BB_ACTION_SET_DELAY_US; action->value0 = 12u; action->deadline_ms = 220u; break;
                case 3u: action->kind = BB_ACTION_COMPLETE; action->deadline_ms = 900u; break;
                default: return false;
            }
            break;
        case BB_SCENARIO_IDLE:
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
    bb_script_action_t action;
    while (schedule_next_action(&action)) {
        apply_action(&action);
    }
}

static void clear_mutation_state(void) {
    g_runtime.glitch_delay_us = 0u;
    g_runtime.truncation_len = 0u;
    g_runtime.strap_mask = 0u;
    g_runtime.mutation_enabled = false;
    g_runtime.flags &= ~BB_FLAG_STRAP_ACTIVE;
    glitch_engine_set_delay_us(0u);
    glitch_engine_set_truncation(0u);
    glitch_engine_set_strap_mask(0u);
    spi_link_set_delay_us(0u);
    spi_link_set_truncation(0u);
    spi_link_force_jedec_id(0u);
    spi_link_set_mutation_enabled(false);
    set_overlay("transparent");
}

static void arm_scenario(bb_scenario_kind_t kind) {
    clear_mutation_state();
    memset(&g_runtime.scenario, 0, sizeof(g_runtime.scenario));
    g_runtime.scenario.kind = kind;
    snprintf(g_runtime.scenario.name, sizeof(g_runtime.scenario.name), "%s", scenario_name(kind));
    g_runtime.scenario.armed = true;
    g_runtime.scenario.active = true;
    g_runtime.scenario.triggered = false;
    g_runtime.scenario.completed = false;
    g_runtime.scenario.mutation_budget_ms = 1500u;
    log_event(BB_EVENT_SCENARIO_ARMED, kind, 0u, g_runtime.scenario.name);
}

static void trigger_scenario(void) {
    if (!g_runtime.scenario.armed || !g_runtime.scenario.active) {
        return;
    }
    g_runtime.scenario.triggered = true;
    g_runtime.scenario.start_ms = g_now_ms;
    g_runtime.flags |= BB_FLAG_TRIGGERED;
    log_event(BB_EVENT_SCENARIO_TRIGGERED, g_runtime.scenario.kind, g_now_ms, g_runtime.scenario.name);
}

static void service_remote(void) {
    bb_radio_command_t cmd;
    while (radio_poll_command(&cmd)) {
        log_event(BB_EVENT_REMOTE_COMMAND, cmd.kind, cmd.arg0, cmd.text);
        switch (cmd.kind) {
            case BB_RADIO_CMD_GET_STATUS:
                radio_publish_status(&g_runtime);
                break;
            case BB_RADIO_CMD_ARM_SCENARIO:
                arm_scenario((bb_scenario_kind_t)cmd.arg0);
                break;
            case BB_RADIO_CMD_TRIGGER:
                trigger_scenario();
                break;
            case BB_RADIO_CMD_ABORT:
                clear_mutation_state();
                g_runtime.scenario.active = false;
                g_runtime.scenario.armed = false;
                g_runtime.flags &= ~(BB_FLAG_MUTATION_ARMED | BB_FLAG_TRIGGERED);
                g_runtime.flags |= BB_FLAG_PASSIVE_PROXY;
                log_event(BB_EVENT_SCENARIO_ABORT, 0u, 0u, "remote abort");
                break;
            case BB_RADIO_CMD_LOAD_OVERLAY:
                set_overlay(cmd.text);
                log_event(BB_EVENT_OVERLAY, 0u, 0u, g_runtime.active_overlay);
                break;
            case BB_RADIO_CMD_SET_LIMITS:
                telemetry_set_current_limit(cmd.arg0);
                log_event(BB_EVENT_REMOTE_COMMAND, cmd.arg0, 0u, "telemetry limit updated");
                break;
            case BB_RADIO_CMD_NONE:
            default:
                break;
        }
    }
}

static void update_telemetry(void) {
    g_runtime.telemetry = telemetry_sample(g_now_ms, g_runtime.mutation_enabled);
    if (g_runtime.telemetry.trip) {
        clear_mutation_state();
        g_runtime.flags |= BB_FLAG_SAFETY_TRIP;
        g_runtime.flags |= BB_FLAG_PASSIVE_PROXY;
        g_runtime.flags &= ~(BB_FLAG_MUTATION_ARMED | BB_FLAG_TRIGGERED);
        g_runtime.scenario.active = false;
        log_event(BB_EVENT_FAULT, g_runtime.telemetry.rail_current_ma, (uint32_t)g_runtime.telemetry.board_temp_c, "telemetry safety trip");
    }
}

static void capture_bus(void) {
    bb_spi_frame_t frame;
    char status[BB_STATUS_TEXT_LEN];
    if (!spi_link_poll_frame(g_now_ms, &frame)) {
        return;
    }
    glitch_engine_process(&g_runtime, &frame, g_now_ms, status, sizeof(status));
    if (frame.mutated) {
        log_event(BB_EVENT_CAPTURE, frame.addr, frame.opcode, status);
    } else {
        char passive[BB_STATUS_TEXT_LEN];
        snprintf(passive, sizeof(passive), "opcode=0x%02X addr=0x%08X bus=%s len=%u", frame.opcode, frame.addr, bus_mode_name(), frame.data_len);
        log_event(BB_EVENT_CAPTURE, frame.addr, frame.opcode, passive);
    }
}

static void inject_demo_commands(void) {
    radio_queue_demo_command(BB_RADIO_CMD_GET_STATUS, 0u, 0u, "status");
    radio_queue_demo_command(BB_RADIO_CMD_SET_LIMITS, 700u, 0u, "set-safe-current");
    radio_queue_demo_command(BB_RADIO_CMD_ARM_SCENARIO, BB_SCENARIO_HEADER_GHOST, 0u, "header-ghost");
    radio_queue_demo_command(BB_RADIO_CMD_TRIGGER, 0u, 0u, "trigger");
}

static void dump_recent_events(uint32_t count) {
    const uint32_t available = (g_event_head < BB_EVENT_LOG_CAPACITY) ? g_event_head : BB_EVENT_LOG_CAPACITY;
    const uint32_t start = (available > count) ? (available - count) : 0u;
    for (uint32_t i = start; i < available; ++i) {
        bb_event_t *e = &g_events[i % BB_EVENT_LOG_CAPACITY];
        printf("[event] t=%u type=%u p0=%u p1=%u %s\n", e->timestamp_ms, e->type, e->param0, e->param1, e->text);
    }
}

int main(void) {
    board_init();
    inject_demo_commands();

    for (g_now_ms = 0u; g_now_ms <= 2200u; g_now_ms += 100u) {
        service_remote();
        service_script();
        update_telemetry();
        capture_bus();
        if ((g_now_ms % 400u) == 0u) {
            print_status();
        }
    }

    dump_recent_events(32u);
    return 0;
}
