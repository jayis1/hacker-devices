/*
 * pd_controller.c — USB-C Power Delivery protocol state machine
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements the PD protocol state machine for both the source and sink
 * ports. Handles contract negotiation, role swaps, hard/soft resets,
 * VDMs, and message routing between the two ports (MITM mode).
 *
 * The state machine is polled from the main loop (pd_poll()) and processes
 * one event per call to maintain deterministic timing.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* ---- External state (defined in main.c) ---- */
extern device_state_t g_state;

/* ---- Port role and data role tracking ---- */
static uint8_t s_port_role[PD_PORT_COUNT]  = { 1, 0 };  /* src=Source, snk=Sink */
static uint8_t s_data_role[PD_PORT_COUNT]   = { 1, 0 };  /* src=DFP, snk=UFP */
static uint8_t s_msg_id_rx[PD_PORT_COUNT];  /* Last received msg ID */
static uint32_t s_last_msg_time[PD_PORT_COUNT];

/* ---- MITM forwarding state ---- */
static uint8_t s_mitm_forwarding = 1;  /* Forward PD messages between ports */

/* ---- CC routing control (TMUX2512) ---- */
void pd_set_routing(cc_routing_mode_t mode) {
    /* PA4/PA5 = TMUX2512 #1 (source CC), PA6/PA7 = TMUX2512 #2 (sink CC) */
    /* SEL1 SEL0 → channel:
     * 00 = passthrough (CC1_src→CC1_snk)
     * 01 = isolated
     * 10 = crossover
     * 11 = MCU control
     */
    uint8_t sel1_src, sel0_src, sel1_snk, sel0_snk;

    switch (mode) {
    case CC_MODE_PASSTHROUGH:
        sel1_src = 0; sel0_src = 0; sel1_snk = 0; sel0_snk = 0;
        break;
    case CC_MODE_ISOLATED:
        sel1_src = 0; sel0_src = 1; sel1_snk = 0; sel0_snk = 1;
        break;
    case CC_MODE_CROSSOVER:
        sel1_src = 1; sel0_src = 0; sel1_snk = 1; sel0_snk = 0;
        break;
    case CC_MODE_MCU_CTRL:
        sel1_src = 1; sel0_src = 1; sel1_snk = 1; sel0_snk = 1;
        break;
    default:
        sel1_src = 0; sel0_src = 0; sel1_snk = 0; sel0_snk = 0;
        break;
    }

    /* Write to GPIO */
    volatile uint32_t *bsrr_a = (volatile uint32_t *)(GPIOA_BASE + GPIO_BSRR_OFFSET);
    /* PA4 = sel0_src, PA5 = sel1_src, PA6 = sel0_snk, PA7 = sel1_snk */
    *bsrr_a = (1u << 4) << (sel0_src ? 0 : 16)
            | (1u << 5) << (sel1_src ? 0 : 16)
            | (1u << 6) << (sel0_snk ? 0 : 16)
            | (1u << 7) << (sel1_snk ? 0 : 16);
}

/* ---- PD initialization ---- */
void pd_init(void) {
    for (int p = 0; p < PD_PORT_COUNT; p++) {
        s_msg_id_rx[p] = 0xFF;
        s_last_msg_time[p] = 0;
    }
    s_mitm_forwarding = 1;
}

/* ---- Build and send a Source_Capabilities message ---- */
int pd_send_source_cap(pd_port_t port, const uint32_t *pdos, uint8_t count) {
    if (count > PD_MAX_DATA_OBJS) return -1;

    pd_msg_t msg;
    msg.header = (uint16_t)((PD_DATA_SOURCE_CAP & 0x1Fu)
                  | (1u << PD_HDR_PORT_ROLE_SHIFT)   /* Source role */
                  | (1u << PD_HDR_DATA_ROLE_SHIFT)   /* DFP */
                  | ((uint32_t)g_state.msg_id[port] << PD_HDR_MSG_ID_SHIFT)
                  | ((uint32_t)count << PD_HDR_NUM_OBJ_SHIFT)
                  | (2u << PD_HDR_REV_SHIFT));        /* PD 3.0 */
    msg.num_objs = count;
    for (uint8_t i = 0; i < count; i++) {
        msg.data_obj[i] = pdos[i];
    }

    g_state.msg_id[port] = (g_state.msg_id[port] + 1) & 0x07u;
    return cc_phy_send_msg(port, &msg);
}

