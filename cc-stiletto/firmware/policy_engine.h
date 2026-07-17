/*
 * policy_engine.h — CC-Stiletto attack policy dispatcher.
 *
 * Each attack mode is a struct of callbacks: on_event(), on_tick(), and arm().
 * The dispatcher (policy_engine.c) keeps the active policy and forwards PD
 * events and timer ticks to it. This separation lets new primitives be added
 * without touching the PHY or stack layers.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef CC_STILETTO_POLICY_ENGINE_H
#define CC_STILETTO_POLICY_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "pd_phy.h"
#include "pd_stack.h"

typedef enum {
    POL_SNIFF = 0,        /* transparent relay + log */
    POL_INJECT,            /* queue arbitrary messages */
    POL_SPOOF_VOLTAGE,    /* present fake Source_Capabilities */
    POL_GLITCH,            /* timed VBUS glitch fault injection */
    POL_ROLE_HIJACK,      /* unsolicited DR/PR swaps */
    POL_VCONN_HIJACK,     /* steal VCONN to power embedded payload */
    POL_DEAD_BATTERY,     /* emulate dead-battery sink/source */
    POL_FUZZ,             /* random malformed PD for fuzzing */
    POL_COUNT
} policy_id_t;

typedef struct {
    policy_id_t id;
    const char *name;
    const char *desc;
} policy_info_t;

extern const policy_info_t policy_table[POL_COUNT];

/* Per-policy context shared with the dispatcher */
typedef struct {
    pd_phy_t    *src;     /* FUSB302B toward the source/charger */
    pd_phy_t    *snk;     /* FUSB302B toward the sink/DUT */
    pd_id_t     *ids;     /* message-id trackers */
    policy_id_t  active;

    /* inject policy */
    uint8_t      inject_queue[16][34];   /* queued [sop][hdr+obj] blobs */
    uint8_t      inject_len[16];
    uint8_t      inject_head;
    uint8_t      inject_tail;

    /* spoof voltage policy */
    uint16_t     spoof_mv;
    uint16_t     spoof_ma;

    /* glitch policy */
    uint16_t     glitch_high_mv;
    uint16_t     glitch_low_mv;
    uint32_t     glitch_high_us;
    uint32_t     glitch_low_us;
    uint8_t      glitch_repeat;
    bool         glitch_armed;

    /* role hijack */
    bool         hijack_do_dr;
    bool         hijack_do_pr;
    bool         hijack_do_frs;
    uint16_t     hijack_interval_ms;

    /* fuzz */
    uint16_t     fuzz_count;
    uint16_t     fuzz_crashes;

    /* telemetry snapshot */
    uint16_t     vbus_src_mv;
    uint16_t     vbus_snk_mv;
    int16_t      ma_src;
    int16_t      ma_snk;
    uint8_t      temp_c;
    bool         efuse_fault;
} policy_ctx_t;

/* Public API */
void policy_init(policy_ctx_t *c, pd_phy_t *src, pd_phy_t *snk, pd_id_t *ids);
int  policy_set(policy_ctx_t *c, policy_id_t id);
void policy_dispatch_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
void policy_dispatch_tick(policy_ctx_t *c, uint32_t now_ms);
int  policy_queue_inject(policy_ctx_t *c, uint8_t sop, uint16_t hdr,
                          const uint32_t *obj, uint8_t nobj);
int  policy_configure_glitch(policy_ctx_t *c, uint16_t hi_mv, uint16_t lo_mv,
                              uint32_t hi_us, uint32_t lo_us, uint8_t repeat);
int  policy_configure_spoof(policy_ctx_t *c, uint16_t mv, uint16_t ma);
int  policy_configure_hijack(policy_ctx_t *c, bool dr, bool pr, bool frs,
                              uint16_t interval_ms);
int  policy_arm(policy_ctx_t *c);
int  policy_disarm(policy_ctx_t *c);

#endif /* CC_STILETTO_POLICY_ENGINE_H */