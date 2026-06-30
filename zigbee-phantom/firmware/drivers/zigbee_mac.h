/*
 * drivers/zigbee_mac.h — IEEE 802.15.4 MAC frame parser/builder
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef ZIGBEE_PHANTOM_ZIGBEE_MAC_H
#define ZIGBEE_PHANTOM_ZIGBEE_MAC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define IEEE802154_MAX_FRM_LEN   127
#define IEEE802154_FCS_LEN        2
#define IEEE802154_AIFS_SYMBOLS  12
#define IEEE802154_ACK_WAIT_SYMBOLS  120
#define IEEE802154_SYMBOL_US     16      /* 62.5 kbaud -> 16 us/symbol */

/* Frame types */
typedef enum {
    FRM_TYPE_BEACON    = 0,
    FRM_TYPE_DATA      = 1,
    FRM_TYPE_ACK       = 2,
    FRM_TYPE_CMD       = 3,
    FRM_TYPE_RESERVED  = 4,
    FRM_TYPE_LLDN      = 5,
    FRM_TYPE_MULTIPURP = 6,
    FRM_TYPE_FRAG      = 7
} zb_frm_type_t;

/* MAC command frame identifiers */
typedef enum {
    MAC_CMD_ASSOC_REQ         = 0x01,
    MAC_CMD_ASSOC_RSP         = 0x02,
    MAC_CMD_DISASSOC_NOTIF    = 0x03,
    MAC_CMD_DATA_REQ          = 0x04,
    MAC_CMD_PAN_ID_CONFLICT    = 0x05,
    MAC_CMD_ORPHAN_NOTIF      = 0x06,
    MAC_CMD_BEACON_REQ         = 0x07,
    MAC_CMD_COORD_REALIGN      = 0x08,
    MAC_CMD_GTS_REQ           = 0x09,
    MAC_CMD_GTS_RSP           = 0x0A,
    MAC_CMD_GTS_ALLOC_NOTIF   = 0x0B
} zb_mac_cmd_t;

/* Addressing modes */
typedef enum {
    ADDR_MODE_NONE = 0,
    ADDR_MODE_SHORT = 2,
    ADDR_MODE_LONG  = 3
} zb_addr_mode_t;

/* Security levels (Zigbee NWK/APS) */
typedef enum {
    SEC_NONE             = 0x00,
    SEC_MIC_32           = 0x01,
    SEC_MIC_64           = 0x02,
    SEC_MIC_128          = 0x03,
    SEC_ENC              = 0x04,
    SEC_ENC_MIC_32       = 0x05,
    SEC_ENC_MIC_64       = 0x06,
    SEC_ENC_MIC_128      = 0x07
} zb_sec_level_t;

/* Parsed MAC frame */
typedef struct {
    uint8_t        *raw;            /* pointer to raw frame (excluding FCS) */
    uint8_t         raw_len;        /* length excluding FCS */

    /* Frame Control field (parsed) */
    zb_frm_type_t   type;
    bool            security;
    bool            frame_pending;
    bool            ack_req;
    bool            pan_compress;
    uint8_t         seq_num;
    uint8_t         dst_panid[2];
    uint8_t         dst_addr[8];
    uint8_t         src_panid[2];
    uint8_t         src_addr[8];
    zb_addr_mode_t  dst_mode;
    zb_addr_mode_t  src_mode;

    /* Security header (if present) */
    uint8_t         sec_ctrl;
    uint8_t         key_id_mode;
    uint8_t         key_id[9];
    uint32_t        frame_ctr;

    /* Payload pointer + length */
    uint8_t        *payload;
    uint8_t         payload_len;

    /* FCS check */
    bool            crc_ok;
} zb_mac_frm_t;

/* ---- API ---- */
int  zb_mac_parse(const uint8_t *buf, uint8_t len, zb_mac_frm_t *out);
int  zb_mac_build(uint8_t *out, const zb_mac_frm_t *frm);
uint16_t zb_mac_fcs(const uint8_t *data, uint8_t len);   /* ITU-T CRC-16 */
bool zb_mac_match_pan(const zb_mac_frm_t *frm, uint16_t panid);
bool zb_mac_match_src_eui(const zb_mac_frm_t *frm, const uint8_t eui[8]);

/* Build common frames for injection */
int  zb_mac_build_beacon(uint8_t *out, uint16_t panid,
                         const uint8_t *ext_panid, uint8_t chan,
                         bool router_cap, bool enddev_cap,
                         uint8_t protocol_ver, uint8_t stack_profile);
int  zb_mac_build_data(uint8_t *out, uint16_t dst_pan, uint16_t dst_short,
                       uint16_t src_pan, uint16_t src_short,
                       uint8_t seq, const uint8_t *payload, uint8_t plen,
                       bool sec, bool ack_req);
int  zb_mac_build_ack(uint8_t *out, uint8_t seq);
int  zb_mac_build_cmd(uint8_t *out, uint16_t dst_pan, uint16_t dst_short,
                      uint16_t src_pan, uint16_t src_short,
                      uint8_t seq, uint8_t cmd_id,
                      const uint8_t *cmd_payload, uint8_t cmd_plen);

#endif /* ZIGBEE_PHANTOM_ZIGBEE_MAC_H */