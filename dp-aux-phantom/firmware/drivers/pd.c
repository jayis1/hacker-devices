/*
 * DP AUX Phantom USB-C / PD manager
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "pd.h"
#include "capture.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static bool g_attached = false;
static uint32_t g_last_hpd_ms = 0U;

void pd_init(void)
{
    g_attached = false;
    g_last_hpd_ms = 0U;
}

void pd_attach_sink(dpa_sink_identity_t *sink)
{
    if (sink == NULL) {
        return;
    }

    g_attached = true;
    snprintf(sink->sink_name, sizeof(sink->sink_name), "27in Research Panel");
    snprintf(sink->vendor, sizeof(sink->vendor), "AUX");
    snprintf(sink->serial, sizeof(sink->serial), "JAYIS1-DP-042");
    sink->dpcd_rev = 0x14U;
    sink->max_link_rate = DPA_LINK_RATE_HBR3;
    sink->lane_count = DPA_LANE_COUNT_4;
    sink->mst_capable = true;
    sink->hdcp_capable = true;
    sink->hpd_high = true;
    sink->edid_cached = false;
    capture_log(DPA_EVENT_INFO, "sink attached on USB-C alt-mode path", NULL);
}

void pd_apply_policy(const dpa_policy_t *policy, dpa_runtime_status_t *status)
{
    if (policy == NULL || status == NULL) {
        return;
    }
    status->active_profile = policy->profile_id;
    status->negotiated_link_rate = policy->forced_link_rate;
    status->negotiated_lane_count = policy->forced_lane_count;
    status->sink_present = g_attached;
    status->hpd_asserted = g_attached;
    status->capture_armed = true;
}

static void maybe_bounce_hpd(uint32_t now_ms,
                             dpa_runtime_status_t *status,
                             const dpa_policy_t *policy,
                             dpa_sink_identity_t *sink)
{
    if (!policy->bounce_hpd || !g_attached || sink == NULL || status == NULL) {
        return;
    }

    if ((now_ms - g_last_hpd_ms) >= 900U) {
        g_last_hpd_ms = now_ms;
        sink->hpd_high = !sink->hpd_high;
        status->hpd_asserted = sink->hpd_high;
        capture_log(DPA_EVENT_ALERT,
                    sink->hpd_high ? "HPD asserted by policy pulse" : "HPD dropped by policy pulse",
                    NULL);
    }
}

static void maybe_downgrade_link(const dpa_policy_t *policy,
                                 dpa_runtime_status_t *status,
                                 dpa_sink_identity_t *sink)
{
    if (!policy->slow_link_training || sink == NULL || status == NULL) {
        return;
    }

    sink->max_link_rate = DPA_LINK_RATE_HBR;
    sink->lane_count = DPA_LANE_COUNT_2;
    status->negotiated_link_rate = DPA_LINK_RATE_HBR;
    status->negotiated_lane_count = DPA_LANE_COUNT_2;
}

void pd_tick(uint32_t now_ms,
             dpa_runtime_status_t *status,
             const dpa_policy_t *policy,
             dpa_sink_identity_t *sink)
{
    (void)REG_DPA_PD_STATUS;
    (void)REG_DPA_HPD_CONTROL;

    if (status == NULL || policy == NULL || sink == NULL) {
        return;
    }

    status->sink_present = g_attached;
    if (!g_attached) {
        return;
    }

    maybe_downgrade_link(policy, status, sink);
    maybe_bounce_hpd(now_ms, status, policy, sink);

    if (policy->emulate_dock) {
        sink->mst_capable = true;
        sink->hdcp_capable = false;
        capture_log(DPA_EVENT_INFO, "advertising dock-oriented capability set", NULL);
    }
}
