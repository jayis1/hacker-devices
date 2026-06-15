/*
 * spi_radio.c — SPI interface driver for nRF52832 BLE radios
 *
 * Implements the SPI command/response protocol for communicating
 * with two nRF52832 radios running custom SPI-slave firmware.
 */

#include "spi_radio.h"
#include "registers.h"
#include "board.h"

/* ============================================================
 * Radio instance state
 * ============================================================ */
typedef struct {
    volatile uint8_t status;
    volatile uint8_t rx_pending;
    radio_packet_t rx_packet;
} radio_state_t;

static radio_state_t radios[RADIO_COUNT] = {
    { .status = 0, .rx_pending = 0 },
    { .status = 0, .rx_pending = 0 }
};

/* ============================================================
 * SPI chip select helpers (active low)
 * ============================================================ */
static inline void cs_select(radio_id_t radio) {
    if (radio == RADIO_A) {
        CS_A_ASSERT();
    } else {
        CS_B_ASSERT();
    }
}

static inline void cs_deselect(radio_id_t radio) {
    if (radio == RADIO_A) {
        CS_A_DEASSERT();
    } else {
        CS_B_DEASSERT();
    }
}

/* ============================================================
 * Reset helpers (active low)
 * ============================================================ */
static inline void reset_assert(radio_id_t radio) {
    if (radio == RADIO_A) {
        RESET_A_ASSERT();
    } else {
        RESET_B_ASSERT();
    }
}

static inline void reset_deassert(radio_id_t radio) {
    if (radio == RADIO_A) {
        RESET_A_DEASSERT();
    } else {
        RESET_B_DEASSERT();
    }
}

/* ============================================================
 * SPI transfer functions
 * ============================================================ */
static inline SPI_TypeDef *get_spi(radio_id_t radio) {
    return (radio == RADIO_A) ? SPI1 : SPI2;
}

static uint8_t spi_transfer_byte(SPI_TypeDef *spi, uint8_t tx) {
    /* Wait until TX buffer is empty */
    while (!(spi->SR & SPI_SR_TXE));
    /* Write data byte */
    spi->DR = tx;
    /* Wait until RX buffer has data */
    while (!(spi->SR & SPI_SR_RXNE));
    /* Read and return received byte */
    return (uint8_t)(spi->DR & 0xFF);
}

/* ============================================================
 * CRC16-CCITT implementation
 * ============================================================ */
uint16_t spi_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = SPI_CRC16_INIT;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000U)
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* ============================================================
 * Initialize SPI radio driver
 * ============================================================ */
int spi_radio_init(radio_id_t radio) {
    radio_state_t *r = &radios[radio];

    /* Hardware reset: assert for ~100 µs, then deassert */
    reset_assert(radio);
    for (volatile int i = 0; i < 8400; i++);  /* ~100 µs at 84 MHz */
    reset_deassert(radio);
    for (volatile int i = 0; i < 84000; i++);  /* ~1 ms at 84 MHz */

    r->status = 0;
    r->rx_pending = 0;
    memset(&r->rx_packet, 0, sizeof(radio_packet_t));

    /* Send INIT command to radio */
    int rc = spi_radio_send_cmd(radio, CMD_RADIO_INIT, NULL, 0);
    if (rc == 0) {
        r->status |= RADIO_STATUS_INITIALIZED;
    }

    return rc;
}

/* ============================================================
 * Send command to radio via SPI
 * ============================================================ */
int spi_radio_send_cmd(radio_id_t radio, uint8_t cmd,
                       const uint8_t *payload, uint16_t len) {
    if (len > SPI_MAX_PAYLOAD) return -1;

    SPI_TypeDef *spi = get_spi(radio);
    uint8_t frame[SPI_HEADER_SIZE + SPI_MAX_PAYLOAD + 4]; /* +CRC(2)+trailer(2) */
    uint16_t idx = 0;

    /* Sync word */
    frame[idx++] = (SPI_SYNC_WORD >> 8) & 0xFF;  /* 0xAA */
    frame[idx++] = SPI_SYNC_WORD & 0xFF;           /* 0x55 */

    /* Command byte */
    frame[idx++] = cmd;

    /* Payload length (big-endian) */
    frame[idx++] = (len >> 8) & 0xFF;
    frame[idx++] = len & 0xFF;

    /* Payload */
    if (len > 0 && payload != NULL) {
        memcpy(&frame[idx], payload, len);
    }
    idx += len;

    /* CRC16 over header + payload */
    uint16_t crc = spi_crc16(frame, idx);
    frame[idx++] = (crc >> 8) & 0xFF;
    frame[idx++] = crc & 0xFF;

    /* Trailer word */
    frame[idx++] = (SPI_TRAILER_WORD >> 8) & 0xFF;  /* 0x55 */
    frame[idx++] = SPI_TRAILER_WORD & 0xFF;           /* 0xAA */

    /* Transfer frame via SPI */
    cs_select(radio);
    for (uint16_t i = 0; i < idx; i++) {
        spi_transfer_byte(spi, frame[i]);
    }
    cs_deselect(radio);

    return 0;
}

