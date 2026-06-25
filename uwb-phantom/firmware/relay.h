/*
 * relay.h — UWB relay engine (digital-key relay-attack test bench).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This module implements the relay path between two UWB radios.  In the
 * default single-radio configuration, the same DW3110 time-shares
 * between prover-side and verifier-side; in the dual-radio expansion,
 * a second DW3110 on the expansion header gives true full-duplex
 * forwarding.
 *
 * The relay is the canonical test bench for the UWB anti-relay claim.
 * Operators must hold written authorisation for every target; the
 * firmware refuses to arm the relay until a signed authorised-target
 * record is loaded (see ble_comms / target module).
 *
 * The relay supports three modes of timestamp manipulation:
 *
 *  RELAY_TRANSPARENT  — forward unchanged; the verifier sees the
 *                       extra wire latency (a working STS will reject).
 *  RELAY_SHRINK       — subtract a target ToF from timestamps so the
 *                       prover appears N cm away (defeats no-STS impls).
 *  RELAY_JITTER       — add deterministic jitter to flush out
 *                       implementations that tolerate timing slack.
 */

#ifndef UWB_PHANTOM_RELAY_H
#define UWB_PHANTOM_RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RELAY_TRANSPARENT = 0,
    RELAY_SHRINK      = 1,
    RELAY_JITTER      = 2,
} relay_mode_t;

typedef struct {
    relay_mode_t mode;
    uint32_t     target_shrink_dtu;   /* ToF to subtract (0..~5000 DTU) */
    uint32_t     jitter_us;           /* ±jitter applied to re-TX time */
    uint32_t     delay_us;            /* fixed forwarding delay */
    bool         armed;
    uint64_t     forwarded_count;
    uint64_t     dropped_count;
} relay_state_t;

int  relay_init(relay_state_t *st);
int  relay_arm(relay_state_t *st);
void relay_disarm(relay_state_t *st);
void relay_task(void *arg);           /* FreeRTOS task entry */

/* Called from the DW3110 IRQ path when a frame arrives on either radio. */
int  relay_on_frame(int radio_id, const uint8_t *frame, size_t len,
                    uint64_t rx_dtu, uint8_t sts_quality);

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_RELAY_H */