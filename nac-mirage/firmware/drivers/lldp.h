/*
 * LLDP and NAC metadata parser
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NAC_MIRAGE_LLDP_H
#define NAC_MIRAGE_LLDP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

typedef struct {
    uint16_t ethertype;
    uint16_t length;
    uint8_t ingress_port;
    uint8_t data[512];
} nm_frame_t;

void lldp_init(void);
bool lldp_parse(const nm_frame_t *frame, nm_neighbor_t *neighbor_out, char *summary, size_t summary_len);
bool lldp_apply_policy(nm_frame_t *frame, const nm_policy_t *policy, nm_neighbor_t *neighbor_io, char *summary, size_t summary_len);
void lldp_print_neighbor(const nm_neighbor_t *neighbor);

#endif
