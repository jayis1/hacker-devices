/*
 * bridge_driver.h — Inline cut-through bridge engine for AxleTap
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 */
#ifndef AXLETAP_BRIDGE_DRIVER_H
#define AXLETAP_BRIDGE_DRIVER_H

#include <stdint.h>

/* Bridge operating mode */
typedef enum {
    BRIDGE_OFF = 0,        /* Both PHYs link up but no forwarding — pure tap */
    BRIDGE_PASSTHROUGH = 1,/* Forward A<->B unmodified (repeater with mirror) */
    BRIDGE_MITM = 2        /* Forward with active injection ruleset */
} bridge_mode_t;

/* Action taken by the injection ruleset on a given frame */
typedef enum {
    ACT_FORWARD = 0,
    ACT_DROP    = 1,
    ACT_MODIFY = 2,
    ACT_CLONE   = 3,       /* Forward + inject a copy */
    ACT_INJECT  = 4        /* Drop original, inject replacement */
} bridge_action_t;

/* A frame seen by the bridge */
typedef struct {
    uint8_t  *data;        /* Pointer to frame in DMA buffer */
    uint16_t  len;
    uint8_t   direction;   /* 0 = A->B, 1 = B->A */
    uint64_t  timestamp_ns;
} bridge_frame_t;

/* Injection rule (simplified — production uses a bytecode engine) */
typedef struct {
    uint8_t  eth_proto[2];  /* EtherType to match (0xFFFF = wildcard) */
    uint8_t  match_direction; /* 0=A->B, 1=B->A, 2=any */
    bridge_action_t action;
    uint8_t  modify_mask;   /* Bitmask: bit0=dst, bit1=src, bit2=type, bit3=payload */
    uint8_t  new_dst[6];
    uint8_t  new_src[6];
    uint16_t new_type;
} bridge_rule_t;

#define BRIDGE_MAX_RULES 16

/* Initialize the bridge engine (DMA rings, MAC config). */
int  bridge_init(void);

/* Set bridge mode. */
void bridge_set_mode(bridge_mode_t m);
bridge_mode_t bridge_get_mode(void);

/* Add / remove / clear injection rules. */
int  bridge_add_rule(const bridge_rule_t *r);
int  bridge_remove_rule(int idx);
void bridge_clear_rules(void);

/* Process pending frames — called from the cooperative scheduler. */
void bridge_poll(void);

/* Statistics */
typedef struct {
    uint64_t frames_ab;
    uint64_t frames_ba;
    uint64_t dropped;
    uint64_t modified;
    uint64_t cloned;
    uint64_t injected;
    uint64_t rx_errors;
    uint64_t tx_errors;
} bridge_stats_t;
void bridge_get_stats(bridge_stats_t *st);

/* Submit an injected frame from a high-level engine (gPTP, TSN, fuzzer). */
int  bridge_inject(const uint8_t *frame, uint16_t len, uint8_t direction);

#endif /* AXLETAP_BRIDGE_DRIVER_H */