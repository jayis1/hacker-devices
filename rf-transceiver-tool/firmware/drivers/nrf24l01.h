/*
 * nrf24l01.h — Nordic nRF24L01+ 2.4 GHz Transceiver Driver
 *
 * Driver for the nRF24L01+ connected via SPI2 on STM32F401CCU6:
 *   PB12 = CSN (bit-banged)
 *   PB13 = SCK (SPI2_SCK)
 *   PB14 = MISO (SPI2_MISO)
 *   PB15 = MOSI (SPI2_MOSI)
 *   PA15 = CE (Chip Enable)
 *   PB3  = IRQ (Active-low interrupt)
 */

#ifndef NRF24L01_H
#define NRF24L01_H

#include <stdint.h>

/* ========================================================================
 * nRF24L01+ Data Types
 * ======================================================================== */

/* Data rate */
typedef enum {
    NRF24_DR_250K = 0,    /* 250 kbps */
    NRF24_DR_1M = 1,       /* 1 Mbps */
    NRF24_DR_2M = 2,       /* 2 Mbps */
} nrf24_datarate_t;

/* TX Power */
typedef enum {
    NRF24_PWR_MINUS_18DBM = 0,  /* -18 dBm */
    NRF24_PWR_MINUS_12DBM = 1,  /* -12 dBm */
    NRF24_PWR_MINUS_6DBM = 2,   /* -6 dBm */
    NRF24_PWR_0DBM = 3,         /* 0 dBm */
} nrf24_power_t;

/* CRC encoding */
typedef enum {
    NRF24_CRC_OFF = 0,
    NRF24_CRC_1BYTE = 1,
    NRF24_CRC_2BYTE = 2,
} nrf24_crc_t;

/* Address width */
typedef enum {
    NRF24_AW_3 = 1,  /* 3 bytes */
    NRF24_AW_4 = 2,  /* 4 bytes */
    NRF24_AW_5 = 3,  /* 5 bytes */
} nrf24_addr_width_t;

/* RX pipe */
typedef enum {
    NRF24_PIPE_0 = 0,
    NRF24_PIPE_1 = 1,
    NRF24_PIPE_2 = 2,
    NRF24_PIPE_3 = 3,
    NRF24_PIPE_4 = 4,
    NRF24_PIPE_5 = 5,
} nrf24_pipe_t;

/* Configuration structure */
typedef struct {
    uint8_t channel;             /* RF channel (0-127) */
    nrf24_datarate_t datarate;
    nrf24_power_t tx_power;
    nrf24_crc_t crc_encoding;
    nrf24_addr_width_t addr_width;
    uint8_t auto_ack;           /* Bitmask: which pipes have auto-ack */
    uint8_t rx_pipes;           /* Bitmask: which pipes are enabled */
    uint8_t payload_width;      /* 1-32 bytes (0 = dynamic) */
    uint8_t auto_retr_count;    /* 0-15 (0 = disabled) */
    uint16_t auto_retr_delay;   /* 250-4000 µs (in 250 µs steps) */
} nrf24_config_t;

/* Received packet */
typedef struct {
    uint8_t data[32];            /* Payload (max 32 bytes) */
    uint8_t length;              /* Payload length */
    uint8_t pipe;                /* RX pipe number */
    int8_t rssi;                 /* Approximate RSSI (only valid in RX mode) */
} nrf24_packet_t;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

/**
 * nrf24_init - Initialize nRF24L01+ to known state
 *
 * Resets the chip, verifies SPI communication, and applies
 * default configuration (channel 76, 2 Mbps, 0 dBm, 5-byte addr).
 *
 * Returns: 0 on success, -1 on error
 */
int nrf24_init(void);

/**
 * nrf24_configure - Apply a complete configuration
 * @param cfg Configuration structure
 * Returns: 0 on success, -1 on error
 */
int nrf24_configure(const nrf24_config_t *cfg);

/**
 * nrf24_set_channel - Set RF channel
 * @param channel Channel number (0-127, freq = 2400 + channel MHz)
 */
void nrf24_set_channel(uint8_t channel);

/**
 * nrf24_set_datarate - Set data rate
 * @param dr Data rate (250K, 1M, 2M)
 */
void nrf24_set_datarate(nrf24_datarate_t dr);

/**
 * nrf24_set_tx_power - Set TX output power
 * @param power Power level enum
 */
void nrf24_set_tx_power(nrf24_power_t power);

