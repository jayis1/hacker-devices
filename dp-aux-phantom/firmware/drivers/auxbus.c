/*
 * DP AUX Phantom AUX bus driver simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "auxbus.h"
#include "capture.h"
#include "policy.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static dpa_aux_transaction_t g_scripted[] = {
    { .timestamp_ms = 50U,  .address = DPA_DPCD_REV,                .op = DPA_AUX_NATIVE_READ,  .length = 1U, .data = { 0x14U }, .note = "read DPCD revision" },
    { .timestamp_ms = 90U,  .address = DPA_DPCD_MAX_LINK_RATE,      .op = DPA_AUX_NATIVE_READ,  .length = 1U, .data = { DPA_LINK_RATE_HBR3 }, .note = "read max rate" },
    { .timestamp_ms = 110U, .address = DPA_DPCD_MAX_LANE_COUNT,     .op = DPA_AUX_NATIVE_READ,  .length = 1U, .data = { DPA_LANE_COUNT_4 }, .note = "read max lanes" },
    { .timestamp_ms = 160U, .address = DPA_DPCD_LINK_BW_SET,        .op = DPA_AUX_NATIVE_WRITE, .length = 1U, .data = { DPA_LINK_RATE_HBR3 }, .note = "host requests HBR3" },
    { .timestamp_ms = 220U, .address = DPA_DPCD_LANE_COUNT_SET,     .op = DPA_AUX_NATIVE_WRITE, .length = 1U, .data = { DPA_LANE_COUNT_4 }, .note = "host requests four lanes" },
    { .timestamp_ms = 280U, .address = DPA_DPCD_TRAINING_PATTERN_SET,.op = DPA_AUX_NATIVE_WRITE,.length = 1U, .data = { 0x21U }, .note = "training pattern 1" },
    { .timestamp_ms = 340U, .address = DPA_DPCD_TRAINING_PATTERN_SET,.op = DPA_AUX_NATIVE_WRITE,.length = 1U, .data = { 0x22U }, .note = "training pattern 2" },
    { .timestamp_ms = 410U, .address = DPA_DPCD_LANE0_1_STATUS,     .op = DPA_AUX_NATIVE_READ,  .length = 2U, .data = { 0x77U, 0x77U }, .note = "read lane status" },
    { .timestamp_ms = 470U, .address = 0x00050U,                    .op = DPA_AUX_I2C_READ,     .length = 16U, .data = { 0x00U,0xFFU,0xFFU,0xFFU,0xFFU,0xFFU,0xFFU,0x00U,'J','A','Y','0','0','0','1',0x01U }, .note = "read EDID block 0" },
    { .timestamp_ms = 550U, .address = DPA_DPCD_SINK_COUNT,         .op = DPA_AUX_NATIVE_READ,  .length = 1U, .data = { 0x01U }, .note = "read sink count" },
    { .timestamp_ms = 620U, .address = DPA_DPCD_EDP_CONFIG_CAP,     .op = DPA_AUX_NATIVE_READ,  .length = 1U, .data = { 0x00U }, .note = "read eDP config" },
    { .timestamp_ms = 700U, .address = 0x00050U,                    .op = DPA_AUX_I2C_READ,     .length = 16U, .data = { 0x4A,0x59,0x31,0x2D,0x44,0x4F,0x43,0x4B,0x2D,0x41,0x55,0x58,0x2D,0x30,0x31,0x00 }, .note = "read descriptor" }
};

static dpa_aux_transaction_t g_trace[DPA_MAX_TRACES];
static size_t g_trace_count = 0U;
static size_t g_next_index = 0U;

void auxbus_init(void)
{
    memset(g_trace, 0, sizeof(g_trace));
    g_trace_count = 0U;
    g_next_index = 0U;
}

size_t auxbus_pending_transactions(void)
{
    return (sizeof(g_scripted) / sizeof(g_scripted[0])) - g_next_index;
}

static void update_counters(const dpa_aux_transaction_t *tx, dpa_runtime_status_t *status)
{
    if (tx->op == DPA_AUX_NATIVE_READ) {
        ++status->aux_reads;
    } else if (tx->op == DPA_AUX_NATIVE_WRITE) {
        ++status->aux_writes;
    } else if (tx->op == DPA_AUX_I2C_READ) {
        ++status->i2c_reads;
    } else if (tx->op == DPA_AUX_I2C_WRITE) {
        ++status->i2c_writes;
    }
}

static void update_identity_from_transaction(const dpa_aux_transaction_t *tx,
                                             dpa_sink_identity_t *sink)
{
    if (tx->address == DPA_DPCD_REV && tx->length > 0U) {
        sink->dpcd_rev = tx->data[0];
    } else if (tx->address == DPA_DPCD_MAX_LINK_RATE && tx->length > 0U) {
        sink->max_link_rate = tx->data[0];
    } else if (tx->address == DPA_DPCD_MAX_LANE_COUNT && tx->length > 0U) {
        sink->lane_count = tx->data[0];
    } else if (tx->address == 0x00050U && tx->length >= 12U) {
        sink->edid_cached = true;
        snprintf(sink->serial, sizeof(sink->serial), "%c%c%c%c-%c%c%c%c",
                 tx->data[8], tx->data[9], tx->data[10], tx->data[11],
                 tx->data[12], tx->data[13], tx->data[14], tx->data[15]);
    }
}

static void record_trace(const dpa_aux_transaction_t *tx)
{
    if (g_trace_count >= DPA_MAX_TRACES) {
        return;
    }
    g_trace[g_trace_count++] = *tx;
}

void auxbus_tick(uint32_t now_ms,
                 dpa_runtime_status_t *status,
                 const dpa_policy_t *policy,
                 dpa_sink_identity_t *sink)
{
    dpa_aux_transaction_t tx;
    dpa_rule_t rules[DPA_MAX_RULES];
    dpa_policy_t profiles[DPA_MAX_PROFILES];

    (void)REG_DPA_AUX_CONTROL;
    (void)REG_DPA_AUX_STATUS;
    (void)REG_DPA_AUX_FIFO;

    if (status == NULL || policy == NULL || sink == NULL) {
        return;
    }

    policy_load_defaults(profiles, rules);

    while (g_next_index < (sizeof(g_scripted) / sizeof(g_scripted[0])) &&
           g_scripted[g_next_index].timestamp_ms <= now_ms) {
        tx = g_scripted[g_next_index++];
        update_counters(&tx, status);
        update_identity_from_transaction(&tx, sink);
        policy_apply_transaction(policy, rules, &tx, status, sink);
        if (tx.injected) {
            ++status->rule_hits;
        }
        record_trace(&tx);
        capture_log(DPA_EVENT_CAPTURE, tx.note, &tx);
    }
}

const dpa_aux_transaction_t *auxbus_trace_at(size_t index)
{
    if (index >= g_trace_count) {
        return NULL;
    }
    return &g_trace[index];
}

size_t auxbus_trace_count(void)
{
    return g_trace_count;
}

void auxbus_print_summary(const dpa_sink_identity_t *sink)
{
    size_t i = 0U;
    if (sink == NULL) {
        return;
    }

    puts("-- DP AUX Phantom AUX summary --");
    printf("sink=%s vendor=%s serial=%s dpcd_rev=0x%02X max_rate=0x%02X lanes=%u mst=%s hdcp=%s hpd=%s cached_edid=%s\n",
           sink->sink_name,
           sink->vendor,
           sink->serial,
           sink->dpcd_rev,
           sink->max_link_rate,
           sink->lane_count,
           sink->mst_capable ? "yes" : "no",
           sink->hdcp_capable ? "yes" : "no",
           sink->hpd_high ? "yes" : "no",
           sink->edid_cached ? "yes" : "no");

    for (i = 0U; i < g_trace_count; ++i) {
        printf("trace[%02zu] t=%ums addr=0x%05X op=0x%X len=%u injected=%s note=%s\n",
               i,
               g_trace[i].timestamp_ms,
               g_trace[i].address,
               g_trace[i].op,
               g_trace[i].length,
               g_trace[i].injected ? "yes" : "no",
               g_trace[i].note);
    }
}
