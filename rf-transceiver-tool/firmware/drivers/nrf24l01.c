/*
 * nrf24l01.c — Nordic nRF24L01+ 2.4 GHz Transceiver Driver Implementation
 *
 * Production-quality SPI driver for nRF24L01+ on SPI2.
 * Supports TX, RX, channel scanning, and Enhanced ShockBurst.
 */

#include "nrf24l01.h"
#include "board.h"
#include "registers.h"

extern volatile uint32_t systick_ms;

/* ========================================================================
 * SPI Transfer Primitive
 * ======================================================================== */

static uint8_t spi2_transfer(uint8_t tx_byte)
{
    while (!(SPIx_SR(SPI2_BASE) & SPI_SR_TXE))
        ;
    SPIx_DR(SPI2_BASE) = tx_byte;
    while (!(SPIx_SR(SPI2_BASE) & SPI_SR_RXNE))
        ;
    return (uint8_t)SPIx_DR(SPI2_BASE);
}

/* ========================================================================
 * CSN Control
 * ======================================================================== */

static inline void nrf24_csn_assert(void)
{
    NRF24_CSN_LOW();
    __asm volatile("nop"); __asm volatile("nop");
}

static inline void nrf24_csn_deassert(void)
{
    __asm volatile("nop"); __asm volatile("nop");
    NRF24_CSN_HIGH();
    __asm volatile("nop"); __asm volatile("nop");
}

/* ========================================================================
 * SPI Register Access
 * ======================================================================== */

void nrf24_spi_write(uint8_t reg, uint8_t value)
{
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_W_REGISTER | reg);
    spi2_transfer(value);
    nrf24_csn_deassert();
}

uint8_t nrf24_spi_read(uint8_t reg)
{
    uint8_t value;
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_R_REGISTER | reg);
    value = spi2_transfer(0xFF);  /* Dummy byte to clock out data */
    nrf24_csn_deassert();
    return value;
}

void nrf24_spi_write_buf(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_W_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) {
        spi2_transfer(buf[i]);
    }
    nrf24_csn_deassert();
}

void nrf24_spi_read_buf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_R_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi2_transfer(0xFF);
    }
    nrf24_csn_deassert();
}

/* ========================================================================
 * Initialization
 * ======================================================================== */

int nrf24_init(void)
{
    /* CE low, CSN high */
    NRF24_CE_LOW();
    NRF24_CSN_HIGH();
    
    /* Wait for power-on reset (tpd2stby = ~5 ms from power-down) */
    uint32_t deadline = systick_ms + 100;
    while (systick_ms < deadline)
        ;
    
    /* Write CONFIG: Power down first, then configure */
    nrf24_spi_write(NRF24_CONFIG, 0x00);  /* Power down, no CRC, no RX */
    
    /* Verify SPI communication by reading CONFIG back */
    uint8_t config_read = nrf24_spi_read(NRF24_CONFIG);
    if (config_read != 0x00) {
        return -1;  /* SPI communication failure */
    }
    
    /* Set default configuration */
    nrf24_config_t default_cfg = {
        .channel = 76,
        .datarate = NRF24_DR_2M,
        .tx_power = NRF24_PWR_0DBM,
        .crc_encoding = NRF24_CRC_1BYTE,
        .addr_width = NRF24_AW_5,
        .auto_ack = 0x01,     /* Auto-ack on pipe 0 */
        .rx_pipes = 0x01,     /* Enable pipe 0 */
        .payload_width = 32,
        .auto_retr_count = 3,
        .auto_retr_delay = 500,  /* 500 µs */
    };
    
    if (nrf24_configure(&default_cfg) != 0) {
        return -1;
    }
    
    return 0;
}

/* ========================================================================
 * Configuration
 * ======================================================================== */