/**
 * nrf24_set_rx_addr - Set RX address for a pipe
 * @param pipe Pipe number (0-5)
 * @param addr Pointer to address bytes (LSB first, 3-5 bytes)
 * @param len Address width (3-5)
 */
void nrf24_set_rx_addr(nrf24_pipe_t pipe, const uint8_t *addr, uint8_t len);

/**
 * nrf24_set_tx_addr - Set TX address
 * @param addr Pointer to address bytes (LSB first, 3-5 bytes)
 * @param len Address width (3-5)
 */
void nrf24_set_tx_addr(const uint8_t *addr, uint8_t len);

/**
 * nrf24_enable_pipe - Enable RX on a specific pipe
 * @param pipe Pipe number (0-5)
 * @param auto_ack Enable auto-acknowledgement
 * @param payload_width Static payload width (0 = dynamic)
 */
void nrf24_enable_pipe(nrf24_pipe_t pipe, uint8_t auto_ack, uint8_t payload_width);

/**
 * nrf24_disable_pipe - Disable RX on a specific pipe
 * @param pipe Pipe number (0-5)
 */
void nrf24_disable_pipe(nrf24_pipe_t pipe);

/**
 * nrf24_tx_mode - Enter TX mode
 *
 * Powers up the chip, sets PRIM_RX=0, and asserts CE.
 * Returns: 0 on success
 */
int nrf24_tx_mode(void);

/**
 * nrf24_rx_mode - Enter RX mode
 *
 * Powers up the chip, sets PRIM_RX=1, and asserts CE.
 * Returns: 0 on success
 */
int nrf24_rx_mode(void);

/**
 * nrf24_power_down - Power down the chip
 *
 * Clears PWR_UP and de-asserts CE.
 */
void nrf24_power_down(void);

/**
 * nrf24_transmit - Transmit a packet
 * @param data Payload data (1-32 bytes)
 * @param len Payload length
 *
 * Loads TX FIFO, pulses CE, and waits for TX complete or max retransmit.
 *
 * Returns: 0 on success (ACK received), -1 on max retransmit, -2 on timeout
 */
int nrf24_transmit(const uint8_t *data, uint8_t len);

/**
 * nrf24_receive - Check for received packet
 * @param pkt Pointer to packet structure to fill
 *
 * Non-blocking check. If a packet is available in RX FIFO, reads it.
 *
 * Returns: 1 if packet available, 0 if no packet, -1 on error
 */
int nrf24_receive(nrf24_packet_t *pkt);

/**
 * nrf24_scan_channel - Check if channel has activity
 * @param channel Channel number (0-127)
 * @param threshold_dbm RSSI threshold (not used, uses RPD detector)
 * @param dwell_us Dwell time in microseconds (min 128 µs for RPD)
 *
 * Uses the Received Power Detector (RPD) feature.
 *
 * Returns: 1 if signal detected, 0 if clear
 */
int nrf24_scan_channel(uint8_t channel, uint32_t dwell_us);

/**
 * nrf24_flush_tx - Flush TX FIFO
 */
void nrf24_flush_tx(void);

/**
 * nrf24_flush_rx - Flush RX FIFO
 */
void nrf24_flush_rx(void);

/**
 * nrf24_get_status - Read STATUS register
 * Returns: Current STATUS register value
 */
uint8_t nrf24_get_status(void);

/**
 * nrf24_clear_interrupts - Clear all interrupt flags
 */
void nrf24_clear_interrupts(void);

/* ========================================================================
 * SPI Access Functions (Internal)
 * ======================================================================== */

/**
 * nrf24_spi_write - Write a single register
 * @param reg Register address
 * @param value Value to write
 */
void nrf24_spi_write(uint8_t reg, uint8_t value);

/**
 * nrf24_spi_read - Read a single register
 * @param reg Register address
 * Returns: Register value
 */
uint8_t nrf24_spi_read(uint8_t reg);

/**
 * nrf24_spi_write_buf - Write multiple bytes to a register
 * @param reg Register address (typically RX_ADDR_P0 or TX_ADDR)
 * @param buf Data buffer (LSB first)
 * @param len Number of bytes
 */
void nrf24_spi_write_buf(uint8_t reg, const uint8_t *buf, uint8_t len);

/**
 * nrf24_spi_read_buf - Read multiple bytes from a register
 * @param reg Register address
 * @param buf Data buffer (LSB first)
 * @param len Number of bytes
 */
void nrf24_spi_read_buf(uint8_t reg, uint8_t *buf, uint8_t len);

#endif /* NRF24L01_H */