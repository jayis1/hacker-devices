/*
 * drivers/cc2652_rf.h — CC2652R1F radio core command API wrapper
 * Author: jayis1
 * License: GPL-2.0
 *
 * Wraps the CC2652 RF doorbell command interface for 802.15.4 promiscuous
 * RX/TX. The CC2652 has a dedicated Cortex-M0 radio core that runs the
 * 802.15.4 PHY/MAC; the application core issues commands via the RFC doorbell.
 */
#ifndef ZIGBEE_PHANTOM_CC2652_RF_H
#define ZIGBEE_PHANTOM_CC2652_RF_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"   /* for IEEE802154_MAX_FRM_LEN */

/* RF command opcodes (IEEE mode) */
#define RF_CMD_IEEE_RX          0x0001
#define RF_CMD_IEEE_TX          0x0002
#define RF_CMD_IEEE_FS          0x0003   /* frequency synth */
#define RF_CMD_IEEE_ABORT       0x0004
#define RF_CMD_IEEE_RX_NOK      0x0005   /* reject ACK */
#define RF_CMD_IEEE_RX_ACK      0x0006
#define RF_CMD_IEEE_SETUP       0x0007

/* RF command status bits (in command struct status field) */
#define RF_STATUS_IDLE          0x00
#define RF_STATUS_ACTIVE        0x01
#define RF_STATUS_DONE          0x04
#define RF_STATUS_ERROR         0x0F

/* RF command struct layout (simplified for doorbell submission).
 * The first word is the command opcode; the second word is the status. */
typedef struct {
    uint16_t commandNo;       /* opcode */
    uint16_t status;          /* status written by radio core */
    uint32_t pNextOp;          /* pointer to next chained command (0 = none) */
    uint32_t startTime;        /* absolute or relative start time */
    uint32_t startTrigger;    /* trigger type */
    uint32_t condition;        /* rule for next command */
} rf_cmd_hdr_t;

/* IEEE RX command (promiscuous capture) */
typedef struct {
    rf_cmd_hdr_t hdr;
    uint8_t  channel;         /* 11..26 */
    uint8_t  rxConfig;         /* auto-ACK, auto-filter flags (0=promiscuous) */
    uint8_t  preambleTxBytes;
    uint8_t  maxPreambleLen;
    uint8_t  syncWord[2];
    uint16_t bslLen;           /* beacon-ness length */
    uint8_t  endTrigger;
    uint8_t  reserved;
    uint32_t endTime;
    /* RX buffer pointers (set up by RF core) */
    uint32_t pOutput;          /* pointer to RX entry queue */
    uint8_t  frameFilter;      /* 0 = accept all (promiscuous) */
    uint8_t  autoAckPendLen;
    uint8_t  autoAckPend[16];
    uint32_t pRxQ;             /* pointer to RX queue */
} rf_cmd_ieee_rx_t;

/* IEEE TX command */
typedef struct {
    rf_cmd_hdr_t hdr;
    uint8_t  txConfig;         /* 0 = no auto-ACK wait */
    uint8_t  txLen;            /* payload length (including FCS) */
    uint8_t  *pPayload;        /* payload buffer */
    uint8_t  endTrigger;
    uint8_t  reserved;
    uint32_t endTime;
    uint8_t  ackConfig;        /* ACK wait time / retransmits */
    uint8_t  seqnum;
    uint8_t  reserved2[2];
} rf_cmd_ieee_tx_t;

/* A single RX entry in the RX queue (data queue element). */
typedef struct {
    uint8_t  *pNextEntry;      /* pointer to next entry (circular) */
    uint8_t  status;           /* 0 = pending, 1 = done */
    uint8_t  length;           /* payload length */
    int8_t   rssi;             /* RSSI in dBm */
    uint8_t  lqi;              /* link-quality indicator */
    uint8_t  channel;
    uint8_t  frame[IEEE802154_MAX_FRM_LEN];
} rf_rx_entry_t;

/* API */
int  rf_init(void);
int  rf_set_channel(uint8_t channel);          /* 11..26 */
int  rf_start_promiscuous_rx(void);             /* promiscuous mode */
int  rf_stop_rx(void);
int  rf_tx_frame(const uint8_t *frame, uint8_t len);  /* blocking TX */
int  rf_set_tx_power(int8_t dbm);                /* -20..+20 (with nRF21540 FEM) */
int  rf_energy_scan(int8_t rssi_out[16]);        /* 16-channel RSSI sweep */
int  rf_antenna_select(uint8_t ant);             /* 0=ant1, 1=ant2 */
int  rf_fem_enable(bool pa, bool lna);           /* enable PA/LNA in FEM */
int  rf_fem_gain(uint8_t gain);                  /* 0=low, 1=high (+20 dBm) */
bool rf_rx_pending(void);                         /* new frame in queue? */
int  rf_rx_read(rf_rx_entry_t *out);              /* pop one frame from queue */
uint32_t rf_get_timestamp_us(void);              /* AON_RTC timestamp */

#endif /* ZIGBEE_PHANTOM_CC2652_RF_H */