int nrf24_configure(const nrf24_config_t *cfg)
{
    if (cfg == NULL) return -1;
    
    /* Power down during configuration */
    NRF24_CE_LOW();
    nrf24_spi_write(NRF24_CONFIG, 0x00);
    
    /* Set channel */
    nrf24_set_channel(cfg->channel);
    
    /* Set data rate and TX power in RF_SETUP */
    uint8_t rf_setup = 0x01;  /* PLL lock always on */
    switch (cfg->datarate) {
    case NRF24_DR_250K: rf_setup |= NRF24_RF_SETUP_DR_250K; break;
    case NRF24_DR_1M:   rf_setup |= NRF24_RF_SETUP_DR_1M; break;
    case NRF24_DR_2M:   rf_setup |= NRF24_RF_SETUP_DR_2M; break;
    }
    rf_setup |= (cfg->tx_power << NRF24_RF_SETUP_PWR_SHIFT);
    nrf24_spi_write(NRF24_RF_SETUP, rf_setup);
    
    /* Set address width */
    nrf24_spi_write(NRF24_SETUP_AW, cfg->addr_width);
    
    /* Set auto-ack */
    nrf24_spi_write(NRF24_EN_AA, cfg->auto_ack);
    
    /* Set enabled RX pipes */
    nrf24_spi_write(NRF24_EN_RXADDR, cfg->rx_pipes);
    
    /* Set static payload width for each enabled pipe */
    if (cfg->payload_width > 0) {
        nrf24_spi_write(NRF24_RX_PW_P0, cfg->payload_width);
        nrf24_spi_write(NRF24_RX_PW_P1, cfg->payload_width);
        /* Pipes 2-5 use same width as pipe 1 */
    }
    
    /* Set auto-retransmit delay and count */
    uint8_t retr = ((cfg->auto_retr_delay / 250 - 1) << 4) | (cfg->auto_retr_count & 0x0F);
    nrf24_spi_write(NRF24_SETUP_RETR, retr);
    
    /* Set default addresses */
    static const uint8_t default_addr_p0[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    static const uint8_t default_addr_p1[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
    nrf24_spi_write_buf(NRF24_RX_ADDR_P0, default_addr_p0, 5);
    nrf24_spi_write_buf(NRF24_RX_ADDR_P1, default_addr_p1, 5);
    nrf24_spi_write_buf(NRF24_TX_ADDR, default_addr_p0, 5);
    
    /* Flush FIFOs */
    nrf24_flush_tx();
    nrf24_flush_rx();
    
    /* Clear interrupts */
    nrf24_clear_interrupts();
    
    return 0;
}

/* ========================================================================
 * Channel, Data Rate, Power
 * ======================================================================== */

void nrf24_set_channel(uint8_t channel)
{
    if (channel > 127) channel = 127;
    nrf24_spi_write(NRF24_RF_CH, channel & 0x7F);
}

void nrf24_set_datarate(nrf24_datarate_t dr)
{
    uint8_t rf_setup = nrf24_spi_read(NRF24_RF_SETUP);
    rf_setup &= ~(NRF24_RF_SETUP_DR_2M | NRF24_RF_SETUP_DR_250K);
    switch (dr) {
    case NRF24_DR_250K: rf_setup |= NRF24_RF_SETUP_DR_250K; break;
    case NRF24_DR_2M:   rf_setup |= NRF24_RF_SETUP_DR_2M; break;
    default: break;  /* 1 Mbps = both bits clear */
    }
    nrf24_spi_write(NRF24_RF_SETUP, rf_setup);
}

void nrf24_set_tx_power(nrf24_power_t power)
{
    uint8_t rf_setup = nrf24_spi_read(NRF24_RF_SETUP);
    rf_setup &= ~(3U << NRF24_RF_SETUP_PWR_SHIFT);
    rf_setup |= (power << NRF24_RF_SETUP_PWR_SHIFT);
    nrf24_spi_write(NRF24_RF_SETUP, rf_setup);
}

/* ========================================================================
 * Address Configuration
 * ======================================================================== */

void nrf24_set_rx_addr(nrf24_pipe_t pipe, const uint8_t *addr, uint8_t len)
{
    if (pipe <= NRF24_PIPE_1) {
        /* Pipes 0 and 1 have full 5-byte addresses */
        nrf24_spi_write_buf(NRF24_RX_ADDR_P0 + pipe, addr, len);
    } else {
        /* Pipes 2-5 share the MSBs with pipe 1, only set LSB */
        nrf24_spi_write(NRF24_RX_ADDR_P0 + pipe, addr[0]);
    }
}

void nrf24_set_tx_addr(const uint8_t *addr, uint8_t len)
{
    nrf24_spi_write_buf(NRF24_TX_ADDR, addr, len);
    /* Also set pipe 0 RX address to TX address for auto-ack */
    nrf24_spi_write_buf(NRF24_RX_ADDR_P0, addr, len);
}

void nrf24_enable_pipe(nrf24_pipe_t pipe, uint8_t auto_ack, uint8_t payload_width)
{
    uint8_t en_rx = nrf24_spi_read(NRF24_EN_RXADDR);
    en_rx |= (1U << pipe);
    nrf24_spi_write(NRF24_EN_RXADDR, en_rx);
    
    if (auto_ack) {
        uint8_t en_aa = nrf24_spi_read(NRF24_EN_AA);
        en_aa |= (1U << pipe);
        nrf24_spi_write(NRF24_EN_AA, en_aa);
    }
    
    if (payload_width > 0 && payload_width <= 32) {
        nrf24_spi_write(NRF24_RX_PW_P0 + pipe, payload_width);
    }
}

void nrf24_disable_pipe(nrf24_pipe_t pipe)
{
    uint8_t en_rx = nrf24_spi_read(NRF24_EN_RXADDR);
    en_rx &= ~(1U << pipe);
    nrf24_spi_write(NRF24_EN_RXADDR, en_rx);
}

/* ========================================================================
 * TX/RX Mode Control
 * ======================================================================== */

int nrf24_tx_mode(void)
{
    NRF24_CE_LOW();
    
    /* Power up, TX mode (PRIM_RX=0) */
    uint8_t config = nrf24_spi_read(NRF24_CONFIG);
    config |= NRF24_CONFIG_PWR_UP;
    config &= ~NRF24_CONFIG_PRIM_RX;
    nrf24_spi_write(NRF24_CONFIG, config);
    
    /* Wait for power-up (tpd2stby = 1.5 ms from power-down) */
    uint32_t deadline = systick_ms + 5;
    while (systick_ms < deadline)
        ;
    
    return 0;
}

int nrf24_rx_mode(void)
{
    NRF24_CE_LOW();
    
    /* Power up, RX mode (PRIM_RX=1) */
    uint8_t config = nrf24_spi_read(NRF24_CONFIG);
    config |= NRF24_CONFIG_PWR_UP | NRF24_CONFIG_PRIM_RX;
    nrf24_spi_write(NRF24_CONFIG, config);
    
    /* Wait for power-up */
    uint32_t deadline = systick_ms + 5;
    while (systick_ms < deadline)
        ;
    
    /* Flush RX FIFO */
    nrf24_flush_rx();
    
    /* Set CE high to start listening */
    NRF24_CE_HIGH();
    
    /* Wait for RX settling (130 µs) */
    deadline = systick_ms + 1;
    while (systick_ms < deadline)
        ;
    
    return 0;
}

void nrf24_power_down(void)
{
    NRF24_CE_LOW();
    uint8_t config = nrf24_spi_read(NRF24_CONFIG);
    config &= ~NRF24_CONFIG_PWR_UP;
    nrf24_spi_write(NRF24_CONFIG, config);
}

/* ========================================================================
 * Packet Transmission
 * ======================================================================== */

int nrf24_transmit(const uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0 || len > 32) return -1;
    
    /* Ensure we're in TX mode */
    uint8_t config = nrf24_spi_read(NRF24_CONFIG);
    if (config & NRF24_CONFIG_PRIM_RX) {
        NRF24_CE_LOW();
        config &= ~NRF24_CONFIG_PRIM_RX;
        nrf24_spi_write(NRF24_CONFIG, config);
    }
    
    /* Flush TX FIFO */
    nrf24_flush_tx();
    
    /* Clear TX flags */
    nrf24_clear_interrupts();
    
    /* Write payload to TX FIFO */
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) {
        spi2_transfer(data[i]);
    }
    nrf24_csn_deassert();
    
    /* Pulse CE for TX (minimum 10 µs) */
    NRF24_CE_HIGH();
    uint32_t deadline = systick_ms + 1;
    while (systick_ms < deadline)
        ;
    NRF24_CE_LOW();
    
    /* Wait for TX_DS or MAX_RT flag */
    deadline = systick_ms + 50;  /* 50 ms timeout */
    while (systick_ms < deadline) {
        uint8_t status = nrf24_get_status();
        if (status & NRF24_STATUS_TX_DS) {
            /* Packet sent and ACK received */
            nrf24_clear_interrupts();
            return 0;
        }
        if (status & NRF24_STATUS_MAX_RT) {
            /* Max retransmit reached */
            nrf24_flush_tx();
            nrf24_clear_interrupts();
            return -1;
        }
    }
    
    return -2;  /* Timeout */
}