/* ---- Build and send a Request message ---- */
int pd_send_request(pd_port_t port, uint32_t pdo_index) {
    /* Build a Request DO for the given PDO index */
    uint32_t req_do = 0;
    req_do |= (pdo_index & 0x07u) << 28u;  /* Object position (1-based, but index here) */
    req_do |= (pdo_index + 1u) << 28u;      /* 1-based object position */
    req_do |= 0x21u << 10u;                 /* Operating current = 100mA units */
    req_do |= 0x21u;                        /* Max operating current */

    pd_msg_t msg;
    msg.header = (uint16_t)((PD_DATA_REQUEST & 0x1Fu)
                  | (0u << PD_HDR_PORT_ROLE_SHIFT)   /* Sink role */
                  | (0u << PD_HDR_DATA_ROLE_SHIFT)   /* UFP */
                  | ((uint32_t)g_state.msg_id[port] << PD_HDR_MSG_ID_SHIFT)
                  | (1u << PD_HDR_NUM_OBJ_SHIFT)
                  | (2u << PD_HDR_REV_SHIFT));
    msg.num_objs = 1;
    msg.data_obj[0] = req_do;

    g_state.msg_id[port] = (g_state.msg_id[port] + 1) & 0x07u;
    return cc_phy_send_msg(port, &msg);
}

/* ---- Send a control message ---- */
int pd_send_control(pd_port_t port, pd_ctrl_msg_t ctrl) {
    pd_msg_t msg;
    uint8_t role = s_port_role[port];
    uint8_t drole = s_data_role[port];

    msg.header = (uint16_t)((ctrl & 0x1Fu)
                  | ((uint32_t)role << PD_HDR_PORT_ROLE_SHIFT)
                  | ((uint32_t)drole << PD_HDR_DATA_ROLE_SHIFT)
                  | ((uint32_t)g_state.msg_id[port] << PD_HDR_MSG_ID_SHIFT)
                  | (0u << PD_HDR_NUM_OBJ_SHIFT)
                  | (2u << PD_HDR_REV_SHIFT));
    msg.num_objs = 0;

    g_state.msg_id[port] = (g_state.msg_id[port] + 1) & 0x07u;
    return cc_phy_send_msg(port, &msg);
}

/* ---- Send Hard Reset ---- */
int pd_send_hard_reset(pd_port_t port) {
    /* Hard Reset is a special FUSB302B command, not a regular PD message */
    /* Write to CONTROL0: bit 7 = SEND_HARD_RESET */
    uint8_t bus = (port == PD_PORT_SOURCE) ? I2C1_BUS : I2C2_BUS;
    uint8_t addr = (port == PD_PORT_SOURCE) ? FUSB302B_SRC_ADDR : FUSB302B_SNK_ADDR;
    uint8_t val = 0x80u; /* SEND_HARD_RESET */
    i2c_write_reg(bus, addr, FUSB302_CONTROL3, &val, 1);

    /* Update state */
    g_state.contract_state[port] = CONTRACT_STATE_HARD_RESET;
    vbus_disconnect(port);

    return 0;
}

/* ---- Send Power Role Swap ---- */
int pd_send_pr_swap(pd_port_t port) {
    return pd_send_control(port, PD_CTRL_PR_SWAP);
}

/* ---- Send Data Role Swap ---- */
int pd_send_dr_swap(pd_port_t port) {
    return pd_send_control(port, PD_CTRL_DR_SWAP);
}

