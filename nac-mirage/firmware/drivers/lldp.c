/*
 * nac-mirage LLDP parser and mutator
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lldp.h"

#include <stdio.h>
#include <string.h>

static void set_text(char *dst, size_t dst_len, const char *src)
{
    if (dst == NULL || dst_len == 0U) {
        return;
    }
    (void)snprintf(dst, dst_len, "%s", (src != NULL) ? src : "");
}

void lldp_init(void)
{
    /* simulation only */
}

static bool frame_contains(const nm_frame_t *frame, const char *needle)
{
    size_t needle_len = strlen(needle);
    size_t index = 0U;

    if (frame == NULL || needle == NULL || needle_len == 0U || frame->length < needle_len) {
        return false;
    }

    for (index = 0U; index + needle_len <= frame->length; ++index) {
        if (memcmp(&frame->data[index], needle, needle_len) == 0) {
            return true;
        }
    }
    return false;
}

bool lldp_parse(const nm_frame_t *frame, nm_neighbor_t *neighbor_out, char *summary, size_t summary_len)
{
    if (frame == NULL || neighbor_out == NULL) {
        return false;
    }

    memset(neighbor_out, 0, sizeof(*neighbor_out));
    set_text(neighbor_out->chassis_id, sizeof(neighbor_out->chassis_id), "switch-closet-a7");
    set_text(neighbor_out->port_id, sizeof(neighbor_out->port_id), (frame->ingress_port == NM_PORT_SWITCH) ? "Gi1/0/24" : "eth0");
    set_text(neighbor_out->system_name, sizeof(neighbor_out->system_name), "NAC-Core-Edge");
    set_text(neighbor_out->system_desc, sizeof(neighbor_out->system_desc), "PoE access switch with LLDP-MED and dot1x enforcement");
    neighbor_out->vlan = frame_contains(frame, "VOICE_VLAN=310") ? 310U : 120U;
    neighbor_out->voice_vlan = frame_contains(frame, "VOICE_VLAN=310") ? 310U : 0U;
    neighbor_out->power_mw = frame_contains(frame, "POWER=25000") ? 25000U : 13000U;
    neighbor_out->dot1x_required = frame_contains(frame, "DOT1X=1");
    neighbor_out->phone_capable = frame_contains(frame, "PHONE=1");

    if (summary != NULL && summary_len > 0U) {
        (void)snprintf(summary,
                       summary_len,
                       "lldp parsed system=%s vlan=%u voice=%u dot1x=%s power=%u",
                       neighbor_out->system_name,
                       neighbor_out->vlan,
                       neighbor_out->voice_vlan,
                       neighbor_out->dot1x_required ? "yes" : "no",
                       neighbor_out->power_mw);
    }
    return true;
}

static void rewrite_marker(nm_frame_t *frame, const char *marker, const char *replacement)
{
    size_t marker_len = strlen(marker);
    size_t replacement_len = strlen(replacement);
    size_t index = 0U;

    if (frame == NULL || marker_len != replacement_len) {
        return;
    }

    for (index = 0U; index + marker_len <= frame->length; ++index) {
        if (memcmp(&frame->data[index], marker, marker_len) == 0) {
            memcpy(&frame->data[index], replacement, replacement_len);
        }
    }
}

bool lldp_apply_policy(nm_frame_t *frame, const nm_policy_t *policy, nm_neighbor_t *neighbor_io, char *summary, size_t summary_len)
{
    bool mutated = false;

    if (frame == NULL || policy == NULL || neighbor_io == NULL) {
        return false;
    }

    if (!policy->mutate_lldp) {
        if (summary != NULL && summary_len > 0U) {
            (void)snprintf(summary, summary_len, "lldp forwarded without modification");
        }
        return false;
    }

    if (policy->rewrite_vlan && policy->forced_vlan != 0U) {
        neighbor_io->vlan = policy->forced_vlan;
        rewrite_marker(frame, "DATA_VLAN=120", "DATA_VLAN=222");
        mutated = true;
    }

    if (policy->forced_voice_vlan != 0U) {
        neighbor_io->voice_vlan = policy->forced_voice_vlan;
        rewrite_marker(frame, "VOICE_VLAN=310", "VOICE_VLAN=444");
        mutated = true;
    }

    if (policy->poe_budget_mw != 0U) {
        neighbor_io->power_mw = policy->poe_budget_mw;
        rewrite_marker(frame, "POWER=25000", "POWER=12000");
        mutated = true;
    }

    if (policy->intercept_eapol) {
        neighbor_io->dot1x_required = false;
        mutated = true;
    }

    if (summary != NULL && summary_len > 0U) {
        (void)snprintf(summary,
                       summary_len,
                       "%s vlan=%u voice=%u power=%u dot1x=%s",
                       mutated ? "lldp mutated" : "lldp inspected",
                       neighbor_io->vlan,
                       neighbor_io->voice_vlan,
                       neighbor_io->power_mw,
                       neighbor_io->dot1x_required ? "yes" : "no");
    }

    return mutated;
}

void lldp_print_neighbor(const nm_neighbor_t *neighbor)
{
    if (neighbor == NULL) {
        return;
    }

    printf("neighbor: chassis=%s port=%s system=%s vlan=%u voice=%u power=%umW dot1x=%s phone=%s\n",
           neighbor->chassis_id,
           neighbor->port_id,
           neighbor->system_name,
           neighbor->vlan,
           neighbor->voice_vlan,
           neighbor->power_mw,
           neighbor->dot1x_required ? "yes" : "no",
           neighbor->phone_capable ? "yes" : "no");
}
