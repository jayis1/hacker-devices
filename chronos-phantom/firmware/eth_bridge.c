/*
 * eth_bridge.c — L2 transparent bridge implementation
 *
 * Cut-through forwarding for non-PTP frames; PTP/NTP frames are punted
 * to the PTP/NTP engine for inspection and modification.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <string.h>
#include "eth_bridge.h"
#include "registers.h"

void eth_bridge_init(eth_bridge_t *br)
{
    memset(br, 0, sizeof(*br));
    /* Mark all MAC table entries as empty using 0xFF sentinel
     * (00:00:00:00:00:00 is a valid dst and would falsely match) */
    for (int i = 0; i < 16; i++)
        memset(br->mac_table[i], 0xFF, 6);
}

void eth_bridge_learn(eth_bridge_t *br, const uint8_t *mac, uint8_t port)
{
    /* Check if MAC already in table */
    for (int i = 0; i < 16; i++) {
        if (memcmp(br->mac_table[i], mac, 6) == 0) {
            br->mac_port[i] = port;
            return;
        }
    }
    /* Add new entry (round-robin replacement) */
    memcpy(br->mac_table[br->mac_next], mac, 6);
    br->mac_port[br->mac_next] = port;
    br->mac_next = (br->mac_next + 1) & 0x0F;
}

int eth_bridge_lookup_port(eth_bridge_t *br, const uint8_t *dst_mac)
{
    /* Broadcast/multicast: flood to both ports */
    if (dst_mac[0] & 0x01)
        return -1;  /* flood */
    for (int i = 0; i < 16; i++) {
        if (memcmp(br->mac_table[i], dst_mac, 6) == 0)
            return br->mac_port[i];  /* forward to the port where MAC lives */
    }
    return -1;  /* unknown — flood */
}

void eth_bridge_forward(eth_bridge_t *br, const uint8_t *frame, uint32_t len,
                        uint8_t src_port, uint8_t *fwd_port)
{
    if (len < 14) {
        br->frames_dropped++;
        *fwd_port = 0xFF;
        return;
    }

    /* Learn source MAC */
    eth_bridge_learn(br, &frame[6], src_port);

    /* Look up destination */
    int dst = eth_bridge_lookup_port(br, &frame[0]);
    if (dst < 0) {
        /* Flood — send to opposite port (simplified: single fwd) */
        *fwd_port = src_port ^ 1;
    } else {
        *fwd_port = (uint8_t)dst;
    }

    if (src_port == 0)
        br->frames_a_to_b++;
    else
        br->frames_b_to_a++;
}