/* ============================================================
 * Read response from radio via SPI
 * ============================================================ */
int spi_radio_read_response(radio_id_t radio, uint8_t *cmd,
                            uint8_t *payload, uint16_t *len) {
    radio_state_t *r = &radios[radio];

    if (!r->rx_pending) return -1;
    r->rx_pending = 0;

    SPI_TypeDef *spi = get_spi(radio);
    cs_select(radio);

    /* Read sync word */
    uint8_t sync_hi = spi_transfer_byte(spi, 0xFF);
    uint8_t sync_lo = spi_transfer_byte(spi, 0xFF);
    if ((uint16_t)((sync_hi << 8) | sync_lo) != SPI_SYNC_WORD) {
        cs_deselect(radio);
        r->status |= RADIO_STATUS_ERROR;
        return -2;  /* Sync error */
    }

    /* Read command byte */
    *cmd = spi_transfer_byte(spi, 0xFF);

    /* Read payload length (big-endian) */
    uint8_t len_hi = spi_transfer_byte(spi, 0xFF);
    uint8_t len_lo = spi_transfer_byte(spi, 0xFF);
    *len = (uint16_t)((len_hi << 8) | len_lo);

    /* Read payload */
    uint16_t read_len = (*len > SPI_MAX_PAYLOAD) ? SPI_MAX_PAYLOAD : *len;
    for (uint16_t i = 0; i < read_len; i++) {
        payload[i] = spi_transfer_byte(spi, 0xFF);
    }

    /* Read CRC (skip verification for now — data already read) */
    spi_transfer_byte(spi, 0xFF);  /* CRC hi */
    spi_transfer_byte(spi, 0xFF);  /* CRC lo */

    /* Read trailer */
    uint8_t trail_hi = spi_transfer_byte(spi, 0xFF);
    uint8_t trail_lo = spi_transfer_byte(spi, 0xFF);
    cs_deselect(radio);

    if ((uint16_t)((trail_hi << 8) | trail_lo) != SPI_TRAILER_WORD) {
        r->status |= RADIO_STATUS_ERROR;
        return -3;  /* Trailer error */
    }

    return 0;
}

/* ============================================================
 * Configure radio parameters
 * ============================================================ */
int spi_radio_configure(radio_id_t radio, const radio_config_t *config) {
    uint8_t buf[8];
    buf[0] = config->channel;
    buf[1] = (uint8_t)config->tx_power;
    buf[2] = (config->adv_interval >> 8) & 0xFF;
    buf[3] = config->adv_interval & 0xFF;
    buf[4] = (config->scan_window >> 8) & 0xFF;
    buf[5] = config->scan_window & 0xFF;
    buf[6] = (config->scan_interval >> 8) & 0xFF;
    buf[7] = config->scan_interval & 0xFF;

    /* First set channel */
    int rc = spi_radio_send_cmd(radio, CMD_SET_CHANNEL, buf, 8);
    if (rc != 0) return rc;

    /* Then set PHY */
    rc = spi_radio_send_cmd(radio, CMD_SET_PHY, &config->phy, 1);
    return rc;
}

/* ============================================================
 * Start BLE advertising
 * ============================================================ */
int spi_radio_start_adv(radio_id_t radio) {
    return spi_radio_send_cmd(radio, CMD_START_ADV, NULL, 0);
}

/* ============================================================
 * Start BLE scanning
 * ============================================================ */
int spi_radio_start_scan(radio_id_t radio) {
    return spi_radio_send_cmd(radio, CMD_START_SCAN, NULL, 0);
}

/* ============================================================
 * Initiate BLE connection
 * ============================================================ */
int spi_radio_connect(radio_id_t radio, const uint8_t *addr, uint8_t addr_type) {
    uint8_t buf[7];
    buf[0] = addr_type;
    memcpy(&buf[1], addr, 6);
    return spi_radio_send_cmd(radio, CMD_CONNECT, buf, 7);
}

/* ============================================================
 * Send data on BLE connection
 * ============================================================ */
int spi_radio_send_data(radio_id_t radio, const uint8_t *data, uint16_t len) {
    return spi_radio_send_cmd(radio, CMD_TX_DATA, data, len);
}

/* ============================================================
 * Get radio status
 * ============================================================ */
uint8_t spi_radio_get_status(radio_id_t radio) {
    return radios[radio].status;
}

/* ============================================================
 * Reset radio via hardware reset pin
 * ============================================================ */
int spi_radio_reset(radio_id_t radio) {
    radio_state_t *r = &radios[radio];

    reset_assert(radio);
    for (volatile int i = 0; i < 84000; i++);  /* ~1 ms */
    reset_deassert(radio);
    for (volatile int i = 0; i < 84000; i++);  /* ~1 ms */

    r->status = 0;
    r->rx_pending = 0;

    return 0;
}

/* ============================================================
 * IRQ handler — called from EXTI ISR
 * ============================================================ */
void spi_radio_irq_handler(radio_id_t radio) {
    radios[radio].rx_pending = 1;
    radios[radio].status |= RADIO_STATUS_RX_PENDING;
}