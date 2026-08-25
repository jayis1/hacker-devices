/*
 * MoCA Phantom Control Plane Firmware Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "registers.h"
#include "drivers/coax_monitor.h"
#include "drivers/moca.h"
#include "drivers/power.h"
#include "drivers/radio.h"
#include "drivers/rule_engine.h"
#include "drivers/storage.h"

static void mph_print_banner(void) {
    printf("==============================================================\n");
    printf(" %s — covert MoCA coax assessment bridge\n", MPH_DEVICE_NAME);
    printf(" Author: %s\n", MPH_AUTHOR);
    printf("==============================================================\n");
}

static void mph_init_runtime(mph_runtime_config_t *config,
                             mph_metrics_t *metrics,
                             mph_power_state_t *power,
                             mph_spectrum_snapshot_t *spectrum) {
    memset(config, 0, sizeof(*config));
    memset(metrics, 0, sizeof(*metrics));
    memset(power, 0, sizeof(*power));
    memset(spectrum, 0, sizeof(*spectrum));

    config->mode = MPH_MODE_INLINE;
    config->safe_mode = false;
    config->radio_enabled = true;
    config->inline_bridge_enabled = true;
    config->spectrum_watch_enabled = true;
    config->target_network_id = 0x241u;
    config->privacy_key = MPH_DEFAULT_PRIVACY_KEY;
    config->latency_budget_us = 400u;
    config->operator_channel = 3u;
    strncpy(config->active_profile, "hotel-riser-audit", sizeof(config->active_profile) - 1u);

    power->battery_mv = 4030u;
    power->soc_percent = 91u;
    power->usb_present = false;
    power->charging = false;
}

static void mph_boot_pipeline(mph_runtime_config_t *config, mph_metrics_t *metrics) {
    mph_storage_init();
    mph_power_init();
    mph_rule_engine_init();
    mph_rule_engine_load_defaults();
    mph_radio_init(config);
    mph_coax_monitor_init();
    mph_moca_init(config, metrics);
    mph_moca_seed_demo_frames();
    mph_storage_store_config(config);
}

static void mph_print_rules(void) {
    const mph_rule_t *rules = mph_rule_engine_rules();
    size_t count = mph_rule_engine_rule_count();
    size_t i;

    printf("[rules] loaded %zu rules hash=0x%08lx\n", count, (unsigned long)mph_rule_engine_hash());
    for (i = 0u; i < count; ++i) {
        printf("  #%u %-26s src=%u dst=%u class=%d ch=%u action=%d a=%lu b=%lu enabled=%s\n",
               rules[i].index,
               rules[i].label,
               rules[i].match_src,
               rules[i].match_dst,
               (int)rules[i].match_class,
               rules[i].match_channel,
               (int)rules[i].action,
               (unsigned long)rules[i].param_a,
               (unsigned long)rules[i].param_b,
               rules[i].enabled ? "yes" : "no");
    }
}

static void mph_discover_and_print_nodes(mph_metrics_t *metrics, uint32_t network_id) {
    mph_node_t nodes[MPH_MAX_NODES];
    size_t count = 0u;
    mph_moca_discover_nodes(nodes, &count, network_id, metrics);
    mph_moca_print_topology(nodes, count);
}

static void mph_process_stream(mph_runtime_config_t *config,
                               mph_metrics_t *metrics,
                               mph_power_state_t *power,
                               mph_spectrum_snapshot_t *spectrum) {
    mph_capture_ring_t ring = {0};
    mph_frame_t frame;
    uint32_t epoch_ms = 0u;

    while (mph_moca_next_frame(&frame)) {
        mph_rule_result_t result;
        mph_frame_t drained[MPH_CAPTURE_DEPTH];
        uint8_t encoded[MPH_MAX_CAPTURE_BYTES + 16u];
        size_t encoded_len;
        bool injection_active;
        size_t drained_count;

        metrics->frames_seen += 1u;

        if (frame.traffic_class == MPH_TRAFFIC_BEACON && frame.dst_node == 1u) {
            metrics->alerts_raised += 1u;
        }

        mph_rule_engine_apply(&frame, &result, metrics, config->safe_mode);
        if (result.modified) {
            metrics->frames_modified += 1u;
        }

        if (result.dropped) {
            printf("[stream] ts=%lu src=%u dst=%u class=%s action=drop rule=%s\n",
                   (unsigned long)frame.timestamp_ms,
                   frame.src_node,
                   frame.dst_node,
                   mph_moca_class_name(frame.traffic_class),
                   result.rule != NULL ? result.rule->label : "none");
            continue;
        }

        mph_moca_capture(&ring, &frame, result.modified);
        encoded_len = mph_moca_encode_frame(&frame, encoded, sizeof(encoded));
        if (encoded_len == 0u) {
            frame.anomaly = true;
            metrics->alerts_raised += 1u;
        }

        printf("[stream] ts=%lu src=%u dst=%u class=%-10s rate=%4lu key=0x%04lx enc=%s mgmt=%s modified=%s alert=%s latency=%luus\n",
               (unsigned long)frame.timestamp_ms,
               frame.src_node,
               frame.dst_node,
               mph_moca_class_name(frame.traffic_class),
               (unsigned long)frame.phy_rate_mbps,
               (unsigned long)frame.privacy_key_id,
               frame.encrypted ? "yes" : "no",
               frame.management ? "yes" : "no",
               result.modified ? "yes" : "no",
               frame.anomaly || result.alerted ? "yes" : "no",
               (unsigned long)result.latency_added_us);

        if (result.rule != NULL) {
            printf("         rule=%s rewritten_dst=%u rewritten_key=0x%04lx rewritten_rate=%lu\n",
                   result.rule->label,
                   result.rewritten_dst,
                   (unsigned long)result.rewritten_key,
                   (unsigned long)result.rewritten_rate);
        }

        injection_active = config->mode == MPH_MODE_INJECT || result.modified;
        epoch_ms += 100u;
        mph_coax_monitor_tick(epoch_ms, spectrum, metrics);
        mph_power_tick(power, metrics, config->radio_enabled, injection_active, config->spectrum_watch_enabled);
        mph_radio_tick(epoch_ms, metrics);

        if (power->critical && !config->safe_mode) {
            config->safe_mode = true;
            printf("[guard] power critical, asserting safe mode and relay bypass bits=0x%08lx\n",
                   (unsigned long)mph_reg_mask(MPH_REG_BYPASS_CONTROL, MPH_BYPASS_SAFE_ASSERT | MPH_BYPASS_FORCE_ENABLE));
        }

        if ((metrics->frames_seen % 3u) == 0u) {
            drained_count = mph_moca_drain_captures(&ring, drained, MPH_CAPTURE_DEPTH);
            mph_storage_append_captures(drained, drained_count, metrics);
            mph_radio_send_captures(drained, drained_count, metrics);
            mph_radio_send_status(config, metrics, spectrum);
            mph_coax_monitor_print(spectrum);
        }
    }

    {
        mph_frame_t drained[MPH_CAPTURE_DEPTH];
        size_t drained_count = mph_moca_drain_captures(&ring, drained, MPH_CAPTURE_DEPTH);
        mph_storage_append_captures(drained, drained_count, metrics);
        mph_radio_send_captures(drained, drained_count, metrics);
    }
}

static void mph_print_summary(const mph_runtime_config_t *config,
                              const mph_metrics_t *metrics,
                              const mph_power_state_t *power,
                              const mph_spectrum_snapshot_t *spectrum) {
    printf("\n[summary]\n");
    printf("  profile=%s mode=%d safe_mode=%s radio=%s spectrum=%s network=0x%03lx latency_budget=%uus\n",
           config->active_profile,
           (int)config->mode,
           config->safe_mode ? "yes" : "no",
           config->radio_enabled ? "yes" : "no",
           config->spectrum_watch_enabled ? "yes" : "no",
           (unsigned long)config->target_network_id,
           config->latency_budget_us);
    printf("  frames_seen=%lu modified=%lu dropped=%lu alerts=%lu privacy_violations=%lu nodes=%lu\n",
           (unsigned long)metrics->frames_seen,
           (unsigned long)metrics->frames_modified,
           (unsigned long)metrics->frames_dropped,
           (unsigned long)metrics->alerts_raised,
           (unsigned long)metrics->privacy_violations,
           (unsigned long)metrics->nodes_discovered);
    printf("  rate_caps=%lu spectrum_peaks=%lu radio_packets=%lu storage_records=%lu battery_warnings=%lu bypass_events=%lu\n",
           (unsigned long)metrics->rate_cap_events,
           (unsigned long)metrics->spectrum_peaks,
           (unsigned long)metrics->radio_packets,
           (unsigned long)metrics->storage_records,
           (unsigned long)metrics->battery_warnings,
           (unsigned long)metrics->bypass_events);
    mph_coax_monitor_print(spectrum);
    mph_power_print(power);
    mph_storage_dump_recent(6u);
}

int main(void) {
    mph_runtime_config_t config;
    mph_metrics_t metrics;
    mph_power_state_t power;
    mph_spectrum_snapshot_t spectrum;

    mph_print_banner();
    mph_init_runtime(&config, &metrics, &power, &spectrum);
    mph_boot_pipeline(&config, &metrics);
    mph_print_rules();
    mph_discover_and_print_nodes(&metrics, config.target_network_id);
    mph_process_stream(&config, &metrics, &power, &spectrum);
    mph_print_summary(&config, &metrics, &power, &spectrum);

    return 0;
}
