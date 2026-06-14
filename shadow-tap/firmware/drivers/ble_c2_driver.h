/*
 * ble_c2_driver.h — ShadowTap BLE Command & Control driver
 *
 * Manages the UART link to the nRF52840-M.2 BLE module.
 * Provides encrypted SLIP-framed command/response protocol
 * for the companion app to control the tap.
 */

#ifndef SHADOWTAP_BLE_C2_DRIVER_H
#define SHADOWTAP_BLE_C2_DRIVER_H

#include "board.h"
#include "registers.h"

/* C2 Command opcodes */
#define BLE_C2_CMD_NOP          0x00U
#define BLE_C2_CMD_GET_STATUS   0x01U
#define BLE_C2_CMD_ADD_RULE    0x02U
#define BLE_C2_CMD_DEL_RULE    0x03U
#define BLE_C2_CMD_LIST_RULES  0x04U
#define BLE_C2_CMD_START_CAP   0x05U
#define BLE_C2_CMD_STOP_CAP    0x06U
#define BLE_C2_CMD_STREAM_PCAP 0x07U
#define BLE_C2_CMD_SET_MODE    0x08U  /* 0=tap, 1=mitm */
#define BLE_C2_CMD_GET_RULES   0x09U
#define BLE_C2_CMD_PING        0x0AU

/* C2 Response opcodes (high bit set) */
#define BLE_C2_RSP_FLAG        0x80U
#define BLE_C2_RSP_STATUS      (0x01U | BLE_C2_RSP_FLAG)
#define BLE_C2_RSP_RULE_ADDED  (0x02U | BLE_C2_RSP_FLAG)
#define BLE_C2_RSP_RULE_DEL    (0x03U | BLE_C2_RSP_FLAG)
#define BLE_C2_RSP_RULE_LIST   (0x04U | BLE_C2_RSP_FLAG)
#define BLE_C2_RSP_CAP_START   (0x05U | BLE_C2_RSP_FLAG)
#define BLE_C2_RSP_CAP_STOP    (0x06U | BLE_C2_RSP_FLAG)
#define BLE_C2_RSP_PONG        (0x0AU | BLE_C2_RSP_FLAG)
#define BLE_C2_RSP_ERROR       (0xFFU | BLE_C2_RSP_FLAG)

/* C2 Packet structure (before SLIP encoding) */
typedef struct __attribute__((packed)) {
    uint8_t  opcode;
    uint8_t  seq;
    uint16_t payload_len;
    uint8_t  payload[BLE_C2_MAX_CMD_LEN - 4];
    uint16_t crc16;
} ble_c2_packet_t;

/* BLE C2 driver state */
typedef struct {
    uint8_t  rx_buf[sizeof(ble_c2_packet_t) * 2]; /* SLIP can double size */
    uint16_t rx_len;
    uint8_t  tx_buf[sizeof(ble_c2_packet_t) * 2];
    uint16_t tx_len;
    uint8_t  rx_escaping;  /* SLIP escape state */
    uint8_t  tx_seq;       /* Sequence counter */
    uint8_t  connected;    /* BLE connection established */
    uint8_t  encrypted;     /* Encryption enabled */
    uint8_t  aes_key[32];  /* AES-256 key */
    uint8_t  aes_nonce[12]; /* AES-256-CTR nonce */
} ble_c2_state_t;

/* Function prototypes */

/**
 * Initialize BLE C2 driver: configure LPUART1, reset state.
 */
void ble_c2_init(void);

/**
 * Process an incoming byte from LPUART1 RX ISR.
 * Handles SLIP decoding and packet assembly.
 * @param byte  Received byte from UART
 */
void ble_c2_rx_byte(uint8_t byte);

/**
 * Process pending commands (call from main loop).
 * Parses complete packets and dispatches handlers.
 */
void ble_c2_process(void);

/**
 * Send a response packet to BLE module.
 * @param opcode  Response opcode (with RSP_FLAG)
 * @param payload Response payload data
 * @param len     Payload length
 */
void ble_c2_send_response(uint8_t opcode, const uint8_t *payload, uint16_t len);

/**
 * Send asynchronous notification to BLE (e.g., rule match event).
 * @param event_type  Notification type
 * @param data       Notification data
 * @param len        Data length
 */
void ble_c2_notify(uint8_t event_type, const uint8_t *data, uint16_t len);

/**
 * Set AES-256 encryption key for C2 channel.
 * @param key   32-byte AES key
 * @param nonce 12-byte nonce
 */
void ble_c2_set_crypto(const uint8_t key[32], const uint8_t nonce[12]);

/**
 * Check if BLE connection is active.
 * @return 1 if connected, 0 if not
 */
uint8_t ble_c2_is_connected(void);

#endif /* SHADOWTAP_BLE_C2_DRIVER_H */