/* ---- Send a VDM (Vendor Defined Message) ---- */
int pd_send_vdm(pd_port_t port, uint16_t vdm_header, const uint32_t *data, uint8_t count) {
    if (count > 6) return -1; /* 1 VDM header + 6 data objects = 7 max */

    pd_msg_t msg;
    msg.header = (uint16_t)((PD_DATA_VDM & 0x1Fu)
                  | ((uint32_t)s_port_role[port] << PD_HDR_PORT_ROLE_SHIFT)
                  | ((uint32_t)s_data_role[port] << PD_HDR_DATA_ROLE_SHIFT)
                  | ((uint32_t)g_state.msg_id[port] << PD_HDR_MSG_ID_SHIFT)
                  | ((uint32_t)(count + 1u) << PD_HDR_NUM_OBJ_SHIFT)
                  | (2u << PD_HDR_REV_SHIFT));
    msg.num_objs = count + 1u;
    msg.data_obj[0] = (uint32_t)vdm_header;
    for (uint8_t i = 0; i < count; i++) {
        msg.data_obj[i + 1] = data[i];
    }

    g_state.msg_id[port] = (g_state.msg_id[port] + 1) & 0x07u;
    return cc_phy_send_msg(port, &msg);
}

/* ---- State name string ---- */
const char *pd_state_name(contract_state_t s) {
    switch (s) {
    case CONTRACT_STATE_IDLE:         return "IDLE";
    case CONTRACT_STATE_SRC_SEND_CAP: return "SRC_SEND_CAP";
    case CONTRACT_STATE_SNK_REQUEST:  return "SNK_REQUEST";
    case CONTRACT_STATE_SRC_ACCEPT:   return "SRC_ACCEPT";
    case CONTRACT_STATE_PS_RDY_SRC:   return "PS_RDY_SRC";
    case CONTRACT_STATE_PS_RDY_SNK:   return "PS_RDY_SNK";
    case CONTRACT_STATE_ACTIVE:       return "ACTIVE";
    case CONTRACT_STATE_HARD_RESET:   return "HARD_RESET";
    case CONTRACT_STATE_ERROR:        return "ERROR";
    default:                          return "UNKNOWN";
    }
}

