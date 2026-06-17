/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * nRF52840 BLE Bridge Driver
 *
 * Manages UART communication with the nRF52840 BLE module.
 * Implements the command/response protocol for the companion app.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef BLE_BRIDGE_H
#define BLE_BRIDGE_H

#include <stdint.h>
#include "../board.h"

/* =========================================================================
 * BLE Command IDs (mirrored in companion app protocol.js)
 * ========================================================================= */
#define BLE_CMD_GET_STATUS         0x01
#define BLE_CMD_STATUS_RESP        0x02
#define BLE_CMD_SET_MODE           0x03
#define BLE_CMD_SET_FSK_PARAMS     0x04
#define BLE_CMD_SET_OOK_PARAMS     0x05
#define BLE_CMD_TX_MESSAGE         0x06
#define BLE_CMD_TX_INTERLEAVED     0x07
#define BLE_CMD_RX_START           0x10
#define BLE_CMD_RX_STOP            0x11
#define BLE_CMD_RX_DATA            0x12
#define BLE_CMD_RX_SPECTRUM        0x13
#define BLE_CMD_RX_SIGNAL_QUALITY  0x14
#define BLE_CMD_BEACON_SET         0x20
#define BLE_CMD_BEACON_DATA        0x21
#define BLE_CMD_SET_POWER          0x30
#define BLE_CMD_SET_GAIN           0x31
#define BLE_CMD_STORE_CAPTURE      0x40
#define BLE_CMD_RETRIEVE_CAPTURE   0x41
#define BLE_CMD_ERASE_STORAGE      0x42
#define BLE_CMD_UPDATE_FIRMWARE    0x50
#define BLE_CMD_RESET              0xFF

/* =========================================================================
 * BLE Packet Format
 * =========================================================================
 * | Byte 0 | Bytes 1-2 | Bytes 3-4 | Bytes 5...N-2 | Bytes N-2, N-1 |
 * |  START |  LENGTH   |  CMD_ID   |   PAYLOAD     |   CRC-16 (LE)  |
 * |  0xAA  |  LE u16   |  LE u16   |   var         |   LE u16       |
 * ========================================================================= */

#define BLE_START_BYTE          0xAA
#define BLE_HEADER_SIZE         5       /* START(1) + LENGTH(2) + CMD(2) */
#define BLE_CRC_SIZE            2
#define BLE_MAX_PACKET_SIZE     (BLE_HEADER_SIZE + BLE_MAX_PAYLOAD + BLE_CRC_SIZE)
#define BLE_RX_FIFO_SIZE        512

/* =========================================================================
 * BLE Bridge State
 * ========================================================================= */

typedef struct {
    /* UART state */
    uint8_t  initialized;
    
    /* Rx packet parser state machine */
    enum {
        BLE_PARSE_START,      /* Waiting for START byte */
        BLE_PARSE_LENGTH,     /* Reading length (2 bytes) */
        BLE_PARSE_CMD,        /* Reading command (2 bytes) */
        BLE_PARSE_PAYLOAD,    /* Reading payload */
        BLE_PARSE_CRC         /* Reading CRC (2 bytes) */
    } parse_state;
    
    uint8_t  rx_buf[BLE_MAX_PACKET_SIZE];
    uint16_t rx_index;
    uint16_t expected_length;
    uint16_t expected_cmd;
    uint16_t expected_payload_len;
    
    /* Decoded command ready for processing */
    uint8_t  cmd_pending;
    uint16_t pending_cmd;
    uint16_t pending_payload_len;
    uint8_t  pending_payload[BLE_MAX_PAYLOAD];
    
    /* Tx buffer */
    uint8_t  tx_buf[BLE_MAX_PACKET_SIZE];
    uint16_t tx_len;
    uint8_t  tx_busy;
    
    /* Status info (updated by main application) */
    struct {
        uint8_t  battery_pct;       /* 0–100% */
        uint16_t battery_mv;        /* mV */
        uint8_t  mode;              /* Current device mode */
        uint8_t  signal_quality;    /* 0–100 */
        uint32_t storage_used;      /* Bytes used in QSPI */
        uint32_t storage_total;     /* Total bytes */
        uint8_t  tx_power;          /* Current Tx power level */
        uint8_t  rx_gain;           /* Current Rx gain level */
        uint32_t uptime_s;          /* Seconds since boot */
        uint8_t  fw_version_major;
        uint8_t  fw_version_minor;
        uint8_t  fw_version_patch;
    } status;
} ble_bridge_t;

