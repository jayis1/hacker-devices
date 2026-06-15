/*
 * spi_radio.h — SPI interface driver for nRF52832 BLE radios
 *
 * Provides command/response interface to two nRF52832 radios
 * connected via SPI (Radio A on SPI1, Radio B on SPI2).
 */

#ifndef SPI_RADIO_H
#define SPI_RADIO_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * Radio instance identifiers
 * ============================================================ */
typedef enum {
    RADIO_A = 0,
    RADIO_B = 1,
    RADIO_COUNT = 2
} radio_id_t;

/* ============================================================
 * SPI command opcodes (sent from STM32 to nRF)
 * ============================================================ */
#define CMD_RADIO_INIT       0x01
#define CMD_START_ADV        0x02
#define CMD_START_SCAN       0x03
#define CMD_CONNECT          0x04
#define CMD_TX_DATA          0x05
#define CMD_RX_DATA          0x06
#define CMD_SET_CHANNEL      0x07
#define CMD_SET_TX_POWER     0x08
#define CMD_DISCONNECT       0x09
#define CMD_SET_PHY          0x0A
#define CMD_SET_ADDR         0x0B
#define CMD_SET_ADV_DATA     0x0C
#define CMD_SET_SCAN_FILTER  0x0D
#define CMD_GET_STATUS       0x10
#define CMD_GET_RSSI         0x11
#define CMD_GET_VERSION      0x12
#define CMD_RESET            0xFF

/* ============================================================
 * SPI protocol constants
 * ============================================================ */
#define SPI_SYNC_WORD        0xAA55U
#define SPI_TRAILER_WORD     0x55AAU
#define SPI_HEADER_SIZE      5    /* sync(2) + cmd(1) + len(2) */
#define SPI_MAX_PAYLOAD      255
#define SPI_CRC16_INIT       0xFFFFU

/* ============================================================
 * Radio status flags (returned by CMD_GET_STATUS)
 * ============================================================ */
#define RADIO_STATUS_INITIALIZED  (1U << 0)
#define RADIO_STATUS_SCANNING     (1U << 1)
#define RADIO_STATUS_ADVERTISING  (1U << 2)
#define RADIO_STATUS_CONNECTED    (1U << 3)
#define RADIO_STATUS_RX_PENDING   (1U << 4)
#define RADIO_STATUS_TX_COMPLETE  (1U << 5)
#define RADIO_STATUS_ERROR         (1U << 7)

/* ============================================================
 * BLE PHY modes
 * ============================================================ */
#define BLE_PHY_1M          1
#define BLE_PHY_2M          2
#define BLE_PHY_CODED       3

/* ============================================================
 * BLE channel definitions
 * ============================================================ */
#define BLE_CHAN_ADV_37     37
#define BLE_CHAN_ADV_38     38
#define BLE_CHAN_ADV_39     39
#define BLE_CHAN_DATA_MIN   0
#define BLE_CHAN_DATA_MAX   36

/* ============================================================
 * Radio configuration structure
 * ============================================================ */
typedef struct {
    uint8_t  channel;        /* BLE channel (0-36 data, 37-39 adv) */
    int8_t   tx_power;       /* -40 to +4 dBm */
    uint16_t adv_interval;  /* Advertising interval in 0.625 ms units */
    uint16_t scan_window;   /* Scan window in 0.625 ms units */
    uint16_t scan_interval; /* Scan interval in 0.625 ms units */
    uint8_t  phy;           /* 1M, 2M, coded */
} radio_config_t;

/* ============================================================
 * Received BLE packet structure
 * ============================================================ */
typedef struct {
    uint8_t  channel;       /* Channel number */
    int8_t   rssi;          /* Signal strength in dBm */
    uint8_t  length;        /* PDU length */
    uint8_t  data[255];     /* PDU data */
} radio_packet_t;

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Initialize SPI radio driver for given radio instance.
 * Resets the radio and sends CMD_RADIO_INIT.
 * @param radio  RADIO_A or RADIO_B
 * @return 0 on success, negative on error
 */
int spi_radio_init(radio_id_t radio);

/**
 * Send a command to the radio with optional payload.
 * @param radio   RADIO_A or RADIO_B
 * @param cmd     Command opcode (CMD_*)
 * @param payload Data payload (may be NULL if len==0)
 * @param len     Payload length (0-255)
 * @return 0 on success, negative on error
 */
int spi_radio_send_cmd(radio_id_t radio, uint8_t cmd,
                       const uint8_t *payload, uint16_t len);

/**
 * Read a response from the radio (non-blocking).
 * Check RADIO_STATUS_RX_PENDING first.
 * @param radio   RADIO_A or RADIO_B
 * @param cmd     Output: response command byte
 * @param payload Output: payload buffer (must hold SPI_MAX_PAYLOAD bytes)
 * @param len     Output: payload length
 * @return 0 on success, -1 if no data, -2 on sync error, -3 on CRC error
 */
int spi_radio_read_response(radio_id_t radio, uint8_t *cmd,
                            uint8_t *payload, uint16_t *len);

/**
 * Configure radio parameters (channel, power, intervals, PHY).
 * @param radio  RADIO_A or RADIO_B
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int spi_radio_configure(radio_id_t radio, const radio_config_t *config);

/**
 * Start BLE advertising on the given radio.
 * @param radio  RADIO_A or RADIO_B
 * @return 0 on success, negative on error
 */
int spi_radio_start_adv(radio_id_t radio);

/**
 * Start BLE scanning on the given radio.
 * @param radio  RADIO_A or RADIO_B
 * @return 0 on success, negative on error
 */
int spi_radio_start_scan(radio_id_t radio);

/**
 * Initiate a BLE connection to a specific address.
 * @param radio     RADIO_A or RADIO_B
 * @param addr      6-byte BLE device address (LSB first)
 * @param addr_type 0=public, 1=random
 * @return 0 on success, negative on error
 */
int spi_radio_connect(radio_id_t radio, const uint8_t *addr, uint8_t addr_type);

/**
 * Send data on an established BLE connection.
 * @param radio  RADIO_A or RADIO_B
 * @param data   Data to send
 * @param len    Data length
 * @return 0 on success, negative on error
 */
int spi_radio_send_data(radio_id_t radio, const uint8_t *data, uint16_t len);

/**
 * Get current radio status flags.
 * @param radio  RADIO_A or RADIO_B
 * @return Bitmask of RADIO_STATUS_* flags
 */
uint8_t spi_radio_get_status(radio_id_t radio);

/**
 * Reset the radio (hardware reset via RST pin).
 * @param radio  RADIO_A or RADIO_B
 * @return 0 on success
 */
int spi_radio_reset(radio_id_t radio);

/**
 * IRQ handler — call from EXTI ISR when radio asserts IRQ line.
 * @param radio  RADIO_A or RADIO_B
 */
void spi_radio_irq_handler(radio_id_t radio);

/**
 * CRC16-CCITT calculation for SPI frame integrity.
 * @param data Data bytes
 * @param len  Data length
 * @return 16-bit CRC
 */
uint16_t spi_crc16(const uint8_t *data, size_t len);

#endif /* SPI_RADIO_H */