/* ---- Process a received PD message ---- */
static void process_rx_msg(pd_port_t port, const pd_msg_t *msg) {
    uint8_t msg_type = (uint8_t)(msg->header & 0x1Fu);
    uint8_t num_objs = (uint8_t)((msg->header >> PD_HDR_NUM_OBJ_SHIFT) & 0x07u);
    uint8_t msg_id   = (uint8_t)((msg->header >> PD_HDR_MSG_ID_SHIFT) & 0x07u);
    uint8_t is_data = (num_objs > 0);

    /* Check for duplicate (same msg_id) */
    if (msg_id == s_msg_id_rx[port]) return;
    s_msg_id_rx[port] = msg_id;

    /* Send GoodCRC (FUSB302B auto-replies, but just in case) */
    /* cc_phy_send_goodcrc(port, msg_id, s_port_role[port], s_data_role[port]); */

    s_last_msg_time[port] = millis();

    if (!is_data) {
        /* Control message */
        switch (msg_type) {
        case PD_CTRL_ACCEPT:
            g_state.contract_state[port] = CONTRACT_STATE_SRC_ACCEPT;
            break;
        case PD_CTRL_PS_RDY:
            if (g_state.contract_state[port] == CONTRACT_STATE_SRC_ACCEPT) {
                g_state.contract_state[port] = CONTRACT_STATE_PS_RDY_SRC;
                /* Enable VBUS path for this port */
                vbus_enable_path(port, 1);
            }
            break;
        case PD_CTRL_REJECT:
        case PD_CTRL_NOT_SUPPORTED:
            g_state.contract_state[port] = CONTRACT_STATE_IDLE;
            break;
        case PD_CTRL_SOFT_RESET:
            /* Reset message ID counter */
            g_state.msg_id[port] = 0;
            break;
        case PD_CTRL_PR_SWAP:
            /* Toggle power role */
            s_port_role[port] ^= 1;
            pd_send_control(port, PD_CTRL_ACCEPT);
            break;
        case PD_CTRL_DR_SWAP:
            /* Toggle data role */
            s_data_role[port] ^= 1;
            pd_send_control(port, PD_CTRL_ACCEPT);
            break;
        case PD_CTRL_PING:
            /* Source is checking if we're alive; GoodCRC is sufficient */
            break;
        default:
            break;
        }
    } else {
        /* Data message */
        switch (msg_type) {
        case PD_DATA_SOURCE_CAP:
            /* Source advertised capabilities — store them */
            g_state.contract_state[port] = CONTRACT_STATE_SRC_SEND_CAP;
            /* In MITM mode, forward to the other port with modified PDOs */
            if (s_mitm_forwarding && g_state.cc_mode == CC_MODE_PASSTHROUGH) {
                /* Forward source caps to the sink port */
                pd_port_t other = (port == PD_PORT_SOURCE) ? PD_PORT_SINK : PD_PORT_SOURCE;
                pd_send_source_cap(other, msg->data_obj, num_objs);
            }
            break;
        case PD_DATA_REQUEST:
            /* Sink is requesting a PDO */
            g_state.contract_state[port] = CONTRACT_STATE_SNK_REQUEST;
            /* In MITM mode, forward the request to the source port */
            if (s_mitm_forwarding && g_state.cc_mode == CC_MODE_PASSTHROUGH) {
                pd_port_t other = (port == PD_PORT_SOURCE) ? PD_PORT_SINK : PD_PORT_SOURCE;
                /* Re-send the request on the other port */
                pd_msg_t fwd;
                fwd.header = (uint16_t)((PD_DATA_REQUEST & 0x1Fu)
                              | ((uint32_t)s_port_role[other] << PD_HDR_PORT_ROLE_SHIFT)
                              | ((uint32_t)s_data_role[other] << PD_HDR_DATA_ROLE_SHIFT)
                              | ((uint32_t)g_state.msg_id[other] << PD_HDR_MSG_ID_SHIFT)
                              | ((uint32_t)num_objs << PD_HDR_NUM_OBJ_SHIFT)
                              | (2u << PD_HDR_REV_SHIFT));
                fwd.num_objs = num_objs;
                for (uint8_t i = 0; i < num_objs; i++) {
                    fwd.data_obj[i] = msg->data_obj[i];
                }
                g_state.msg_id[other] = (g_state.msg_id[other] + 1) & 0x07u;
                cc_phy_send_msg(other, &fwd);
            }
            /* Accept the request */
            pd_send_control(port, PD_CTRL_ACCEPT);
            break;
        case PD_DATA_VDM:
            /* VDM received — route to covert channel handler */
            if (num_objs >= 1) {
                uint16_t vdm_hdr = (uint16_t)(msg->data_obj[0] & 0xFFFFu);
                uint8_t vdm_cmd = (uint8_t)(vdm_hdr & 0x7Fu);
                if (vdm_cmd == VDM_CMD_COVERT_DATA) {
                    /* This is a covert channel message */
                    /* Data is in data_obj[1..num_objs-1] */
                    extern void covert_channel_handle_vdm(const uint32_t *data, uint8_t count);
                    if (num_objs > 1) {
                        covert_channel_handle_vdm(&msg->data_obj[1], num_objs - 1);
                    }
                }
            }
            break;
        case PD_DATA_SINK_CAP:
            /* Sink capabilities — forward in MITM mode */
            break;
        default:
            break;
        }
    }
}

/* ---- PD poll (called from main loop) ---- */
void pd_poll(void) {
    for (int p = 0; p < PD_PORT_COUNT; p++) {
        pd_msg_t rx_msg;
        int rc = cc_phy_recv_msg((pd_port_t)p, &rx_msg, 0);
        if (rc == 0) {
            process_rx_msg((pd_port_t)p, &rx_msg);
        } else if (rc == -2) {
            /* CRC error — flush and continue */
            cc_phy_flush_rx((pd_port_t)p);
        }

        /* Check for CC detach */
        uint8_t cc_status = cc_phy_get_cc_status((pd_port_t)p);
        if (cc_status == 0 && g_state.contract_state[p] == CONTRACT_STATE_ACTIVE) {
            /* CC line disconnected */
            g_state.contract_state[p] = CONTRACT_STATE_IDLE;
            vbus_disconnect((pd_port_t)p);
        } else if (cc_status > 0 && g_state.contract_state[p] == CONTRACT_STATE_IDLE) {
            /* CC line attached — start negotiation */
            if (p == PD_PORT_SOURCE) {
                /* We are the source: send capabilities */
                /* (In MITM mode, this is triggered by the upstream source) */
            }
        }
    }
}