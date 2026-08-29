/*
 * Type-C Specter PD PHY abstraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef TYPEC_SPECTER_PD_PHY_H
#define TYPEC_SPECTER_PD_PHY_H

#include <stdbool.h>
#include "../board.h"

typedef struct {
    bool target_present;
    bool peer_present;
    ts_power_role_t target_role;
    ts_power_role_t peer_role;
    ts_data_role_t data_role;
} ts_attach_state_t;

void pd_phy_init(void);
void pd_phy_set_proxy_mode(bool enabled);
void pd_phy_set_mutation_enabled(bool enabled);
ts_attach_state_t pd_phy_poll_attach(void);
bool pd_phy_receive_packet(ts_pd_packet_t *packet);
void pd_phy_load_identity_profile(const char *name);
void pd_phy_request_role_swap(ts_power_role_t desired_role);
void pd_phy_assert_debug_accessory(bool asserted);
void pd_phy_send_hard_reset(void);

#endif
