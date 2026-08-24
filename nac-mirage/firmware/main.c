/*
 * NAC Mirage firmware simulation entrypoint
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "board.h"
#include "registers.h"
#include "drivers/bridge.h"
#include "drivers/capture.h"
#include "drivers/lldp.h"
#include "drivers/poe.h"
#include "drivers/radio.h"

#include <stdio.h>
#include <string.h>

static void policy_load_defaults(nm_policy_t policies[NM_PROFILE_COUNT])
{
    memset(policies, 0, sizeof(nm_policy_t) * NM_PROFILE_COUNT);

    policies[0] = (nm_policy_t){
        .profile_id = NM_PROFILE_TRANSPARENT,
        .name = "transparent",
        .mutate_lldp = false,
        .intercept_eapol = false,
        .delay_link_up = false,
        .enable_poe_glitch = false,
        .rewrite_vlan = false,
        .mirror_traffic = true,
        .forced_vlan = 0U,
        .forced_voice_vlan = 0U,
        .poe_budget_mw = 13000U,
        .link_delay_ms = 0U
    };

    policies[1] = (nm_policy_t){
        .profile_id = NM_PROFILE_LAB_PHANTOM_PHONE,
        .name = "lab-phantom-phone",
        .mutate_lldp = true,
        .intercept_eapol = false,
        .delay_link_up = false,
        .enable_poe_glitch = false,
        .rewrite_vlan = true,
        .mirror_traffic = true,
        .forced_vlan = 222U,
        .forced_voice_vlan = 444U,
        .poe_budget_mw = 12000U,
        .link_delay_ms = 0U
    };

    policies[2] = (nm_policy_t){
        .profile_id = NM_PROFILE_CAMERA_BROWNOUT,
        .name = "camera-brownout",
        .mutate_lldp = true,
        .intercept_eapol = false,
        .delay_link_up = false,
        .enable_poe_glitch = true,
        .rewrite_vlan = false,
        .mirror_traffic = true,
        .forced_vlan = 120U,
        .forced_voice_vlan = 0U,
        .poe_budget_mw = 9000U,
        .link_delay_ms = 0U
    };

    policies[3] = (nm_policy_t){
        .profile_id = NM_PROFILE_VOICE_VLAN_DECOY,
        .name = "voice-vlan-decoy",
        .mutate_lldp = true,
        .intercept_eapol = false,
        .delay_link_up = false,
        .enable_poe_glitch = false,
        .rewrite_vlan = true,
        .mirror_traffic = true,
        .forced_vlan = 333U,
        .forced_voice_vlan = 555U,
        .poe_budget_mw = 15000U,
        .link_delay_ms = 0U
    };

    policies[4] = (nm_policy_t){
        .profile_id = NM_PROFILE_NAC_DELAY,
        .name = "nac-delay",
        .mutate_lldp = false,
        .intercept_eapol = true,
        .delay_link_up = true,
        .enable_poe_glitch = false,
        .rewrite_vlan = false,
        .mirror_traffic = true,
        .forced_vlan = 0U,
        .forced_voice_vlan = 0U,
        .poe_budget_mw = 13000U,
        .link_delay_ms = 1800U
    };

    policies[5] = (nm_policy_t){
        .profile_id = NM_PROFILE_STAGED_RELAY,
        .name = "staged-relay",
        .mutate_lldp = true,
        .intercept_eapol = true,
        .delay_link_up = true,
        .enable_poe_glitch = true,
        .rewrite_vlan = true,
        .mirror_traffic = true,
        .forced_vlan = 222U,
        .forced_voice_vlan = 444U,
        .poe_budget_mw = 11000U,
        .link_delay_ms = 1200U
    };
}

static void print_banner(void)
{
    puts("NAC Mirage — Inline PoE / LLDP / NAC Deception Bridge");
    puts("Author: jayis1");
    puts("Authorized use only.");
    printf("Simulated registers: BRIDGE=0x%08X POE=0x%08X RADIO=0x%08X\n",
           REG_BRIDGE_CONTROL,
           REG_POE_CONTROL,
           REG_RADIO_STATUS);
}

static void print_policy(const nm_policy_t *policy)
{
    if (policy == NULL) {
        return;
    }

    printf("policy: id=%u name=%s mutate_lldp=%s intercept_eapol=%s delay=%s poe_glitch=%s mirror=%s vlan=%u voice=%u poe=%u delay_ms=%u\n",
           policy->profile_id,
           policy->name,
           policy->mutate_lldp ? "yes" : "no",
           policy->intercept_eapol ? "yes" : "no",
           policy->delay_link_up ? "yes" : "no",
           policy->enable_poe_glitch ? "yes" : "no",
           policy->mirror_traffic ? "yes" : "no",
           policy->forced_vlan,
           policy->forced_voice_vlan,
           policy->poe_budget_mw,
           policy->link_delay_ms);
}

static void runtime_init(nm_runtime_status_t *status)
{
    if (status == NULL) {
        return;
    }
    memset(status, 0, sizeof(*status));
    status->poe_budget_mw = 13000U;
}

static void print_summary(const nm_runtime_status_t *status, const nm_neighbor_t *neighbor)
{
    if (status == NULL || neighbor == NULL) {
        return;
    }

    puts("-- NAC Mirage summary --");
    printf("uptime=%u ms\n", status->uptime_ms);
    printf("frames_seen=%u\n", status->frames_seen);
    printf("frames_mutated=%u\n", status->frames_mutated);
    printf("frames_dropped=%u\n", status->frames_dropped);
    printf("alerts=%u\n", status->alerts);
    printf("observed_vlan=%u advertised_vlan=%u\n", status->current_vlan, status->advertised_vlan);
    printf("poe_budget=%umW relay_open=%s bypass=%s radio=%s\n",
           status->poe_budget_mw,
           status->relay_open ? "yes" : "no",
           status->bypass_engaged ? "yes" : "no",
           status->radio_connected ? "yes" : "no");
    lldp_print_neighbor(neighbor);
    printf("capture-forward=%u mutate=%u drop=%u alert=%u total=%zu\n",
           capture_reason_count(NM_CAPTURE_REASON_FORWARD),
           capture_reason_count(NM_CAPTURE_REASON_MUTATE),
           capture_reason_count(NM_CAPTURE_REASON_DROP),
           capture_reason_count(NM_CAPTURE_REASON_ALERT),
           capture_count());
}

int main(void)
{
    nm_policy_t policies[NM_PROFILE_COUNT];
    nm_policy_t *active_policy = NULL;
    nm_runtime_status_t status;
    nm_neighbor_t neighbor;
    uint32_t now_ms = 0U;

    print_banner();
    policy_load_defaults(policies);
    active_policy = &policies[NM_PROFILE_STAGED_RELAY];
    print_policy(active_policy);

    runtime_init(&status);
    memset(&neighbor, 0, sizeof(neighbor));

    capture_init();
    lldp_init();
    poe_init();
    radio_init();
    bridge_init();

    radio_pair("operator-lab-a");
    poe_configure(active_policy);
    bridge_apply_policy(active_policy);

    while (bridge_pending_frames() > 0U) {
        now_ms += 400U;
        status.uptime_ms = now_ms;
        bridge_tick(now_ms, &status, active_policy, &neighbor);
        poe_tick(now_ms, &status);
        radio_tick(now_ms, &status);
    }

    radio_publish_status(&status, active_policy);
    print_summary(&status, &neighbor);
    capture_dump();

    return 0;
}
