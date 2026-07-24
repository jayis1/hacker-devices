/*
 * smbus_port.h — SMBus/PMBus dual-port I2C driver header for Joule-Phantom.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Two independent SMBus ports are maintained:
 *   - HOST port  (I2C1)  -> faces the laptop / charger / host controller
 *   - BATT port  (I2C2)  -> faces the smart battery pack
 *
 * The driver is interrupt-driven and supports master and slave modes
 * simultaneously, enabling transparent bridging with per-frame MITM
 * hooks.
 */

#ifndef JOULE_PHANTOM_SMBUS_PORT_H
#define JOULE_PHANTOM_SMBUS_PORT_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Port identity */
typedef enum {
    SMB_PORT_HOST = 0,
    SMB_PORT_BATT = 1,
    SMB_PORT_COUNT
} smb_port_t;

/* Direction of a transaction relative to the port */
typedef enum {
    SMB_DIR_HOST_READ  = 0,   /* host reads from battery (read-word/block) */
    SMB_DIR_HOST_WRITE = 1,   /* host writes to battery                    */
    SMB_DIR_ALERT      = 2    /* SMBALERT# asserted                        */
} smb_dir_t;

/* A captured SMBus frame (host<->battery exchange) */
typedef struct {
    uint32_t   ts_ms;        /* timestamp (ms)          */
    smb_port_t port;         /* which port captured     */
    smb_dir_t  dir;          /* direction               */
    uint8_t    addr;         /* target slave addr (7-bit) */
    uint8_t    cmd;          /* SBS/PMBus command code  */
    uint8_t    wlen;         /* write payload length    */
    uint8_t    rlen;         /* read payload length     */
    uint8_t    wbuf[32];     /* write payload           */
    uint8_t    rbuf[32];     /* read payload            */
    uint8_t    flags;        /* see SMB_FLAG_* below    */
} smb_frame_t;

#define SMB_FLAG_NACK      0x01
#define SMB_FLAG_TIMEOUT   0x02
#define SMB_FLAG_MODIFIED  0x04
#define SMB_FLAG_INJECTED  0x08
#define SMB_FLAG_PEC_ERR   0x10

/* MITM rule engine -------------------------------------------------- */
typedef enum {
    MITM_ACT_NONE      = 0,
    MITM_ACT_SPOOF     = 1,   /* replace returned value               */
    MITM_ACT_BLOCK     = 2,   /* NACK the transaction                 */
    MITM_ACT_INJECT    = 3,   /* inject a fake host read on the fly   */
    MITM_ACT_LOG_ONLY  = 4,   /* capture but pass-through unmodified  */
    MITM_ACT_GLITCH    = 5    /* fire glitch pulse during frame       */
} mitm_action_t;

typedef struct {
    uint8_t       cmd;        /* SBS/PMBus command to match (0xFF=any) */
    uint8_t       mask;       /* match mask applied to cmd             */
    mitm_action_t action;     /* what to do                            */
    uint8_t       spoof_len;  /* length of spoof data                  */
    uint8_t       spoof[32];  /* replacement bytes                     */
    uint8_t       enabled;    /* rule active?                          */
} mitm_rule_t;

/* Public API -------------------------------------------------------- */

/* Initialise both SMBus ports (host + battery side). */
void smbus_init(void);

/* Slave-mode: listen on a given port for host/battery-originated traffic. */
void smbus_listen(smb_port_t port, uint8_t own_addr);

/* Master-mode: perform a blocking SBS read-word (cmd -> 2 bytes). */
int  smbus_read_word(smb_port_t port, uint8_t addr, uint8_t cmd, uint16_t *out);

/* Master-mode: perform a blocking SBS write-word. */
int  smbus_write_word(smb_port_t port, uint8_t addr, uint8_t cmd, uint16_t val);

/* Master-mode: SBS read-block (cmd -> up to 32 bytes). */
int  smbus_read_block(smb_port_t port, uint8_t addr, uint8_t cmd,
                      uint8_t *buf, uint8_t *len);

/* Bridge a single transaction: read from HOST side, forward to BATT,
 * apply MITM rules, return response to HOST.  Returns 0 on success. */
int  smbus_bridge_transaction(smb_frame_t *f);

/* Push a captured frame into the ring buffer (ISR-safe). */
void smbus_capture_push(const smb_frame_t *f);

/* Pop a captured frame from the ring buffer (main loop). */
int  smbus_capture_pop(smb_frame_t *out);

/* MITM rule management */
void          mitm_rules_clear(void);
void          mitm_rule_add(const mitm_rule_t *r);
uint8_t       mitm_rule_count(void);
const mitm_rule_t *mitm_rule_get(uint8_t idx);

/* Apply rules to a frame; returns the action taken. */
mitm_action_t mitm_apply(smb_frame_t *f);

/* ISR handlers (called from startup) */
void smbus_host_ev_isr(void);
void smbus_host_er_isr(void);
void smbus_batt_ev_isr(void);
void smbus_batt_er_isr(void);

#ifdef __cplusplus
}
#endif

#endif /* JOULE_PHANTOM_SMBUS_PORT_H */