/* ========================================================================
 * Packet Reception
 * ======================================================================== */

int nrf24_receive(nrf24_packet_t *pkt)
{
    if (pkt == NULL) return -1;
    
    /* Check if RX_DR flag is set (packet in FIFO) */
    uint8_t status = nrf24_get_status();
    if (!(status & NRF24_STATUS_RX_DR) && 
        (nrf24_spi_read(NRF24_FIFO_STATUS) & 0x01)) {
        /* No packet available */
        return 0;
    }
    
    /* Read payload from RX FIFO */
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < 32; i++) {
        pkt->data[i] = spi2_transfer(0xFF);
    }
    nrf24_csn_deassert();
    
    /* Determine pipe number from STATUS */
    pkt->pipe = (status >> 1) & 0x07;
    pkt->length = nrf24_spi_read(NRF24_RX_PW_P0 + pkt->pipe);
    if (pkt->length > 32) pkt->length = 32;
    
    /* Read RPD (Received Power Detector) */
    uint8_t rpd = nrf24_spi_read(NRF24_RPD);
    pkt->rssi = rpd ? -64 : -100;  /* RPD > -64 dBm → approx -64, else < -64 */
    
    /* Clear RX_DR interrupt */
    nrf24_clear_interrupts();
    
    return 1;  /* Packet available */
}

