/*
 * cc1101.h — TI CC1101 Sub-GHz Transceiver Driver
 *
 * Production-quality SPI driver for the CC1101 with DMA support.
 * Handles initialization, configuration, TX, RX, RSSI scanning,
 * and packet capture for security research applications.
 *
 * Connected via SPI1 on STM32F401CCU6:
 *   PA4  = CSN (bit-banged)
 *   PA5  = SCK (SPI1_SCK)
 *   PA6  = MISO (SPI1_MISO)
 *   PA7  = MOSI (SPI1_MOSI)
 *   PA3  = RESETn
 *   PA8  = GDO0
 *   PA9  = GDO2
 */

#ifndef CC1101_H
#define CC1101_H

#include <stdint.h>
#include <stddef.h>

/* ========================================================================
 * CC1101 Configuration Structures
 * ======================================================================== */

/* Modulation formats */
typedef enum {
    CC1101_MOD_2FSK = 0x00,
    CC1101_MOD_GFSK = 0x10,
    CC1101_MOD_ASK_OOK = 0x30,
    CC1101_MOD_4FSK = 0x40,
    CC1101_MOD_MSK = 0x70,
} cc1101_modulation_t;

/* Data rates */
typedef enum {
    CC1101_DRATE_1_2K = 0,    /* 1.2 kbps */
    CC1101_DRATE_2_4K = 1,    /* 2.4 kbps */
    CC1101_DRATE_4_8K = 2,    /* 4.8 kbps */
    CC1101_DRATE_9_6K = 3,    /* 9.6 kbps */
    CC1101_DRATE_15_6K = 4,   /* 15.6 kbps */
    CC1101_DRATE_38_4K = 5,  /* 38.4 kbps */
    CC1101_DRATE_76_8K = 6,  /* 76.8 kbps */
    CC1101_DRATE_250K = 7,   /* 250 kbps */
    CC1101_DRATE_500K = 8,   /* 500 kbps */
} cc1101_datarate_t;

/* TX power levels (at 868 MHz) */
typedef enum {
    CC1101_POWER_MINUS_30DBM = 0x00,  /* -30 dBm */
    CC1101_POWER_MINUS_20DBM = 0x01,  /* -20 dBm */
    CC1101_POWER_MINUS_15DBM = 0x02,  /* -15 dBm */
    CC1101_POWER_MINUS_10DBM = 0x05,  /* -10 dBm */
    CC1101_POWER_0DBM = 0x0C,        /* 0 dBm */
    CC1101_POWER_5DBM = 0x0F,        /* +5 dBm */
    CC1101_POWER_7DBM = 0x11,        /* +7 dBm */
    CC1101_POWER_10DBM = 0x1D,       /* +10 dBm */
    CC1101_POWER_12DBM = 0x50,       /* +12 dBm */
} cc1101_power_t;

/* RX bandwidth */
typedef enum {
    CC1101_BW_58K = 0,   /* 58 kHz */
    CC1101_BW_68K = 1,   /* 68 kHz */
    CC1101_BW_81K = 2,   /* 81 kHz */
    CC1101_BW_102K = 3,  /* 102 kHz */
    CC1101_BW_135K = 4,  /* 135 kHz */
    CC1101_BW_203K = 5,  /* 203 kHz */
    CC1101_BW_270K = 6,  /* 270 kHz */
    CC1101_BW_541K = 7,  /* 541 kHz */
} cc1101_bandwidth_t;

/* CC1101 operating state */
typedef enum {
    CC1101_STATE_IDLE = 0,
    CC1101_STATE_RX = 1,
    CC1101_STATE_TX = 2,
    CC1101_STATE_FSTXON = 3,
    CC1101_STATE_CALIBRATE = 4,
    CC1101_STATE_RX_OVERFLOW = 6,
    CC1101_STATE_TX_UNDERFLOW = 7,
} cc1101_state_t;

/* CC1101 configuration for a given frequency band */
typedef struct {
    uint32_t frequency_hz;         /* Operating frequency in Hz */
    cc1101_modulation_t modulation;
    cc1101_datarate_t datarate;
    cc1101_power_t tx_power;
    cc1101_bandwidth_t rx_bandwidth;
    uint8_t channel_number;        /* Channel offset from base freq */
    uint16_t sync_word;            /* Sync word (0xD3A1 default) */
    uint8_t address;               /* Device address filter (0 = no filter) */
    uint8_t pkt_length;            /* 0 = variable, 1-255 = fixed */
    uint8_t crc_enabled : 1;
    uint8_t manchester : 1;
    uint8_t whitening : 1;
} cc1101_config_t;

