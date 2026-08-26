/*
 * DP AUX Phantom radio backhaul
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "radio.h"
#include "capture.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static char g_peer[DPA_TEXT_64];
static bool g_paired = false;

void radio_init(void)
{
    memset(g_peer, 0, sizeof(g_peer));
    g_paired = false;
}

void radio_pair(const char *peer_name)
{
    if (peer_name == NULL) {
        return;
    }
    snprintf(g_peer, sizeof(g_peer), "%s", peer_name);
    g_paired = true;
    capture_log(DPA_EVENT_INFO, "paired BLE control plane", NULL);
}

void radio_tick(uint32_t now_ms, dpa_runtime_status_t *status)
{
    (void)REG_DPA_RADIO_STATUS;
    if (status == NULL) {
        return;
    }
    status->radio_connected = g_paired;
    if (g_paired && (now_ms % 1200U) == 0U) {
        capture_log(DPA_EVENT_CAPTURE, "published incremental telemetry snapshot", NULL);
    }
}

void radio_publish_status(const dpa_runtime_status_t *status,
                          const dpa_policy_t *policy,
                          const dpa_sink_identity_t *sink)
{
    if (status == NULL || policy == NULL || sink == NULL) {
        return;
    }

    printf("radio peer=%s connected=%s profile=%s sink=%s vendor=%s link_rate=0x%02X lanes=%u alerts=%u rules=%u aux_reads=%u aux_writes=%u\n",
           g_paired ? g_peer : "unpaired",
           g_paired ? "yes" : "no",
           policy->name,
           sink->sink_name,
           sink->vendor,
           status->negotiated_link_rate,
           status->negotiated_lane_count,
           status->alerts,
           status->rule_hits,
           status->aux_reads,
           status->aux_writes);
}