/* ========================================================================
 * Channel Scanning
 * ======================================================================== */

int nrf24_scan_channel(uint8_t channel, uint32_t dwell_us)
{
    /* Set channel */
    nrf24_set_channel(channel);
    
    /* Enter RX mode */
    uint8_t config = nrf24_spi_read(NRF24_CONFIG);
    config |= NRF24_CONFIG_PWR_UP | NRF24_CONFIG_PRIM_RX;
    nrf24_spi_write(NRF24_CONFIG, config);
    NRF24_CE_HIGH();
    
    /* Wait at least 130 µs for RX settling, then additional dwell */
    uint32_t total_us = (dwell_us > 128) ? dwell_us : 128;
    volatile uint32_t count = total_us * 21;  /* Approximate µs delay at 84 MHz */
    while (count--) {
        __asm volatile("nop");
    }
    
    /* Read RPD register */
    uint8_t rpd = nrf24_spi_read(NRF24_RPD);
    
    /* Go back to idle */
    NRF24_CE_LOW();
    
    return (rpd & 0x01) ? 1 : 0;
}

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

void nrf24_flush_tx(void)
{
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_FLUSH_TX);
    nrf24_csn_deassert();
}

void nrf24_flush_rx(void)
{
    nrf24_csn_assert();
    spi2_transfer(NRF24_CMD_FLUSH_RX);
    nrf24_csn_deassert();
}

uint8_t nrf24_get_status(void)
{
    nrf24_csn_assert();
    uint8_t status = spi2_transfer(NRF24_CMD_NOP);
    nrf24_csn_deassert();
    return status;
}

void nrf24_clear_interrupts(void)
{
    uint8_t status = nrf24_get_status();
    nrf24_spi_write(NRF24_STATUS, status | NRF24_STATUS_RX_DR 
                                     | NRF24_STATUS_TX_DS 
                                     | NRF24_STATUS_MAX_RT);
}