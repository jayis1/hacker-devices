/*
 * nac-mirage inline bridge simulator
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "bridge.h"
#include "capture.h"
#include "poe.h"
#include "radio.h"

#include <stdio.h>
#include <string.h>

static nm_policy_t g_active_policy;
static nm_frame_t g_scripted_frames[10];
static size_t g_scripted_count = 0U;
static size_t g_scripted_cursor = 0U;
static bool g_link_open = false;

static void stage_frame(size_t slot, uint16_t ethertype, uint8_t ingress, const char *payload)
{
    size_t payload_len = strlen(payload);

    g_scripted_frames[slot].ethertype = ethertype;
    g_scripted_frames[slot].ingress_port = ingress;
    g_scripted_frames[slot].length = (uint16_t)payload_len;
    memset(g_scripted_frames[slot].data, 0, sizeof(g_scripted_frames[slot].data));
    memcpy(g_scripted_frames[slot].data, payload, payload_len);
}

void bridge_init(void)
{
    memset(&g_active_policy, 0, sizeof(g_active_policy));
    memset(g_scripted_frames, 0, sizeof(g_scripted_frames));
    g_scripted_count = 0U;
    g_scripted_cursor = 0U;
    g_link_open = false;

    stage_frame(g_scripted_count++, NM_ETHERTYPE_LLDP, NM_PORT_SWITCH, "CHASSIS=switchA;PORT=Gi1/0/24;DATA_VLAN=120;VOICE_VLAN=310;POWER=25000;DOT1X=1;PHONE=1");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_EAPOL, NM_PORT_SWITCH, "EAPOL-START: identity request");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_IPV4, NM_PORT_ENDPOINT, "DHCP DISCOVER class=voice-handset vendor=polycom");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_LLDP, NM_PORT_ENDPOINT, "ENDPOINT LLDP-MED capabilities=phone location=lab3");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_ARP, NM_PORT_SWITCH, "ARP who-has 10.22.120.1 tell 10.22.120.44");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_EAPOL, NM_PORT_ENDPOINT, "EAPOL RESPONSE identity=MAC:00:11:22:33:44:55");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_IPV4, NM_PORT_SWITCH, "RADIUS accept tunnel-private-group-id=120");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_LLDP, NM_PORT_SWITCH, "CHASSIS=switchA;PORT=Gi1/0/24;DATA_VLAN=120;VOICE_VLAN=310;POWER=25000;DOT1X=1;PHONE=1");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_IPV4, NM_PORT_ENDPOINT, "SIP REGISTER user=helpdesk-phone");
    stage_frame(g_scripted_count++, NM_ETHERTYPE_IPV4, NM_PORT_SWITCH, "TFTP option66 phone-provisioner.local");
}

void bridge_apply_policy(const nm_policy_t *policy)
{
    if (policy == NULL) {
        return;
    }
    g_active_policy = *policy;
}

size_t bridge_pending_frames(void)
{
    if (g_scripted_cursor >= g_scripted_count) {
        return 0U;
    }
    return g_scripted_count - g_scripted_cursor;
}

static bool should_drop_frame(const nm_frame_t *frame, const nm_policy_t *policy)
{
    if (frame == NULL || policy == NULL) {
        return false;
    }

    if (policy->intercept_eapol && frame->ethertype == NM_ETHERTYPE_EAPOL) {
        return true;
    }

    if (policy->delay_link_up && !g_link_open && frame->ingress_port == NM_PORT_ENDPOINT) {
        return true;
    }

    return false;
}

static void summarize_frame(const nm_frame_t *frame, char *summary, size_t summary_len)
{
    const char *type = "UNKNOWN";
    if (frame == NULL || summary == NULL || summary_len == 0U) {
        return;
    }

    switch (frame->ethertype) {
    case NM_ETHERTYPE_LLDP:
        type = "LLDP";
        break;
    case NM_ETHERTYPE_EAPOL:
        type = "EAPOL";
        break;
    case NM_ETHERTYPE_IPV4:
        type = "IPv4";
        break;
    case NM_ETHERTYPE_ARP:
        type = "ARP";
        break;
    default:
        break;
    }

    (void)snprintf(summary,
                   summary_len,
                   "%s %s",
                   type,
                   (const char *)frame->data);
}

static void handle_lldp(uint32_t now_ms,
                        nm_frame_t *frame,
                        nm_runtime_status_t *status,
                        nm_policy_t *policy,
                        nm_neighbor_t *neighbor)
{
    char summary[NM_TEXT_128];
    bool parsed = lldp_parse(frame, neighbor, summary, sizeof(summary));
    if (parsed) {
        status->current_vlan = neighbor->vlan;
        status->advertised_vlan = neighbor->vlan;
        capture_record(now_ms, frame->ingress_port, NM_CAPTURE_REASON_FORWARD, frame->ethertype, frame->length, summary);
        radio_publish_neighbor(neighbor);
    }

    if (lldp_apply_policy(frame, policy, neighbor, summary, sizeof(summary))) {
        status->frames_mutated++;
        status->advertised_vlan = neighbor->vlan;
        capture_record(now_ms, frame->ingress_port, NM_CAPTURE_REASON_MUTATE, frame->ethertype, frame->length, summary);
    }
}

static void handle_generic(uint32_t now_ms,
                           const nm_frame_t *frame,
                           nm_runtime_status_t *status,
                           const nm_policy_t *policy)
{
    char summary[NM_TEXT_128];
    summarize_frame(frame, summary, sizeof(summary));

    if (should_drop_frame(frame, policy)) {
        status->frames_dropped++;
        capture_record(now_ms, frame->ingress_port, NM_CAPTURE_REASON_DROP, frame->ethertype, frame->length, summary);
        if (frame->ethertype == NM_ETHERTYPE_EAPOL) {
            status->alerts++;
        }
        return;
    }

    if (frame->ethertype == NM_ETHERTYPE_IPV4 && strstr((const char *)frame->data, "SIP REGISTER") != NULL) {
        status->alerts++;
        capture_record(now_ms, frame->ingress_port, NM_CAPTURE_REASON_ALERT, frame->ethertype, frame->length, "Observed SIP handset registration opportunity");
    } else {
        capture_record(now_ms, frame->ingress_port, NM_CAPTURE_REASON_FORWARD, frame->ethertype, frame->length, summary);
    }
}

void bridge_tick(uint32_t now_ms, nm_runtime_status_t *status, nm_policy_t *policy, nm_neighbor_t *neighbor)
{
    nm_frame_t *frame = NULL;

    if (status == NULL || policy == NULL || neighbor == NULL) {
        return;
    }

    if (policy->delay_link_up) {
        g_link_open = now_ms >= policy->link_delay_ms;
    } else {
        g_link_open = true;
    }

    status->relay_open = g_link_open;
    status->bypass_engaged = !g_link_open;

    if (g_scripted_cursor >= g_scripted_count) {
        return;
    }

    frame = &g_scripted_frames[g_scripted_cursor++];
    status->frames_seen++;

    if (frame->ethertype == NM_ETHERTYPE_LLDP) {
        handle_lldp(now_ms, frame, status, policy, neighbor);
    } else {
        handle_generic(now_ms, frame, status, policy);
    }

    if (frame->ingress_port == NM_PORT_ENDPOINT && strstr((const char *)frame->data, "voice-handset") != NULL) {
        poe_observe_draw(11800U, now_ms);
    }

    if (policy->mirror_traffic && radio_is_connected()) {
        radio_publish_status(status, policy);
    }
}
