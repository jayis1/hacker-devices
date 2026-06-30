/*
 * drivers/zigbee_aps.h — Zigbee APS layer parser and key-extraction
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef ZIGBEE_PHANTOM_ZIGBEE_APS_H
#define ZIGBEE_PHANTOM_ZIGBEE_APS_H

#include <stdint.h>
#include <stdbool.h>
#include "zigbee_mac.h"

/* APS command frame identifiers */
#define APS_CMD_TRANSPORT_KEY        0x05
#define APS_CMD_UPDATE_DEVICE        0x06
#define APS_CMD_REMOVE_DEVICE        0x07
#define APS_CMD_REQUEST_KEY          0x08
#define APS_CMD_SWITCH_KEY           0x09
#define APS_CMD_TUNNEL                0x0B

/* APS key types carried in Transport-Key */
#define APS_KEY_TYPE_NWK             0x01
#define APS_KEY_TYPE_APP              0x02
#define APS_KEY_TYPE_TCLK_MASTER      0x03
#define APS_KEY_TYPE_TCLK_UPDATE      0x04
#define APS_KEY_TYPE_TCLK_KEEPALIVE   0x05

/* ZCL cluster IDs (selected, for injection attacks) */
#define ZCL_CLUSTER_GEN_ONOFF        0x0006
#define ZCL_CLUSTER_GEN_LEVEL        0x0008
#define ZCL_CLUSTER_CLOSURES_DOORLOCK 0x0101
#define ZCL_CLUSTER_GEN_IDENTIFY     0x0003

/* ZCL command IDs */
#define ZCL_CMD_ONOFF_OFF             0x00
#define ZCL_CMD_ONOFF_ON               0x01
#define ZCL_CMD_ONOFF_TOGGLE           0x02
#define ZCL_CMD_LEVEL_MOVE              0x01

/* Parsed APS frame (carried in MAC data payload) */
typedef struct {
    uint8_t  frame_ctrl;     /* APS frame control */
    uint8_t  delivery_mode;  /* upper nibble */
    uint8_t  security;       /* security-level subfield */
    bool     ack_format;
    bool     ext_hdr;
    uint8_t  dst_ep;
    uint8_t  cluster;        /* 2 bytes */
    uint8_t  profile;        /* 2 bytes */
    uint8_t  src_ep;
    uint8_t  counter;
    uint16_t dst_group;      /* when dst_ep=0 (group) */
    uint8_t  cmd_id;         /* APS command id, if frame_ctrl says cmd */
    /* For Transport-Key captures: */
    uint8_t  key_type;
    uint8_t  key_bytes[16];
    uint8_t  key_seq;
    uint8_t  src_eui[8];
    uint8_t  dst_eui[8];
} zb_aps_frm_t;

/* Parse APS layer from a MAC data frame's payload. */
int  zb_aps_parse(const uint8_t *aps, uint8_t len, zb_aps_frm_t *out);

/* Build a ZCL OnOff Toggle command payload (for cluster-command injection). */
int  zb_zcl_build_onoff(uint8_t *out, uint8_t seq, uint8_t cmd);

/* Build a ZCL DoorLock Unlock command payload. */
int  zb_zcl_build_doorlock_unlock(uint8_t *out, uint8_t seq);

/* Dissect a Transport-Key frame, extracting the encrypted network key blob. */
int  zb_aps_extract_transport_key(const zb_aps_frm_t *aps,
                                  uint8_t out_key[16],
                                  uint8_t *out_key_type,
                                  uint8_t *out_key_seq);

#endif /* ZIGBEE_PHANTOM_ZIGBEE_APS_H */