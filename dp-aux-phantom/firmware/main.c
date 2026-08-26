/*
 * DP AUX Phantom firmware simulation entrypoint
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "board.h"
#include "registers.h"
#include "drivers/auxbus.h"
#include "drivers/capture.h"
#include "drivers/pd.h"
#include "drivers/policy.h"
#include "drivers/radio.h"

#include <stdio.h>
#include <string.h>

static void print_banner(void)
{
    puts("DP AUX Phantom — inline USB-C / DisplayPort AUX instrumentation platform");
    puts("Author: jayis1");
    puts("Authorized use only.");
    printf("registers aux=0x%08X pd=0x%08X radio=0x%08X fpga=0x%08X\n",
           REG_DPA_AUX_CONTROL,
           REG_DPA_PD_CONTROL,
           REG_DPA_RADIO_STATUS,
           REG_DPA_FPGA_STATUS);
}

static void runtime_init(dpa_runtime_status_t *status)
{
    memset(status, 0, sizeof(*status));
    status->capture_armed = true;
}

static void print_runtime(const dpa_runtime_status_t *status)
{
    if (status == NULL) {
        return;
    }

    puts("-- DP AUX Phantom runtime --");
    printf("uptime=%ums profile=%u sink_present=%s hpd=%s radio=%s capture=%s\n",
           status->uptime_ms,
           status->active_profile,
           status->sink_present ? "yes" : "no",
           status->hpd_asserted ? "yes" : "no",
           status->radio_connected ? "yes" : "no",
           status->capture_armed ? "yes" : "no");
    printf("aux_reads=%u aux_writes=%u i2c_reads=%u i2c_writes=%u alerts=%u rule_hits=%u edid_overrides=%u link_training_steps=%u\n",
           status->aux_reads,
           status->aux_writes,
           status->i2c_reads,
           status->i2c_writes,
           status->alerts,
           status->rule_hits,
           status->edid_overrides,
           status->link_training_steps);
    printf("negotiated_rate=0x%02X negotiated_lanes=%u\n",
           status->negotiated_link_rate,
           status->negotiated_lane_count);
}

static void print_research_notes(const dpa_policy_t *policy, const dpa_sink_identity_t *sink)
{
    if (policy == NULL || sink == NULL) {
        return;
    }

    puts("-- operator notes --");
    printf("profile=%s sink=%s vendor=%s serial=%s\n",
           policy->name,
           sink->sink_name,
           sink->vendor,
           sink->serial);
    puts("This simulation demonstrates how an inline AUX interceptor can observe DPCD training, mutate EDID fields, and safely pulse HPD while keeping the host and sink electrically isolated through a retimer/FPGA boundary.");
    puts("All features are intended for authorized hardware security assessment, interoperability testing, and red-team lab validation only.");
}

int main(void)
{
    dpa_policy_t profiles[DPA_MAX_PROFILES];
    dpa_rule_t rules[DPA_MAX_RULES];
    const dpa_policy_t *active = NULL;
    dpa_runtime_status_t status;
    dpa_sink_identity_t sink;
    uint32_t now_ms = 0U;

    print_banner();
    policy_load_defaults(profiles, rules);
    active = policy_find(profiles, DPA_PROFILE_DOCK_EMULATOR);
    if (active == NULL) {
        fprintf(stderr, "failed to select default profile\n");
        return 1;
    }

    policy_print(active);
    runtime_init(&status);
    memset(&sink, 0, sizeof(sink));

    capture_init();
    auxbus_init();
    pd_init();
    radio_init();

    pd_attach_sink(&sink);
    pd_apply_policy(active, &status);
    radio_pair("lab-console-01");

    while (auxbus_pending_transactions() > 0U) {
        now_ms += 100U;
        status.uptime_ms = now_ms;
        pd_tick(now_ms, &status, active, &sink);
        auxbus_tick(now_ms, &status, active, &sink);
        radio_tick(now_ms, &status);
    }

    print_runtime(&status);
    auxbus_print_summary(&sink);
    radio_publish_status(&status, active, &sink);
    print_research_notes(active, &sink);
    printf("capture_total=%zu info=%zu rules=%zu alerts=%zu captures=%zu\n",
           capture_count(),
           capture_type_count(DPA_EVENT_INFO),
           capture_type_count(DPA_EVENT_RULE_HIT),
           capture_type_count(DPA_EVENT_ALERT),
           capture_type_count(DPA_EVENT_CAPTURE));
    capture_dump();
    return 0;
}