/* CC1101 received packet */
typedef struct {
    uint8_t data[64];              /* Payload data */
    uint8_t length;                /* Payload length */
    int8_t rssi_dbm;               /* RSSI in dBm */
    uint8_t lqi;                   /* Link Quality Indicator */
    uint8_t crc_ok : 1;            /* CRC passed */
} cc1101_packet_t;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

/**
 * cc1101_init - Reset and initialize the CC1101 to default configuration
 * 
 * Performs a hardware reset, verifies the chip version, and writes
 * the default configuration registers for 868 MHz GFSK operation.
 *
 * Returns: 0 on success, -1 on communication error, -2 on wrong chip ID
 */
int cc1101_init(void);

/**
 * cc1101_configure - Apply a complete configuration to the CC1101
 * @param cfg Pointer to configuration structure
 *
 * Writes all relevant registers based on the configuration.
 * Must be called after cc1101_init(). The chip is left in IDLE state.
 *
 * Returns: 0 on success, negative on error
 */
int cc1101_configure(const cc1101_config_t *cfg);

/**
 * cc1101_set_frequency - Set the operating frequency
 * @param freq_hz Frequency in Hz (300-928 MHz range)
 *
 * Calculates and writes FREQ2, FREQ1, FREQ0 registers.
 * Common frequencies: 315000000, 433920000, 868000000, 915000000
 *
 * Returns: 0 on success, -1 if frequency out of range
 */
int cc1101_set_frequency(uint32_t freq_hz);

/**
 * cc1101_set_channel - Set channel number offset
 * @param channel Channel number (0-255)
 *
 * The actual frequency = base_freq + channel * channel_step.
 * Channel step depends on MDMCFG1.CHANSPC settings.
 */
void cc1101_set_channel(uint8_t channel);

/**
 * cc1101_set_tx_power - Set transmit power
 * @param power PA power setting value
 *
 * Writes to the PATABLE register. See cc1101_power_t enum for values.
 */
void cc1101_set_tx_power(uint8_t power);

/**
 * cc1101_enter_rx - Put CC1101 into RX mode
 *
 * Issues SRX strobe and waits for GDO0 to indicate RX state.
 * Returns: 0 on success
 */
int cc1101_enter_rx(void);

/**
 * cc1101_enter_tx - Put CC1101 into TX mode
 *
 * Issues STX strobe and waits for GDO2 to indicate TX state.
 * Returns: 0 on success
 */
int cc1101_enter_tx(void);

/**
 * cc1101_enter_idle - Put CC1101 into IDLE mode
 *
 * Issues SIDLE strobe.
 */
void cc1101_enter_idle(void);

/**
 * cc1101_transmit - Transmit a packet
 * @param data Pointer to payload data
 * @param len Length of payload (1-64 bytes)
 *
 * Loads data into TX FIFO and triggers transmission.
 * Blocks until TX complete (GDO2 falls) or timeout.
 *
 * Returns: 0 on success, -1 on error, -2 on timeout
 */
int cc1101_transmit(const uint8_t *data, uint8_t len);

/**
 * cc1101_receive - Receive a packet (blocking)
 * @param pkt Pointer to packet structure to fill
 * @param timeout_ms Maximum wait time in ms (0 = no wait)
 *
 * Waits for a valid packet. If timeout_ms > 0, returns -2 on timeout.
 * Fills the packet structure with data, RSSI, and LQI.
 *
 * Returns: 0 on success, -1 on error, -2 on timeout
 */
int cc1101_receive(cc1101_packet_t *pkt, uint32_t timeout_ms);

/**
 * cc1101_receive_async - Start asynchronous RX (DMA)
 * @param buf Buffer for received data (must be 64 bytes minimum)
 *
 * Configures DMA to automatically transfer RX FIFO data to buffer.
 * GDO0 will pulse on packet received. Call cc1101_get_packet_async()
 * to retrieve the received packet.
 *
 * Returns: 0 on success
 */
int cc1101_receive_async(uint8_t *buf);

