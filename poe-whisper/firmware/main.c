/*
 * main.c - PoE Whisper simulation firmware
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "registers.h"
#include "drivers/poe_port.h"
#include "drivers/lldp.h"
#include "drivers/signature.h"
#include "drivers/relay.h"
#include "drivers/radio.h"

pw_fpga_regs_t PW_FPGA;
pw_radio_regs_t PW_RADIO;

static void add_event(pw_event_t *events, size_t *count, pw_event_code_t code, uint32_t ts, const char *message)
{
    if (*count >= PW_MAX_EVENTS) {
        return;
    }
    events[*count].timestamp_ms = ts;
    events[*count].code = code;
    snprintf(events[*count].message, sizeof(events[*count].message), "%s", message);
    (*count)++;
}

static pw_profile_t build_profile(const char *name)
{
    pw_profile_t p;
    memset(&p, 0, sizeof(p));
    snprintf(p.name, sizeof(p.name), "%s", name);

    if (strcmp(name, "camera-reboot-window") == 0) {
        p.advertised_class = PW_CLASS_4;
        p.requested_power_w = 24.5f;
        p.brownout_ms = 120u;
        p.brownout_target_v = 36.5f;
        p.lldp_spoof = 1;
        p.mps_jitter = 0;
    } else if (strcmp(name, "phone-class-downgrade") == 0) {
        p.advertised_class = PW_CLASS_2;
        p.requested_power_w = 6.5f;
        p.brownout_ms = 0u;
        p.brownout_target_v = 0.0f;
        p.lldp_spoof = 1;
        p.mps_jitter = 1;
    } else if (strcmp(name, "badge-reader-mps-jitter") == 0) {
        p.advertised_class = PW_CLASS_3;
        p.requested_power_w = 12.5f;
        p.brownout_ms = 80u;
        p.brownout_target_v = 40.0f;
        p.lldp_spoof = 0;
        p.mps_jitter = 1;
    } else {
        p.advertised_class = PW_CLASS_4;
        p.requested_power_w = 20.0f;
        p.brownout_ms = 0u;
        p.brownout_target_v = 0.0f;
        p.passive_only = 1;
    }
    return p;
}

static void print_banner(void)
{
    printf("%s simulation firmware by %s\n", PW_DEVICE_NAME, PW_AUTHOR);
    printf("Authorized security research use only.\n\n");
}

static void simulate_current_trace(pw_signature_result_t *sig, const pw_profile_t *profile)
{
    pw_signature_reset(sig);
    float base = profile->requested_power_w > 0.1f ? (profile->requested_power_w * 1000.0f / 52.0f) : 180.0f;
    for (size_t i = 0; i < PW_MAX_CURRENT_SAMPLES; ++i) {
        float sample = base;
        if (i < 12) {
            sample += 150.0f + (float)i * 8.0f;
        } else if (i > 28 && i < 36) {
            sample += 420.0f;
        } else if (profile->mps_jitter && (i % 9u == 0u)) {
            sample -= 90.0f;
        } else {
            sample += (float)((int)(i % 5u) - 2) * 14.0f;
        }
        pw_signature_feed(sig, sample);
    }
    pw_signature_finalize(sig);
}

static void emit_lldp_sequence(const pw_profile_t *profile, pw_event_t *events, size_t *event_count)
{
    pw_lldp_profile_t tx = pw_lldp_default_profile("PoE Whisper Inline", profile->requested_power_w);
    uint8_t frame[PW_MAX_LLDP_PAYLOAD];
    char summary[120];
    size_t frame_len = pw_lldp_build_advertisement(&tx, frame, sizeof(frame));
    pw_lldp_profile_t parsed;
    if (pw_lldp_parse_summary(frame, frame_len, &parsed) == 0) {
        pw_lldp_format_summary(&parsed, summary, sizeof(summary));
        add_event(events, event_count, EVENT_LLDP_CAPTURED, 30u, summary);
    }

    if (profile->lldp_spoof) {
        parsed.requested_power_w += 5.0f;
        snprintf(parsed.system_name, sizeof(parsed.system_name), "camera-maint-inline");
        pw_lldp_format_summary(&parsed, summary, sizeof(summary));
        add_event(events, event_count, EVENT_LLDP_SPOOFED, 42u, summary);
    }
}

static void maybe_trigger_brownout(const pw_profile_t *profile, pw_relay_state_t *relay, pw_status_t *status, pw_event_t *events, size_t *event_count)
{
    if (!profile->brownout_ms) {
        return;
    }
    pw_event_t evt;
    if (pw_relay_schedule_brownout(relay, profile->brownout_ms, profile->brownout_target_v, &evt) == 0) {
        evt.timestamp_ms = 57u;
        if (*event_count < PW_MAX_EVENTS) {
            events[(*event_count)++] = evt;
        }
        for (uint32_t t = 0; t < profile->brownout_ms; t += 20u) {
            pw_relay_tick(relay, status, 20u);
        }
    }
}

static void enforce_safety(pw_status_t *status, pw_relay_state_t *relay, pw_event_t *events, size_t *event_count)
{
    if (status->board_temp_c > PW_SAFE_TEMP_C) {
        pw_event_t evt;
        pw_relay_force_bypass(relay, status, "thermal ceiling exceeded", &evt);
        evt.timestamp_ms = 88u;
        if (*event_count < PW_MAX_EVENTS) {
            events[(*event_count)++] = evt;
        }
        status->mode = PW_MODE_SAFE_ROLLBACK;
        status->thermal_shutdown = 1;
    }
    if (status->line_current_ma > PW_SAFE_CURRENT_MA) {
        pw_event_t evt;
        pw_relay_force_bypass(relay, status, "overcurrent ceiling exceeded", &evt);
        evt.timestamp_ms = 89u;
        if (*event_count < PW_MAX_EVENTS) {
            events[(*event_count)++] = evt;
        }
        status->mode = PW_MODE_SAFE_ROLLBACK;
    }
}

static void simulate_operator_protocol(const char *profile_name)
{
    uint8_t payload[64];
    uint8_t encoded[96];
    pw_radio_frame_t decoded;
    size_t profile_len = strlen(profile_name);
    memcpy(payload, profile_name, profile_len);
    size_t packet_len = pw_radio_encode(PW_OP_LOAD_PROFILE, payload, profile_len, encoded, sizeof(encoded));
    if (packet_len > 0 && pw_radio_decode(encoded, packet_len, &decoded) == 0) {
        printf("radio: opcode=%u payload='%.*s' crc_errors=%u\n",
               decoded.opcode,
               (int)decoded.length,
               decoded.payload,
               PW_RADIO.crc_errors);
    }
}

static void print_status(const pw_status_t *status, const pw_profile_t *profile, const pw_signature_result_t *sig)
{
    printf("status: mode=%d class=%s link=%u bypass=%u voltage=%.1fV current=%.0fmA power=%.1fW temp=%.1fC\n",
           status->mode,
           pw_port_class_name(status->detected_class),
           status->link_up,
           status->bypass_enabled,
           status->line_voltage_v,
           status->line_current_ma,
           status->allocated_power_w,
           status->board_temp_c);
    printf("profile: %s request=%.1fW brownout=%ums target=%.1fV lldp_spoof=%u mps_jitter=%u\n",
           profile->name,
           profile->requested_power_w,
           profile->brownout_ms,
           profile->brownout_target_v,
           profile->lldp_spoof,
           profile->mps_jitter);
    printf("signature: mean=%.1fmA peak=%.1fmA variance=%.1f inferred=%s anomaly_score=%.2f\n",
           sig->mean_ma,
           sig->peak_ma,
           sig->variance,
           sig->inferred_state,
           pw_signature_anomaly_score(sig, 320.0f));
}

static void print_events(const pw_event_t *events, size_t count)
{
    puts("events:");
    for (size_t i = 0; i < count; ++i) {
        printf("  [%03ums] code=%d %s\n", events[i].timestamp_ms, events[i].code, events[i].message);
    }
}

int main(int argc, char **argv)
{
    const char *vendor = argc > 1 ? argv[1] : "Cisco";
    const char *profile_name = argc > 2 ? argv[2] : "camera-reboot-window";

    print_banner();
    pw_radio_init();

    pw_status_t status;
    memset(&status, 0, sizeof(status));
    status.mode = PW_MODE_PROFILED;
    status.board_temp_c = 41.5f;

    pw_port_state_t port;
    pw_port_init(&port);

    pw_profile_t profile = build_profile(profile_name);
    pw_port_apply_profile(&port, &profile);

    pw_pse_fingerprint_t pse = pw_port_fingerprint_pse(vendor);

    pw_event_t events[PW_MAX_EVENTS];
    size_t event_count = 0;
    add_event(events, &event_count, EVENT_BOOT, 0u, "PoE Whisper booted in authorized research mode");
    pw_port_negotiate(&port, &pse, events, &event_count, PW_MAX_EVENTS);

    emit_lldp_sequence(&profile, events, &event_count);
    pw_port_sample(&port, &status, 25u);

    pw_signature_result_t sig;
    simulate_current_trace(&sig, &profile);
    char sig_msg[112];
    snprintf(sig_msg, sizeof(sig_msg), "Current signature inferred=%s peak=%.1fmA", sig.inferred_state, sig.peak_ma);
    add_event(events, &event_count, EVENT_CURRENT_PATTERN, 51u, sig_msg);

    pw_relay_state_t relay;
    pw_relay_init(&relay);
    maybe_trigger_brownout(&profile, &relay, &status, events, &event_count);

    if (profile.mps_jitter) {
        add_event(events, &event_count, EVENT_MPS_JITTER, 72u, "Maintain-power signature jitter enabled for research profile");
    }

    status.board_temp_c = profile.brownout_ms ? 46.0f : 39.0f;
    enforce_safety(&status, &relay, events, &event_count);
    status.mode = status.bypass_enabled ? PW_MODE_SAFE_ROLLBACK : PW_MODE_ACTIVE;

    simulate_operator_protocol(profile_name);
    print_status(&status, &profile, &sig);
    print_events(events, event_count);

    return 0;
}
