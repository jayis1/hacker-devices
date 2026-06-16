/**
 * drivers/usb_ft601.h — FT601 USB 3.0 FIFO Bridge Driver Interface
 *
 * PCIe Screamer — NVMe Interposer
 * Author: Nous Research
 * Date: 2026-06-16
 */

#ifndef USB_FT601_H
#define USB_FT601_H

#include <stdint.h>
#include <stddef.h>

/*============================================================================
 * FT601 FIFO INTERFACE SIGNALS
 *============================================================================*/

/* Signal direction (from FPGA perspective) */
#define FT601_SIG_INPUT   0
#define FT601_SIG_OUTPUT  1

/* Control signal states */
#define FT601_TXE_EMPTY   0  /* TXE# low = FIFO has space */
#define FT601_TXE_FULL    1  /* TXE# high = FIFO full */
#define FT601_RXF_DATA    0  /* RXF# low = data available */
#define FT601_RXF_EMPTY   1  /* RXF# high = no data */

/*============================================================================
 * WIRE PROTOCOL CONSTANTS
 *============================================================================*/

/* Sync marker: "SCREAM" */
#define SYNC_MARKER_0     0x53  /* 'S' */
#define SYNC_MARKER_1     0x43  /* 'C' */
#define SYNC_MARKER_2     0x52  /* 'R' */
#define SYNC_MARKER_3     0x45  /* 'E' */
#define SYNC_MARKER_4     0x41  /* 'A' */
#define SYNC_MARKER_5     0x4D  /* 'M' */

/* Command IDs */
#define CMD_CAPTURE_START       0x01
#define CMD_CAPTURE_STOP        0x02
#define CMD_CAPTURE_DATA        0x03
#define CMD_CAPTURE_OVERFLOW    0x04
#define CMD_INJECT_TLP          0x10
#define CMD_INJECT_RESULT       0x11
#define CMD_QUERY_STATUS        0x20
#define CMD_STATUS_RESPONSE     0x21
#define CMD_READ_REG            0x22
#define CMD_READ_REG_RESP       0x23
#define CMD_WRITE_REG           0x24
#define CMD_WRITE_REG_ACK       0x25
#define CMD_FW_UPDATE_INIT      0x30
#define CMD_FW_UPDATE_DATA      0x31
#define CMD_FW_UPDATE_CRC       0x32
#define CMD_FW_UPDATE_COMMIT    0x33
#define CMD_FW_UPDATE_RESULT    0x34
#define CMD_SELF_TEST_START     0x40
#define CMD_SELF_TEST_RESULT    0x41
#define CMD_ERROR               0xF0
#define CMD_ACK                 0xF1
#define CMD_NACK                0xF2
#define CMD_RESET               0xFF

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * ft601_init — Initialize FT601 USB 3.0 FIFO bridge
 *
 * Sequence:
 *   1. Assert FT_RESET# low, wait 1 ms
 *   2. Start 100 MHz clock to FT601
 *   3. Release FT_RESET#, wait 10 ms
 *   4. Wait for USB enumeration (SUSPEND# goes low)
 *
 * Returns: 0 on success, -1 on enumeration timeout
 */
int ft601_init(void);

/**
 * ft601_reset — Reset the FT601 bridge
 *
 * Pulses FT_RESET# and re-initializes.
 */
void ft601_reset(void);

/**
 * ft601_is_enumerated — Check if USB is enumerated
 *
 * Returns: 1 if enumerated and active, 0 otherwise
 */
int ft601_is_enumerated(void);

/**
 * ft601_tx_packet — Transmit a raw packet over USB
 *
 * Writes data to the FT601 TX FIFO using the synchronous
 * FIFO protocol (TXE# handshake, WR# strobe).
 *
 * @param data       Packet data
 * @param len_bytes  Packet length (must be multiple of 4)
 * @return           0 on success, negative on error
 */
int ft601_tx_packet(const uint8_t *data, uint16_t len_bytes);

/**
 * ft601_rx_packet — Receive a raw packet from USB
 *
 * Reads data from the FT601 RX FIFO using the synchronous
 * FIFO protocol (RXF# handshake, RD# strobe).
 *
 * @param buf      Output buffer
 * @param max_len  Maximum bytes to receive
 * @return         Number of bytes received, 0 if no data, negative on error
 */
int ft601_rx_packet(uint8_t *buf, uint16_t max_len);

/**
 * ft601_tx_response — Transmit a framed response packet
 *
 * Builds a complete wire-protocol frame with sync marker,
 * command ID, length, payload, and CRC-32C, then transmits it.
 *
 * @param cmd         Command ID
 * @param payload     Payload data
 * @param payload_len Payload length in bytes
 * @return            0 on success, negative on error
 */
int ft601_tx_response(uint8_t cmd, const uint8_t *payload,
                      uint16_t payload_len);

/**
 * ft601_tx_capture_data — Transmit a captured TLP data frame
 *
 * Specialized transmit for CMD_CAPTURE_DATA frames which
 * include timestamp, direction, TLP type, and raw TLP bytes.
 *
 * @param record      TLP record header + raw TLP bytes
 * @param record_len  Total record length
 * @return            0 on success, negative on error
 */
int ft601_tx_capture_data(const uint8_t *record, uint16_t record_len);

/**
 * ft601_get_tx_bytes — Get total bytes transmitted
 *
 * Returns: Cumulative TX byte count
 */
uint32_t ft601_get_tx_bytes(void);

/**
 * ft601_get_rx_bytes — Get total bytes received
 *
 * Returns: Cumulative RX byte count
 */
uint32_t ft601_get_rx_bytes(void);

#endif /* USB_FT601_H */
