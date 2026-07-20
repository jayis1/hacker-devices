/*
 * hart-bleeder — hart_stack.h
 * HART protocol stack: framing, checksums, command dispatch,
 * transaction management and session tracking for the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_HART_STACK_H
#define HART_BLEEDER_HART_STACK_H

#include <stdint.h>
#include "board.h"

/* ── HART fieldbus frame format (UART, 8-O-1 over FSK) ──────────
 *
 *  Preamble* | Start | Addr | Cmd | ByteCnt | [Status] | Data | Chk
 *  0xFF (5+) | 0x06/0x82 frame types | short/long addr | 1B cmd |
 *    length byte | [expansion status bytes for long frames] |
 *    0-253 byte data | XOR checksum
 *
 * Short frame (point-to-point, single master): no expansion bytes.
 * Long frame (multi-drop): 2 expansion status bytes + 1 byte-count.
 */

#define HART_SHORT_START   0x06
#define HART_LONG_START    0x82

#define HART_ADDR_SHORT_MSK 0x80  /* bit7 set = primary master in short frame */
#define HART_CMD_BYTE       1    /* position in decoded frame */

#define HART_MAX_TX_RETRIES 3
#define HART_INTERCHAR_TIMEOUT_US 280000UL

/* ── HART command identifiers used by Bleeder ─────────────────── */
#define HART_CMD_READ_UNIQUE_ID    0
#define HART_CMD_READ_PV            1  /* primary variable */
#define HART_CMD_READ_LOOP_CURRENT  2
#define HART_CMD_READ_DV0           3  /* dynamic variable 0 (PV) */
#define HART_CMD_WRITE_LOOP_MODE   8  /* (writes) */
#define HART_CMD_READ_PV_STATUS    9
#define HART_CMD_READ_UNIQUE_TAG   12
#define HART_CMD_READ_TAG_DESC     13
#define HART_CMD_READ_PV_INFO      14
#define HART_CMD_POLL_SUBDEVICE     6  /* sub-device poll */
#define HART_CMD_WRITE_PV          35 /* final element setpoint */
#define HART_CMD_READ_DEV_INFO     11
#define HART_CMD_WRITE_TAG         17
#define HART_CMD_WRITE_POLL_ADDR   35 /* also used for long addr */

#define HART_RSP_BITS 0x80  /* bit7 set in cmd byte = response */

#define HART_STATUS_NO_RESPONSE    0
#define HART_STATUS_OK             0x00
#define HART_STATUS_INVALID_CMD    0x02
#define HART_STATUS_TOO_FEW        0x03
#define HART_STATUS_BUSY           0x06
#define HART_STATUS_ACCESS_RESTRICTED 0x21
#define HART_STATUS_HARDWARE_FAIL  0x0E

/* ── Transaction state ────────────────────────────────────────── */
typedef enum {
    HART_TX_IDLE = 0,
    HART_TX_SENDING,
    HART_TX_WAITING_RX,
    HART_TX_DONE,
    HART_TX_TIMEOUT,
} hart_tx_state_t;

/* ── Decoded frame ────────────────────────────────────────────── */
typedef struct {
    uint8_t  start;        /* 0x06 or 0x82 */
    uint8_t  addr;         /* address byte (short) or 0x80-masked */
    uint8_t  cmd;          /* command byte */
    uint8_t  bytecnt;      /* data byte count */
    uint8_t  status[2];    /* expansion status (long frame) */
    uint8_t  data[HART_MAX_FRAME];
    uint16_t data_len;
    uint8_t  chk;          /* received checksum */
    uint8_t  chk_calc;     /* calculated checksum */
    int      is_long;      /* long frame flag */
    int      is_response;  /* response flag */
    uint32_t ts_ms;        /* arrival timestamp */
} hart_frame_t;

/* ── Transaction descriptor ───────────────────────────────────── */
typedef struct {
    uint8_t            cmd;
    uint8_t            addr;
    const uint8_t     *payload;
    uint16_t           payload_len;
    uint16_t           max_retries;
    uint32_t           timeout_ms;
    hart_tx_state_t    state;
    uint16_t           attempts;
    hart_frame_t       response;
    int                got_response;
} hart_transaction_t;

/* ── Master list (polling) ────────────────────────────────────── */
#define HART_MAX_POLLED_DEVICES 32

typedef struct {
    uint8_t  poll_addr;        /* 0..63 multi-drop */
    uint8_t  long_addr[5];     /* unique ID */
    uint8_t  tag[6];           /* device tag */
    uint8_t  desc[12];        /* descriptor */
    uint8_t  manufacturer;    /* manufacturer code */
    uint8_t  dev_type;        /* device type code */
    uint8_t  cmd_0_done;
    uint8_t  cmd_48_done;
} hart_device_t;

/* ── Global stack state ───────────────────────────────────────── */
typedef struct {
    hart_device_t   devices[HART_MAX_POLLED_DEVICES];
    uint8_t         n_devices;
    uint8_t         master_role;  /* 0 = primary, 1 = secondary */
    uint32_t        interframe_ms;
    uint32_t        stats_tx;
    uint32_t        stats_rx;
    uint32_t        stats_timeout;
    uint32_t        stats_crc_err;
    uint32_t        stats_collisions;
} hart_stack_t;

/* ── API ──────────────────────────────────────────────────────── */
int  hart_init(void);
void hart_set_master(uint8_t primary);
uint32_t hart_get_interframe(void);
void hart_set_interframe(uint32_t ms);

int  hart_send_cmd(uint8_t addr, uint8_t cmd,
                   const uint8_t *payload, uint16_t len,
                   hart_frame_t *resp, uint32_t timeout_ms);

int  hart_send_raw(const uint8_t *buf, uint16_t len);
int  hart_recv_frame(hart_frame_t *f, uint32_t timeout_ms);
int  hart_poll_addr(uint8_t poll_addr, hart_device_t *dev);
int  hart_poll_burst(uint8_t poll_addr);
int  hart_enumerate_bus(void);

uint8_t hart_checksum(const uint8_t *buf, uint16_t len);
int     hart_build_frame(uint8_t *buf, uint16_t *len,
                          uint8_t addr, uint8_t cmd,
                          const uint8_t *data, uint16_t data_len,
                          int long_frame, int is_response);

int     hart_parse_frame(const uint8_t *buf, uint16_t len, hart_frame_t *f);
const char *hart_cmd_name(uint8_t cmd);
const char *hart_status_str(uint8_t code);

extern hart_stack_t g_hart;

#endif /* HART_BLEEDER_HART_STACK_H */