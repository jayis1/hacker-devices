/*
 * pd_stack.c — USB PD message encoder/decoder + message-id tracking for CC-Stiletto.
 *
 * Maintains per-SOP message-id counters (rx + tx), builds PD headers, and sends
 * control / data messages through the PHY. Decoding helpers turn PDOs into
 * human-readable voltage/current pairs for the sniffer view.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#include "pd_stack.h"
#include "pd_phy.h"
#include <string.h>
#include <stdio.h>

static const char *ctrl_names[] = {
    "Reserved", "GoodCRC", "GotoMin", "Accept", "Reject", "Ping",
    "PS_RDY", "Get_Source_Cap", "Get_Sink_Cap", "DR_Swap", "PR_Swap",
    "VCONN_Swap", "Wait", "Soft_Reset", "DR_Swap3", "Reserved",
    "Not_Supported", "Get_Source_Cap_Ext", "Get_Status", "FR_Swap",
    "Get_PPS_Status", "Get_Country_Codes"
};
static const char *data_names[] = {
    "Source_Capabilities", "Reserved", "Request", "BIST", "Sink_Capabilities",
    "Battery_Status", "Get_Country_Info", "Reserved", "Reserved",
    "Enter_USB", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Vendor_Defined"
};

void pd_stack_init(pd_id_t *s)
{
    memset(s, 0, sizeof(*s));
}

/* Increment tx message-id (mod 32) and stash into bits 9..13 of header */
static uint8_t next_tx_id(pd_id_t *s, uint8_t sop)
{
    uint8_t id = s->tx_msg_id[sop];
    s->tx_msg_id[sop] = (uint8_t)((id + 1u) & 0x1Fu);
    return id;
}

uint16_t pd_stack_make_header(pd_id_t *s, pd_phy_t *p, uint8_t type,
                               uint8_t nobj, bool is_control, uint8_t sop)
{
    uint16_t h = (uint16_t)(type & 0x0Fu);
    /* data role (bit 5) */
    h |= (p->data_role == PD_ROLE_SOURCE) ? (1u << 5) : 0u;
    /* spec rev 2.0 in bits 6..7 */
    h |= (2u << 6);
    /* power role in bit 8 (also serves as cable-plug indicator for SOP') */
    if (sop == SOP_SOP) {
        h |= (p->power_role == PD_POWER_SOURCE) ? (1u << 8) : 0u;
    } else {
        /* SOP' / SOP'': bit 8 = 0 for SOP', 1 for SOP'' */
        h |= (sop == 0x13u) ? 0u : (1u << 8);
    }
    /* message id in bits 9..13 */
    h |= (uint16_t)(next_tx_id(s, sop > 2 ? 0 : sop) << 9);
    /* object count in bits 14..15 (0 for control) */
    if (!is_control) h |= (uint16_t)((nobj & 0x07u) << 14);
    return h;
}

int pd_stack_send(pd_id_t *s, pd_phy_t *p, uint8_t sop, uint8_t type,
                  const uint32_t *obj, uint8_t nobj, bool is_control)
{
    uint16_t h = pd_stack_make_header(s, p, type, nobj, is_control, sop);
    return pd_phy_send(p, sop, h, obj, nobj);
}

int pd_stack_send_control(pd_id_t *s, pd_phy_t *p, uint8_t sop,
                           uint8_t ctrl_type)
{
    return pd_stack_send(s, p, sop, ctrl_type, NULL, 0, true);
}

const char *pd_msg_name(uint8_t type, bool is_control)
{
    if (is_control) {
        if (type < (sizeof(ctrl_names)/sizeof(ctrl_names[0])))
            return ctrl_names[type];
    } else {
        if (type < (sizeof(data_names)/sizeof(data_names[0])))
            return data_names[type];
    }
    return "Unknown";
}

/* ---- Sniffer-format helper: render a pd_msg_t into a short text summary ----
 * The caller provides an output buffer of at least 80 bytes.  This is used by
 * the console module to print decoded packets over USB-CDC. */
int pd_sniff_format(const pd_msg_t *m, char *out, int outlen)
{
    if (!m || outlen < 80) return -1;
    bool is_ctrl = ((m->header >> 14) & 0x07u) == 0;
    uint8_t type = m->header & 0x0Fu;
    const char *name = pd_msg_name(type, is_ctrl);
    const char *sop = (m->sop_type == 0) ? "SOP" :
                      (m->sop_type == 1) ? "SOP'" : "SOP''";

    /* Common header */
    int n = 0;
    n += snprintf(out + n, outlen - n, "[%s id=%u] %s ",
                            sop, m->msg_id, name);

    /* For Source_Capabilities, dump each PDO as mV/mA */
    if (!is_ctrl && type == PD_MSG_SOURCE_CAP && m->num_obj > 0) {
        for (uint8_t i = 0; i < m->num_obj && n < outlen - 24; i++) {
            uint32_t pdo = m->obj[i];
            uint8_t pt = (uint8_t)((pdo >> 30) & 0x03u);
            if (pt == PDO_TYPE_FIXED) {
                uint16_t mv, ma;
                pdo_fixed_parse(pdo, &mv, &ma);
                n += snprintf(out + n, outlen - n,
                                         "PDO%u=%umV/%umA ", i, mv, ma);
            } else {
                n += snprintf(out + n, outlen - n,
                                         "PDO%u=type%u ", i, pt);
            }
        }
    } else if (!is_ctrl && type == PD_MSG_REQUEST && m->num_obj >= 1) {
        uint32_t rdo = m->obj[0];
        uint8_t idx = (uint8_t)((rdo >> 28) & 0x07u);
        uint16_t op_ma = (uint16_t)((rdo & 0x3FFu) * 10u);
        n += snprintf(out + n, outlen - n,
                                 "RDO idx=%u op=%umA ", idx, op_ma);
    }
    if (n < outlen) out[n++] = '\n';
    if (n < outlen) out[n] = '\0';
    return n;
}

/* PD 16-bit CRC used for verification when AUTO_CRC is disabled.  Polynomial
 * 0x1021, init 0xFFFF.  Most PHYs handle this in hardware; included for
 * completeness and for software-side validation of captured frames. */
uint16_t pd_crc16(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFFu;
    for (int i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1u) crc = (crc >> 1) ^ 0x8005u;
            else          crc >>= 1;
        }
    }
    return crc;
}