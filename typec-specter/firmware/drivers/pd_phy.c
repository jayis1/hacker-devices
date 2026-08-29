/*
 * Type-C Specter PD PHY abstraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <string.h>
#include <stdio.h>
#include "pd_phy.h"

static bool g_proxy_enabled;
static bool g_mutation_enabled;
static bool g_debug_accessory;
static uint32_t g_seq;
static char g_identity[TS_PROFILE_NAME_LEN] = "transparent-proxy";
static ts_power_role_t g_requested_role = TS_ROLE_NONE;

void pd_phy_init(void) {
    g_proxy_enabled = true;
    g_mutation_enabled = false;
    g_debug_accessory = false;
    g_seq = 1;
    g_requested_role = TS_ROLE_NONE;
    snprintf(g_identity, sizeof(g_identity), "transparent-proxy");
}

void pd_phy_set_proxy_mode(bool enabled) {
    g_proxy_enabled = enabled;
}

void pd_phy_set_mutation_enabled(bool enabled) {
    g_mutation_enabled = enabled;
}

ts_attach_state_t pd_phy_poll_attach(void) {
    ts_attach_state_t st;
    memset(&st, 0, sizeof(st));

    if (g_seq < 4) {
        g_seq++;
        return st;
    }

    st.target_present = true;
    st.peer_present = true;
    st.target_role = TS_ROLE_SINK;
    st.peer_role = TS_ROLE_SOURCE;
    st.data_role = TS_DATA_ROLE_DEVICE;

    if (!g_proxy_enabled) {
        st.peer_present = false;
    }
    if (g_requested_role == TS_ROLE_SOURCE) {
        st.target_role = TS_ROLE_SOURCE;
        st.peer_role = TS_ROLE_SINK;
        st.data_role = TS_DATA_ROLE_HOST;
    }
    if (g_debug_accessory) {
        st.target_role = TS_ROLE_DEBUG_ACCESSORY;
        st.data_role = TS_DATA_ROLE_DEBUG;
    }

    g_seq++;
    return st;
}

static void fill_identity_packet(ts_pd_packet_t *packet) {
    packet->header = 0x41A1u;
    packet->payload_len = 4;
    packet->payload[0] = 0x54595045u;
    packet->payload[1] = 0x43535045u;
    packet->payload[2] = (uint32_t)strlen(g_identity);
    packet->payload[3] = g_mutation_enabled ? 0x0000F1F1u : 0x00000A11u;
}

bool pd_phy_receive_packet(ts_pd_packet_t *packet) {
    if (packet == NULL) {
        return false;
    }
    if ((g_seq % 5u) != 0u) {
        return false;
    }

    memset(packet, 0, sizeof(*packet));
    packet->seq = g_seq;
    packet->mutated = g_mutation_enabled;

    if ((g_seq % 10u) == 0u) {
        fill_identity_packet(packet);
    } else if (g_requested_role != TS_ROLE_NONE) {
        packet->header = 0x19B2u;
        packet->payload_len = 2;
        packet->payload[0] = (uint32_t)g_requested_role;
        packet->payload[1] = 0xABCD0001u;
    } else {
        packet->header = 0x1021u;
        packet->payload_len = 1;
        packet->payload[0] = g_mutation_enabled ? 0x0000DEADu : 0x0000BEEFu;
    }

    g_seq++;
    return true;
}

void pd_phy_load_identity_profile(const char *name) {
    if (name == NULL || name[0] == '\0') {
        snprintf(g_identity, sizeof(g_identity), "transparent-proxy");
        return;
    }
    snprintf(g_identity, sizeof(g_identity), "%s", name);
}

void pd_phy_request_role_swap(ts_power_role_t desired_role) {
    g_requested_role = desired_role;
}

void pd_phy_assert_debug_accessory(bool asserted) {
    g_debug_accessory = asserted;
}

void pd_phy_send_hard_reset(void) {
    g_requested_role = TS_ROLE_NONE;
    g_mutation_enabled = false;
}
