/*
 * main.c - eSPI Revenant simulation entry point
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "registers.h"
#include "drivers/espi_bus.h"
#include "drivers/interposer.h"
#include "drivers/power.h"
#include "drivers/radio.h"
#include "drivers/trace.h"

static er_profile_t make_profile_resume_glitch(void)
{
    er_profile_t p;
    memset(&p, 0, sizeof(p));
    snprintf(p.name, sizeof(p.name), "resume-glitch-window");
    p.allow_vwire_inject = 1u;
    p.allow_flash_delay = 1u;
    p.allow_peripheral_replay = 0u;
    p.trigger_on_resume = 1u;
    p.flash_delay_ns = 96u;
    p.max_vwire_pulses = 1u;
    p.max_peripheral_ops = 0u;
    p.passive_prearm = 1u;
    return p;
}

static er_profile_t make_profile_maintenance_probe(void)
{
    er_profile_t p;
    memset(&p, 0, sizeof(p));
    snprintf(p.name, sizeof(p.name), "ec-maintenance-probe");
    p.allow_vwire_inject = 0u;
    p.allow_flash_delay = 0u;
    p.allow_peripheral_replay = 1u;
    p.trigger_on_resume = 0u;
    p.flash_delay_ns = 0u;
    p.max_vwire_pulses = 0u;
    p.max_peripheral_ops = 2u;
    p.passive_prearm = 0u;
    return p;
}

static void populate_event(er_event_t *event,
                           uint32_t timestamp_ms,
                           er_event_code_t code,
                           er_channel_t channel,
                           er_risk_t risk,
                           const char *message)
{
    event->timestamp_ms = timestamp_ms;
    event->code = code;
    event->channel = channel;
    event->risk = risk;
    snprintf(event->message, sizeof(event->message), "%s", message);
}

static void print_event(const er_event_t *event)
{
    printf("[%05u ms] event code=%d ch=%s risk=%d :: %s\n",
           event->timestamp_ms,
           event->code,
           er_bus_channel_name(event->channel),
           event->risk,
           event->message);
}

static void log_trace_frame(const er_trace_frame_t *frame)
{
    char description[128];
    er_bus_describe_frame(frame, description, sizeof(description));
    printf("  trace %-10s %s\n", er_bus_channel_name(frame->channel), description);
}

static void apply_profile(er_bus_t *bus, er_status_t *status, const er_profile_t *profile, er_event_t *event)
{
    er_bus_load_profile(bus, profile);
    snprintf(status->active_profile, sizeof(status->active_profile), "%s", profile->name);
    populate_event(event,
                   bus->tick_ms,
                   ER_EVENT_PROFILE_LOAD,
                   ER_CH_GPIO,
                   ER_RISK_LOW,
                   "Profile loaded into policy engine");
}

static void maybe_run_action(er_bus_t *bus,
                             er_interposer_t *interposer,
                             er_status_t *status,
                             const er_trace_frame_t *frame,
                             er_event_t *event)
{
    if (frame->channel == ER_CH_FLASH && er_interposer_can_delay_flash(interposer, &bus->profile)) {
        er_bus_set_flash_delay(bus, bus->profile.flash_delay_ns);
        interposer->last_action_ms = bus->tick_ms;
        populate_event(event,
                       bus->tick_ms,
                       ER_EVENT_FLASH_DELAY,
                       ER_CH_FLASH,
                       ER_RISK_MEDIUM,
                       "Bounded flash completion delay applied");
        status->trigger_count++;
        return;
    }

    if (frame->channel == ER_CH_VWIRE && er_interposer_can_inject_vwire(interposer, &bus->profile) &&
        frame->data[2] >= 2u) {
        er_bus_apply_vwire_pulse(bus, ER_VWIRE_HOST_RST_WARN);
        interposer->vwire_pulses_used++;
        er_interposer_note_action(interposer, bus->tick_ms);
        populate_event(event,
                       bus->tick_ms,
                       ER_EVENT_VWIRE_INJECT,
                       ER_CH_VWIRE,
                       ER_RISK_MEDIUM,
                       "Single contradictory HOST_RST_WARN pulse injected");
        status->trigger_count++;
        return;
    }

    if (frame->channel == ER_CH_PERIPHERAL && er_interposer_can_replay_peripheral(interposer, &bus->profile) &&
        frame->data[0] == 0x61u) {
        interposer->periph_ops_used++;
        er_interposer_note_action(interposer, bus->tick_ms);
        populate_event(event,
                       bus->tick_ms,
                       ER_EVENT_PERIPH_REPLAY,
                       ER_CH_PERIPHERAL,
                       ER_RISK_HIGH,
                       "Maintenance-style peripheral sequence replayed");
        status->trigger_count++;
        return;
    }

    populate_event(event,
                   bus->tick_ms,
                   ER_EVENT_TRACE,
                   frame->channel,
                   ER_RISK_LOW,
                   "No active manipulation on this frame");
}

static void run_scenario(const er_profile_t *profile, uint8_t arm)
{
    er_bus_t bus;
    er_interposer_t interposer;
    er_power_t power;
    er_radio_t radio;
    er_trace_log_t trace;
    er_status_t status;
    er_event_t event;

    memset(&status, 0, sizeof(status));
    status.mode = ER_MODE_PASSIVE;
    snprintf(status.active_profile, sizeof(status.active_profile), "baseline-passive");

    er_bus_init(&bus);
    er_interposer_init(&interposer);
    er_power_init(&power);
    er_radio_init(&radio);
    er_trace_init(&trace);

    printf("\n=== %s scenario: %s ===\n", ER_DEVICE_NAME, profile->name);
    printf("author=%s\n", ER_AUTHOR);

    apply_profile(&bus, &status, profile, &event);
    print_event(&event);

    if (profile->trigger_on_resume) {
        er_bus_force_resume_window(&bus);
        populate_event(&event,
                       bus.tick_ms,
                       ER_EVENT_BOOT,
                       ER_CH_GPIO,
                       ER_RISK_LOW,
                       "Resume window staged for trigger testing");
        print_event(&event);
    }

    er_interposer_arm(&interposer, arm);
    if (arm) {
        status.mode = ER_MODE_GUARDED;
        er_radio_accept_command(&radio, "arm-profile", &event);
        event.timestamp_ms = bus.tick_ms;
        print_event(&event);
    }

    for (unsigned i = 0u; i < 18u; ++i) {
        er_trace_frame_t frame;
        char trace_summary[160];
        char latest[160];

        er_bus_step(&bus, &frame);
        er_trace_push(&trace, &frame);
        er_power_step(&power, bus.tick_ms, arm);
        er_interposer_tick(&interposer, &bus, &status);

        status.anomalies = trace.anomaly_score;
        if (status.mode != ER_MODE_BYPASS && arm) {
            status.mode = ER_MODE_GUARDED;
        }

        maybe_run_action(&bus, &interposer, &status, &frame, &event);
        print_event(&event);
        log_trace_frame(&frame);

        if (er_power_apply_status(&power, &status, &event)) {
            event.timestamp_ms = bus.tick_ms;
            print_event(&event);
            er_interposer_force_bypass(&interposer, &status, "safety rollback", &event);
            event.timestamp_ms = bus.tick_ms;
            print_event(&event);
            break;
        }

        er_trace_summarize(&trace, trace_summary, sizeof(trace_summary));
        er_trace_format_latest(&trace, latest, sizeof(latest));
        er_radio_build_status_frame(&radio, &status, trace_summary);
        printf("  latest: %s\n", latest);
        printf("  radio : %s\n", er_radio_last_frame(&radio));
    }

    printf("summary profile=%s triggers=%u anomalies=%u temp=%.1fC current=%.1fmA mode=%u bypass=%u\n",
           status.active_profile,
           status.trigger_count,
           status.anomalies,
           status.board_temp_c,
           status.ec_current_ma,
           status.mode,
           status.bypass_enabled);
}

int main(void)
{
    er_profile_t resume = make_profile_resume_glitch();
    er_profile_t maintenance = make_profile_maintenance_probe();

    printf("%s firmware simulation by %s\n", ER_DEVICE_NAME, ER_AUTHOR);
    printf("authorized use only - design package for security research\n");

    run_scenario(&resume, 1u);
    run_scenario(&maintenance, 1u);

    return 0;
}
