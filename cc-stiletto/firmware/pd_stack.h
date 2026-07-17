/*
 * pd_stack.h — USB PD message encoder/decoder + GoodCRC + message-id tracking.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef CC_STILETTO_PD_STACK_H
#define CC_STILETTO_PD_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include "pd_phy.h"

/* PD control message types (rev 2.0/3.0) */
#define PD_MSG_CTRL_RESERVED       0x00u
#define PD_MSG_GOODCRC             0x01u
#define PD_MSG_GOTOMIN            0x02u
#define PD_MSG_ACCEPT             0x03u
#define PD_MSG_REJECT              0x04u
#define PD_MSG_PING                 0x05u
#define PD_MSG_PS_RDY              0x06u
#define PD_MSG_GET_SOURCE_CAP      0x07u
#define PD_MSG_GET_SINK_CAP        0x08u
#define PD_MSG_DR_SWAP             0x09u
#define PD_MSG_PR_SWAP             0x0Au
#define PD_MSG_VCONN_SWAP          0x0Bu
#define PD_MSG_WAIT                 0x0Cu
#define PD_MSG_SOFT_RESET          0x0Du
#define PD_MSG_DATA_ROLE_SWAP      0x0Eu  /* rev 3.0 alias of DR_SWAP */
#define PD_MSG_NOT_SUPPORTED       0x10u
#define PD_MSG_GET_SOURCE_CAP_EXT   0x11u
#define PD_MSG_GET_STATUS           0x12u
#define PD_MSG_FR_SWAP              0x13u
#define PD_MSG_GET_PPS_STATUS       0x14u
#define PD_MSG_GET_COUNTRY_CODES   0x15u

/* PD data message types */
#define PD_MSG_SOURCE_CAP          0x00u
#define PD_MSG_REQUEST              0x02u
#define PD_MSG_BIST                 0x03u
#define PD_MSG_SINK_CAP             0x04u
#define PD_MSG_BATTERY_STATUS       0x05u
#define PD_MSG_GET_COUNTRY_INFO      0x06u
#define PD_MSG_ENTER_USB            0x09u
#define PD_MSG_VENDOR_DEFINED       0x0Fu

/* PDO (Power Data Object) types — top 2 bits of a Fixed/Variable PDO */
#define PDO_TYPE_FIXED              0u
#define PDO_TYPE_BATTERY            1u
#define PDO_TYPE_VARIABLE            2u
#define PDO_TYPE_AUGMENTED          3u

/* ---- Helpers to build/parse PDOs ----------------------------------------- */

/* Fixed PDO (Source): bits [29:30]=type=0, [9..19]=current in 10mA,
 * [10..19]=voltage in 50mV, [0..9]=max current, etc. */
static inline uint32_t pdo_fixed(uint16_t mv, uint16_t ma, uint8_t idx)
{
    (void)idx;
    return (uint32_t)((mv / 50u) << 10) | (uint32_t)(ma / 10u) | (0u << 30);
}

/* RDO (Request Data Object) for a fixed PDO index */
static inline uint32_t rdo_fixed(uint8_t pdo_idx, uint16_t ma,
                                  bool usb_suspend, bool usb_comms)
{
    uint32_t r = ((uint32_t)(pdo_idx + 1) & 0x7u) << 28;
    r |= (uint32_t)(ma / 10u) & 0x3FFu;          /* operating current */
    r |= ((uint32_t)(ma / 10u) & 0x3FFu) << 10; /* max operating current */
    if (usb_comms)   r |= (1u << 26);
    if (usb_suspend) r |= (1u << 25);
    r |= (1u << 24);                              /* no USB suspend req'd */
    return r;
}

/* Parse a Fixed PDO back into mV / mA */
static inline void pdo_fixed_parse(uint32_t pdo, uint16_t *mv, uint16_t *ma)
{
    *mv = (uint16_t)(((pdo >> 10) & 0x3FFu) * 50u);
    *ma = (uint16_t)((pdo & 0x3FFu) * 10u);
}

/* ---- Message-id tracking ------------------------------------------------- */
typedef struct {
    uint8_t rx_msg_id[3];   /* per SOP: 0=SOP, 1=SOP', 2=SOP'' */
    uint8_t tx_msg_id[3];
} pd_id_t;

void    pd_stack_init(pd_id_t *s);
uint16_t pd_stack_make_header(pd_id_t *s, pd_phy_t *p, uint8_t type,
                              uint8_t nobj, bool is_control, uint8_t sop);
int     pd_stack_send(pd_id_t *s, pd_phy_t *p, uint8_t sop, uint8_t type,
                      const uint32_t *obj, uint8_t nobj, bool is_control);
int     pd_stack_send_control(pd_id_t *s, pd_phy_t *p, uint8_t sop,
                               uint8_t ctrl_type);
const char *pd_msg_name(uint8_t type, bool is_control);

/* Render a decoded pd_msg_t into a short text summary for the sniffer view.
 * Returns the number of bytes written (excluding NUL). */
int pd_sniff_format(const pd_msg_t *m, char *out, int outlen);

/* Compute the PD 16-bit CRC over `len` bytes (used when AUTO_CRC is disabled). */
uint16_t pd_crc16(const uint8_t *data, int len);

#endif /* CC_STILETTO_PD_STACK_H */