/* Global bridge state */
extern ble_bridge_t g_ble;

/* =========================================================================
 * API Functions
 * ========================================================================= */

/**
 * Initialize the BLE bridge.
 * Sets up USART5 for communication with nRF52840 at 115200 baud.
 */
void ble_bridge_init(void);

/**
 * Process any incoming bytes from the BLE module UART.
 * Parses packets and sets cmd_pending when complete.
 * Call this periodically from main loop.
 */
void ble_bridge_process_rx(void);

/**
 * Send a command packet to the BLE module (which forwards to app).
 * 
 * @param cmd     Command ID
 * @param payload Payload bytes
 * @param len     Payload length
 * @return        0 on success, negative on error
 */
int ble_bridge_send(uint16_t cmd, const uint8_t *payload, uint16_t len);

/**
 * Send a response to a pending command.
 * Wraps data in a BLE packet and queues for UART transmission.
 * 
 * @param cmd     Response command ID (e.g., STATUS_RESP for GET_STATUS)
 * @param payload Response payload
 * @param len     Payload length
 * @return        0 on success, negative on error
 */
int ble_bridge_respond(uint16_t cmd, const uint8_t *payload, uint16_t len);

/**
 * Check if a command is pending from the app.
 * 
 * @return  1 if pending, 0 otherwise
 */
int ble_bridge_cmd_pending(void);

/**
 * Get the pending command.
 * 
 * @param out_cmd      Output: command ID
 * @param out_payload  Output: payload (caller must provide buffer)
 * @param out_len      Output: payload length
 * @return             0 on success
 */
int ble_bridge_get_cmd(uint16_t *out_cmd, uint8_t *out_payload, uint16_t *out_len);

/**
 * Mark the pending command as processed.
 */
void ble_bridge_cmd_consumed(void);

/**
 * Update the status fields (call from main loop periodically).
 */
void ble_bridge_update_status(void);

/**
 * Send periodic status update to connected app.
 * 
 * @return 0 on success
 */
int ble_bridge_send_status(void);

/**
 * Transmit a raw byte via USART5 (blocking).
 * 
 * @param c  Byte to send
 */
static inline void ble_uart_putc(uint8_t c)
{
    uintptr_t base = USART5_BASE;
    /* Wait for TX FIFO empty */
    while (!(mmio_read32(base + USART_ISR) & USART_ISR_TXE))
        ;
    mmio_write32(base + USART_TDR, c);
}

/**
 * Check if USART5 has a byte available.
 * 
 * @return 1 if byte available, 0 otherwise
 */
static inline int ble_uart_available(void)
{
    return (mmio_read32(USART5_BASE + USART_ISR) & USART_ISR_RXNE) ? 1 : 0;
}

/**
 * Read a byte from USART5 (non-blocking).
 * 
 * @param out  Output byte
 * @return     1 if byte read, 0 if none available
 */
static inline int ble_uart_read(uint8_t *out)
{
    if (!(mmio_read32(USART5_BASE + USART_ISR) & USART_ISR_RXNE))
        return 0;
    *out = (uint8_t)(mmio_read32(USART5_BASE + USART_RDR) & 0xFF);
    return 1;
}

/**
 * Send a block of bytes via USART5 (blocking).
 * 
 * @param data  Data to send
 * @param len   Length in bytes
 */
static inline void ble_uart_write(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        ble_uart_putc(data[i]);
}

#endif /* BLE_BRIDGE_H */