/*
 * EtherCAT Phantom Control Plane Firmware Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "drivers/ethercat.h"
#include "drivers/fpga_mailbox.h"
#include "drivers/radio.h"
#include "drivers/power.h"
#include "drivers/rule_engine.h"
#include "drivers/storage.h"

static void eph_print_banner(void) {
    printf("==============================================================\n");
    printf(" EtherCAT Phantom — deterministic inline EtherCAT frame surgeon\n");
    printf(" Author: %s\n", EPH_AUTHOR);
    printf("==============================================================\n");
}

static void eph_init_runtime(eph_runtime_config_t *config, eph_metrics_t *metrics, eph_power_state_t *power) {
    memset(config, 0, sizeof(*config));
    memset(metrics, 0, sizeof(*metrics));
    memset(power, 0, sizeof(*power));

    config->role = EPH_ROLE_MANIPULATE;
    config->cycle_budget_ns = 700u;
    config->session_key = EPH_DEFAULT_SESSION_KEY;
    config->safe_mode = false;
    config->logging_enabled = true;
    config->radio_enabled = true;

    power->battery_mv = 4020u;
    power->soc_percent = 88u;
    power->usb_present = false;
    power->charging = false;
}

static void eph_boot_pipeline(eph_runtime_config_t *config, eph_metrics_t *metrics) {
    eph_storage_init();
    eph_power_init();
    eph_rule_engine_init();
    eph_rule_engine_load_defaults();
    eph_radio_init(config);
    eph_ethercat_init();
    eph_ethercat_seed_demo_frames();
    eph_fpga_init(config, metrics);
    eph_storage_store_config(config);
    eph_fpga_set_rules_hash(eph_rule_engine_hash());
}

static void eph_print_rules(void) {
    const eph_rule_t *rules = eph_rule_engine_rules();
    size_t count = eph_rule_engine_rule_count();
    size_t i;

    printf("[rules] loaded %zu rules\n", count);
    for (i = 0u; i < count; ++i) {
        printf("  #%u %-18s slave=%u object=0x%04X:%u action=%d a=%ld b=%ld enabled=%s\n",
               rules[i].index,
               rules[i].label,
               rules[i].matcher_slave,
               rules[i].matcher_index,
               rules[i].matcher_subindex,
               (int)rules[i].action,
               (long)rules[i].param_a,
               (long)rules[i].param_b,
               rules[i].enabled ? "yes" : "no");
    }
}

static void eph_process_stream(eph_runtime_config_t *config, eph_metrics_t *metrics, eph_power_state_t *power) {
    eph_capture_ring_t ring = {0};
    eph_frame_view_t frame;
    uint32_t epoch_ms = 0u;

    while (eph_ethercat_next_frame(&frame)) {
        eph_rule_result_t result;
        eph_capture_t drained[EPH_CAPTURE_DEPTH];
        eph_mailbox_command_t command;
        bool manipulation_active;
        size_t drained_count;
        uint8_t encoded[EPH_MAX_FRAME_BYTES];
        size_t encoded_len;

        metrics->frames_seen += 1u;

        if (frame.working_counter == 0u) {
            metrics->working_counter_faults += 1u;
        }

        eph_rule_engine_apply(&frame, &result, metrics, config->safe_mode);
        if (result.modified) {
            metrics->frames_modified += 1u;
        }

        eph_ethercat_capture(&ring, &frame, result.value_after, result.modified);
        eph_fpga_note_frame(&frame, result.modified);

        encoded_len = eph_ethercat_encode_frame(&frame, encoded, sizeof(encoded));
        if (encoded_len == 0u) {
            metrics->malformed_frames += 1u;
        }

        printf("[stream] cycle=%lu slave=%u obj=0x%04X:%u (%s) before=%ld after=%ld matched=%s modified=%s mailbox=%s\n",
               (unsigned long)metrics->frames_seen,
               frame.slave_address,
               frame.object_index,
               frame.object_subindex,
               eph_ethercat_object_name(frame.object_index, frame.object_subindex),
               (long)frame.process_value,
               (long)result.value_after,
               result.matched ? "yes" : "no",
               result.modified ? "yes" : "no",
               frame.has_mailbox ? "yes" : "no");

        if (result.rule != NULL) {
            printf("         rule=%s action=%d\n", result.rule->label, (int)result.rule->action);
        }

        manipulation_active = config->role == EPH_ROLE_MANIPULATE && result.modified;
        eph_power_tick(power, metrics, config->radio_enabled, manipulation_active);
        epoch_ms += 100u;
        eph_radio_tick(epoch_ms, metrics);

        if ((metrics->frames_seen % 4u) == 0u) {
            drained_count = eph_ethercat_drain_captures(&ring, drained, EPH_CAPTURE_DEPTH);
            eph_storage_append_captures(drained, drained_count);
            eph_radio_send_captures(drained, drained_count);
            eph_radio_send_status(config, metrics);
        }

        if (power->critical) {
            command.opcode = EPH_MB_FORCE_BYPASS;
            command.arg0 = 0u;
            command.arg1 = 0u;
            command.arg2 = 0u;
            eph_fpga_apply_command(&command);
        }
    }

    {
        eph_capture_t drained[EPH_CAPTURE_DEPTH];
        size_t drained_count = eph_ethercat_drain_captures(&ring, drained, EPH_CAPTURE_DEPTH);
        eph_storage_append_captures(drained, drained_count);
        eph_radio_send_captures(drained, drained_count);
    }
}

static void eph_print_summary(const eph_runtime_config_t *config, const eph_metrics_t *metrics, const eph_power_state_t *power) {
    eph_fpga_status_t status;
    eph_fpga_get_status(&status);

    printf("\n[summary]\n");
    printf("  role=%d safe_mode=%s logging=%s radio=%s\n",
           (int)config->role,
           config->safe_mode ? "yes" : "no",
           config->logging_enabled ? "yes" : "no",
           config->radio_enabled ? "yes" : "no");
    printf("  frames_seen=%lu frames_modified=%lu rules_triggered=%lu wc_faults=%lu malformed=%lu\n",
           (unsigned long)metrics->frames_seen,
           (unsigned long)metrics->frames_modified,
           (unsigned long)metrics->rules_triggered,
           (unsigned long)metrics->working_counter_faults,
           (unsigned long)metrics->malformed_frames);
    printf("  radio_packets=%lu mailbox_commands=%lu brownouts=%lu storage_records=%zu\n",
           (unsigned long)metrics->radio_packets,
           (unsigned long)metrics->mailbox_commands,
           (unsigned long)metrics->brownouts,
           eph_storage_log_count());
    printf("  fpga_ready=%s links=%s/%s bypass=%s last_cycle=%uns margin=%uns\n",
           status.ready ? "yes" : "no",
           status.link_up_a ? "up" : "down",
           status.link_up_b ? "up" : "down",
           status.bypass_engaged ? "yes" : "no",
           status.last_cycle_ns,
           status.timing_margin_ns);

    eph_power_print(power);
    eph_storage_dump_recent(6u);
}

int main(void) {
    eph_runtime_config_t config;
    eph_metrics_t metrics;
    eph_power_state_t power;

    eph_print_banner();
    eph_init_runtime(&config, &metrics, &power);
    eph_boot_pipeline(&config, &metrics);
    eph_print_rules();
    eph_process_stream(&config, &metrics, &power);
    eph_print_summary(&config, &metrics, &power);

    return 0;
}