/**
 * cc1101_get_rssi - Read current RSSI value
 *
 * Returns: RSSI in dBm (approximate, ±6 dB accuracy)
 */
int8_t cc1101_get_rssi(void);

/**
 * cc1101_scan_channel - Check if channel is active (carrier sense)
 * @param channel Channel number to check
 * @param threshold_dbm RSSI threshold in dBm (e.g., -80)
 * @param dwell_ms How long to listen (ms)
 *
 * Sets CC1101 to RX on the specified channel and checks if
 * RSSI exceeds the threshold within the dwell time.
 *
 * Returns: 1 if channel is active, 0 if clear, -1 on error
 */
int cc1101_scan_channel(uint8_t channel, int8_t threshold_dbm, uint32_t dwell_ms);

/**
 * cc1101_set_sync_word - Configure sync word for packet filtering
 * @param sync_word 16-bit sync word (0x0000 = no sync, 0xD3A1 = default)
 */
void cc1101_set_sync_word(uint16_t sync_word);

/**
 * cc1101_set_address_filter - Set address filtering
 * @param addr Address byte (0 = no filtering)
 * @param broadcast Whether to accept broadcast (0x00) packets
 */
void cc1101_set_address_filter(uint8_t addr, uint8_t broadcast);

/**
 * cc1101_flush_rx - Flush RX FIFO
 */
void cc1101_flush_rx(void);

/**
 * cc1101_flush_tx - Flush TX FIFO
 */
void cc1101_flush_tx(void);

/**
 * cc1101_get_state - Read current MARCSTATE
 *
 * Returns: Current CC1101 state
 */
cc1101_state_t cc1101_get_state(void);

/**
 * cc1101_calibrate - Run frequency calibration
 *
 * Issues SCAL strobe. Must be called after frequency change.
 */
void cc1101_calibrate(void);

/* ========================================================================
 * Low-Level SPI Access (internal use)
 * ======================================================================== */

/**
 * cc1101_spi_write - Write a single register
 * @param addr Register address
 * @param value Register value
 */
void cc1101_spi_write(uint8_t addr, uint8_t value);

/**
 * cc1101_spi_read - Read a single register
 * @param addr Register address
 * Returns: Register value
 */
uint8_t cc1101_spi_read(uint8_t addr);

/**
 * cc1101_spi_write_burst - Write multiple registers (burst)
 * @param addr Starting register address
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 */
void cc1101_spi_write_burst(uint8_t addr, const uint8_t *data, uint8_t len);

/**
 * cc1101_spi_read_burst - Read multiple registers (burst)
 * @param addr Starting register address
 * @param data Pointer to data buffer
 * @param len Number of bytes to read
 */
void cc1101_spi_read_burst(uint8_t addr, uint8_t *data, uint8_t len);

/**
 * cc1101_spi_strobe - Send a command strobe
 * @param strobe Command strobe address
 */
void cc1101_spi_strobe(uint8_t strobe);

/* ========================================================================
 * DMA Transfer Functions
 * ======================================================================== */

/**
 * cc1101_dma_rx_start - Start DMA transfer from SPI1 RX
 * @param buf Destination buffer
 * @param len Number of bytes to transfer
 *
 * Configures DMA2 Stream 0 Channel 3 for SPI1 RX.
 * Returns: 0 on success
 */
int cc1101_dma_rx_start(uint8_t *buf, uint16_t len);

/**
 * cc1101_dma_tx_start - Start DMA transfer to SPI1 TX
 * @param buf Source buffer
 * @param len Number of bytes to transfer
 *
 * Configures DMA2 Stream 3 Channel 3 for SPI1 TX.
 * Returns: 0 on success
 */
int cc1101_dma_tx_start(const uint8_t *buf, uint16_t len);

/**
 * cc1101_dma_wait_rx - Wait for DMA RX transfer to complete
 * @param timeout_ms Timeout in ms
 * Returns: 0 on success, -2 on timeout
 */
int cc1101_dma_wait_rx(uint32_t timeout_ms);

/**
 * cc1101_dma_wait_tx - Wait for DMA TX transfer to complete
 * @param timeout_ms Timeout in ms
 * Returns: 0 on success, -2 on timeout
 */
int cc1101_dma_wait_tx(uint32_t timeout_ms);

#endif /* CC1101_H */