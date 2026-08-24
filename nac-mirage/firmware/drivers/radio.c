/*
 * nac-mirage operator radio link simulator
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "radio.h"

#include <stdio.h>
#include <string.h>

static bool g_connected = false;
static char g_operator_id[NM_TEXT_32];
static uint32_t g_last_publish_ms = 0U;

void radio_init(void)
{
    g_connected = false;
    memset(g_operator_id, 0, sizeof(g_operator_id));
    g_last_publish_ms = 0U;
}

void radio_pair(const char *operator_id)
{
    (void)snprintf(g_operator_id, sizeof(g_operator_id), "%s", (operator_id != NULL) ? operator_id : "unknown");
    g_connected = true;
    printf("radio: paired secure session with %s\n", g_operator_id);
}

bool radio_is_connected(void)
{
    return g_connected;
}

void radio_publish_status(const nm_runtime_status_t *status, const nm_policy_t *policy)
{
    if (!g_connected || status == NULL || policy == NULL) {
        return;
    }

    printf("radio-status: uptime=%u frames=%u mutated=%u dropped=%u vlan=%u poe=%umW profile=%s bypass=%s\n",
           status->uptime_ms,
           status->frames_seen,
           status->frames_mutated,
           status->frames_dropped,
           status->advertised_vlan,
           status->poe_budget_mw,
           policy->name,
           status->bypass_engaged ? "on" : "off");
}

void radio_publish_neighbor(const nm_neighbor_t *neighbor)
{
    if (!g_connected || neighbor == NULL) {
        return;
    }

    printf("radio-neighbor: system=%s vlan=%u voice=%u dot1x=%s power=%umW\n",
           neighbor->system_name,
           neighbor->vlan,
           neighbor->voice_vlan,
           neighbor->dot1x_required ? "yes" : "no",
           neighbor->power_mw);
}

void radio_tick(uint32_t now_ms, nm_runtime_status_t *status)
{
    if (!g_connected || status == NULL) {
        return;
    }

    if ((now_ms - g_last_publish_ms) >= 1000U) {
        g_last_publish_ms = now_ms;
        status->radio_connected = true;
    }
}
