/*
 * eth_bridge.h — L2 transparent bridge for inline MITM mode
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_ETH_BRIDGE_H
#define CHRONOS_PHANTOM_ETH_BRIDGE_H

#include <stdint.h>

#define ETH_MAX_FRAME  1536

/* Bridge state */
typedef struct {
    uint32_t frames_a_to_b;
    uint32_t frames_b_to_a;
    uint32_t frames_dropped;
    uint32_t ptp_intercepted;
    /* MAC learning table (simplified: 16 entries) */
    uint8_t  mac_table[16][6];
    uint8_t  mac_port[16];   /* 0 = port A, 1 = port B */
    uint8_t  mac_next;
} eth_bridge_t;

void eth_bridge_init(eth_bridge_t *br);
void eth_bridge_learn(eth_bridge_t *br, const uint8_t *mac, uint8_t port);
int  eth_bridge_lookup_port(eth_bridge_t *br, const uint8_t *dst_mac);
void eth_bridge_forward(eth_bridge_t *br, const uint8_t *frame, uint32_t len,
                        uint8_t src_port, uint8_t *fwd_port);

#endif /* CHRONOS_PHANTOM_ETH_BRIDGE_H */