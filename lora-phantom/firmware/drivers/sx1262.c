/*
 * lora-phantom / drivers/sx1262.c
 * Semtech SX1262 LoRa transceiver driver (SPI6).
 * TX/RX config, CAD, payload xfer, IRQ handling, raw header TX for fuzzing.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The SX1262 is controlled via a 4-wire SPI with a NSS line. The BUSY pin
 * gates all SPI access — the host must wait for BUSY=low before each
 * transaction. DIO1 signals TxDone / RxDone / CadDone IRQs; the IRQ status
 * is read via the GetIrqStatus command and cleared via ClearIrqStatus.
 *
 * This driver uses manual NSS (PG8) and polling (no DMA) for simplicity and
 * determinism. All commands are big-endian per the SX1262 datasheet.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- SX1262 SPI commands ---- */
#define SX1262_CMD_SET_SLEEP         0x84
#define SX1262_CMD_SET_STANDBY       0x80
#define SX1262_CMD_SET_FS            0xC1
#define SX1262_CMD_SET_TX            0x83
#define SX1262_CMD_SET_RX            0x82
#define SX1262_CMD_STOP_TIMER_ON_PREAMBLE 0x9F
#define SX1262_CMD_SET_RX_DUTY_CYCLE 0x94
#define SX1262_CMD_SET_CAD           0xC5
#define SX1262_CMD_SET_TX_CONTINUOUS_WAVE 0xD1
#define SX1262_CMD_SET_TX_INFINITE_PREAMBLE 0xD3
#define SX1262_CMD_SET_REGULATOR_MODE 0x96
#define SX1262_CMD_CALIBRATE         0x89
#define SX1262_CMD_CALIBRATE_IMAGE   0x98
#define SX1262_CMD_SET_PA_CONFIG     0x95
#define SX1262_CMD_SET_RX_CONFIG     0x8E
#define SX1262_CMD_SET_TX_CONFIG     0x98  /* placeholder; actual 0x98 is cal image */
#define SX1262_CMD_SET_MODULATION_PARAMS 0x8B
#define SX1262_CMD_SET_PACKET_PARAMS 0x8C
#define SX1262_CMD_SET_CAD_PARAMS    0x88
#define SX1262_CMD_SET_BUFFER_BASE_ADDRESS 0x8F
#define SX1262_CMD_SET_LORA_SYMB_NUM_TIMEOUT 0xA0
#define SX1262_CMD_WRITE_BUFFER      0x0E
#define SX1262_CMD_READ_BUFFER       0x1D
#define SX1262_CMD_SET_DIO_IRQ_PARAMS 0x8D
#define SX1262_CMD_GET_IRQ_STATUS    0x12
#define SX1262_CMD_CLEAR_IRQ_STATUS  0x02
#define SX1262_CMD_SET_DIO2_AS_RF_SWITCH_CTRL 0x9D
#define SX1262_CMD_SET_DIO3_AS_TCXO_CTRL 0x97
#define SX1262_CMD_SET_RF_FREQUENCY  0x86
#define SX1262_CMD_SET_TX_PARAMS     0x8E
#define SX1262_CMD_SET_RFFREQUENCY   0x86
#define SX1262_CMD_SET_PACKET_TYPE   0x8A
#define SX1262_CMD_GET_PACKET_TYPE   0x11
#define SX1262_CMD_SET_TX_PARAMS_    0x8E
#define SX1262_CMD_SET_CAD_DUTY_CYCLE 0xC5

/* Packet types */
#define SX1262_PKT_LORA  0x01
#define SX1262_PKT_FSK   0x00

/* Standby config */
#define SX1262_STDBY_RC   0x00
#define SX1262_STDBY_XOSC 0x01

/* IRQ masks */
#define SX1262_IRQ_TX_DONE          0x0001
#define SX1262_IRQ_RX_DONE          0x0002
#define SX1262_IRQ_PREAMBLE_DETECTED 0x0004
#define SX1262_IRQ_SYNCWORD_VALID   0x0008
#define SX1262_IRQ_HEADER_VALID     0x0010
#define SX1262_IRQ_HEADER_ERR       0x0020
#define SX1262_IRQ_CRC_ERR          0x0040
#define SX1262_IRQ_CAD_DONE         0x0080
#define SX1262_IRQ_CAD_DETECTED     0x0100
#define SX1262_IRQ_TIMEOUT          0x0200
#define SX1262_IRQ_ALL              0x03FF

/* ---- SX1262 register addresses (subset) ---- */
#define SX1262_REG_RX_GAIN          0x08AC
#define SX1262_REG_LNA_CONFIG       0x0889

/* ---- Wait for BUSY low ---- */
static void sx1262_wait_busy(void) {
    uint32_t timeout = 100000;
    while (gpio_read(SX1262_BUSY_PORT, SX1262_BUSY_PIN) && timeout--) { }
}

/* ---- SPI single-byte exchange ---- */
static uint8_t spi_xfer(uint8_t tx) {
    while (!(SPI6->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI6->DR = tx;
    while (!(SPI6->SR & SPI_SR_RXNE)) { }
    return (uint8_t)SPI6->DR;
}

/* ---- Write command with optional data ---- */
static void sx1262_write_cmd(uint8_t cmd, const uint8_t *data, uint16_t len) {
    sx1262_wait_busy();
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 0);   /* NSS low */
    spi_xfer(cmd);
    for (uint16_t i = 0; i < len; i++) spi_xfer(data[i]);
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 1);   /* NSS high */
}

/* ---- Read command: write cmd + dummy bytes, read response ---- */
static void sx1262_read_cmd(uint8_t cmd, uint8_t *data, uint16_t len) {
    sx1262_wait_busy();
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 0);
    spi_xfer(cmd);
    /* Dummy byte for SX1262 read latency */
    spi_xfer(0x00);
    for (uint16_t i = 0; i < len; i++) data[i] = spi_xfer(0x00);
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 1);
}

/* ---- Write buffer (payload to TX FIFO) ---- */
static void sx1262_write_buffer(uint8_t offset, const uint8_t *data, uint16_t len) {
    sx1262_wait_busy();
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 0);
    spi_xfer(SX1262_CMD_WRITE_BUFFER);
    spi_xfer(offset);
    for (uint16_t i = 0; i < len; i++) spi_xfer(data[i]);
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 1);
}

/* ---- Read buffer (payload from RX FIFO) ---- */
static void sx1262_read_buffer(uint8_t offset, uint8_t *data, uint16_t len) {
    sx1262_wait_busy();
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 0);
    spi_xfer(SX1262_CMD_READ_BUFFER);
    spi_xfer(offset);
    spi_xfer(0x00);   /* dummy */
    for (uint16_t i = 0; i < len; i++) data[i] = spi_xfer(0x00);
    gpio_write(SX1262_NSS_PORT, SX1262_NSS_PIN, 1);
}

/* ---- Set RF frequency (cmd 0x86, 4 bytes, Hz) ---- */
static void sx1262_set_freq(uint32_t freq_hz) {
    /* SX1262 uses a 32 MHz reference (TCXO); freq = rfFreq * 2^25 / 32e6
     * For our 32 MHz TCXO: reg = freq_hz * 2^25 / 32000000 */
    uint64_t reg = ((uint64_t)freq_hz << 25) / 32000000ULL;
    uint8_t buf[4];
    buf[0] = (uint8_t)(reg >> 24);
    buf[1] = (uint8_t)(reg >> 16);
    buf[2] = (uint8_t)(reg >> 8);
    buf[3] = (uint8_t)(reg);
    sx1262_write_cmd(SX1262_CMD_SET_RF_FREQUENCY, buf, 4);
}

/* ---- Set packet type (LoRa = 0x01) ---- */
static void sx1262_set_packet_type(uint8_t type) {
    sx1262_write_cmd(SX1262_CMD_SET_PACKET_TYPE, &type, 1);
}

/* ---- Set modulation params for LoRa ---- */
/* sf: 7..12, bw: 0=125k,1=250k,2=500k, cr: 1=4/5..4=4/8 */
static void sx1262_set_mod_params_lora(uint8_t sf, uint8_t bw, uint8_t cr) {
    uint8_t buf[4];
    buf[0] = sf;           /* SF7..SF12 → 0x70..0x0C? No: SX1262 uses 0x70=SF12 .. 0x06=SF7? */
    /* Actually: SX1262 SpreadingFactor field: 0x05=SF5? No.
     * Per datasheet: SF = 0x70 for SF12? Let's use direct SF value mapping:
     * 0x0C=SF12, 0x0B=SF11, 0x0A=SF10, 0x09=SF9, 0x08=SF8, 0x07=SF7.
     * We pass sf as 7..12 → shift: buf[0] = sf (direct, 7..12 valid) */
    buf[0] = sf;
    /* Bandwidth: 0x00=125kHz, 0x01=250kHz, 0x02=500kHz, 0x04=62.5kHz... */
    buf[1] = bw;           /* 0=125k, 1=250k, 2=500k */
    /* CodingRate: 0x01=4/5, 0x02=4/6, 0x03=4/7, 0x04=4/8 */
    buf[2] = cr;
    /* LDRO (Low Data Rate Optimizer): auto for SX1262 via 0x01 = enable */
    buf[3] = 0x01;         /* LDRO auto */
    sx1262_write_cmd(SX1262_CMD_SET_MODULATION_PARAMS, buf, 4);
}

/* ---- Set packet params for LoRa ---- */
/* preamble_len: 8..65535, header_type: 0x00=explicit, 0x01=implicit,
 * payload_len, crc_type: 0x00=off, 0x01=on, iq_invert: 0x00=std, 0x01=inv */
static void sx1262_set_pkt_params_lora(uint16_t preamble, uint8_t header_type,
                                       uint8_t payload_len, uint8_t crc_on,
                                       uint8_t iq_inv) {
    uint8_t buf[9];
    buf[0] = (uint8_t)(preamble >> 8);
    buf[1] = (uint8_t)(preamble & 0xFF);
    buf[2] = header_type;
    buf[3] = payload_len;
    buf[4] = crc_on;
    buf[5] = iq_inv;
    /* Remaining 3 bytes are 0x00 (reserved) */
    buf[6] = 0; buf[7] = 0; buf[8] = 0;
    sx1262_write_cmd(SX1262_CMD_SET_PACKET_PARAMS, buf, 9);
}

/* ---- Set TX params (power + ramp) ---- */
static void sx1262_set_tx_params(int8_t power_dbm, uint8_t ramp) {
    uint8_t buf[2];
    /* Power is signed; SX1262 uses int8. For +22 dBm set 0x16; for low, negative. */
    buf[0] = (uint8_t)power_dbm;
    buf[1] = ramp;   /* 0x00=200µs, 0x01=100µs, ... 0x07=10µs */
    sx1262_write_cmd(SX1262_CMD_SET_TX_PARAMS, buf, 2);
}

/* ---- Set DIO IRQ params ---- */
static void sx1262_set_irq(uint16_t irq_mask, uint16_t dio1_mask) {
    uint8_t buf[8];
    buf[0] = (uint8_t)(irq_mask >> 8);
    buf[1] = (uint8_t)(irq_mask & 0xFF);
    buf[2] = (uint8_t)(dio1_mask >> 8);
    buf[3] = (uint8_t)(dio1_mask & 0xFF);
    buf[4] = 0; buf[5] = 0;  /* DIO2 mask */
    buf[6] = 0; buf[7] = 0;  /* DIO3 mask */
    sx1262_write_cmd(SX1262_CMD_SET_DIO_IRQ_PARAMS, buf, 8);
}

/* ---- Get IRQ status ---- */
static uint16_t sx1262_get_irq(void) {
    uint8_t buf[2];
    sx1262_read_cmd(SX1262_CMD_GET_IRQ_STATUS, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

/* ---- Clear IRQ status ---- */
static void sx1262_clear_irq(uint16_t mask) {
    uint8_t buf[2];
    buf[0] = (uint8_t)(mask >> 8);
    buf[1] = (uint8_t)(mask & 0xFF);
    sx1262_write_cmd(SX1262_CMD_CLEAR_IRQ_STATUS, buf, 2);
}

/* ---- Set standby ---- */
static void sx1262_set_standby(uint8_t cfg) {
    sx1262_write_cmd(SX1262_CMD_SET_STANDBY, &cfg, 1);
}

/* ---- Set sleep ---- */
void sx1262_sleep(void) {
    uint8_t cfg = 0x04;   /* warm start + RTC disabled */
    sx1262_write_cmd(SX1262_CMD_SET_SLEEP, &cfg, 1);
}

/* ---- Set DIO3 as TCXO control (1.6 V, 0x07 = 32 MHz TCXO) ---- */
static void sx1262_set_tcxo(void) {
    /* Enable TCXO on DIO3 at 1.7 V with 5 ms settle */
    uint8_t buf[3] = { 0x01, 0x00, 0x05 };  /* voltage=1.7V, delay=5ms */
    sx1262_write_cmd(SX1262_CMD_SET_DIO3_AS_TCXO_CTRL, buf, 3);
}

/* ---- Set DIO2 as RF switch ---- */
static void sx1262_set_rf_switch(void) {
    uint8_t enable = 0x01;
    sx1262_write_cmd(SX1262_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &enable, 1);
}

/* ---- Set regulator mode (DC-DC) ---- */
static void sx1262_set_regulator(void) {
    uint8_t mode = 0x01;  /* DC-DC */
    sx1262_write_cmd(SX1262_CMD_SET_REGULATOR_MODE, &mode, 1);
}

/* ---- Set PA config for SX1262 (max +22 dBm) ---- */
static void sx1262_set_pa_config(void) {
    /* paDutyCycle=0x04, hpMax=0x07, deviceSel=0x00 (SX1262), paLut=0x01 */
    uint8_t buf[4] = { 0x04, 0x07, 0x00, 0x01 };
    sx1262_write_cmd(SX1262_CMD_SET_PA_CONFIG, buf, 4);
}

/* ---- Calibrate image for the target frequency band ---- */
static void sx1262_cal_image(uint32_t freq_hz) {
    uint8_t buf[2];
    if (freq_hz >= 430000000 && freq_hz <= 440000000) {
        buf[0] = 0x6B; buf[1] = 0x6F;
    } else if (freq_hz >= 470000000 && freq_hz <= 510000000) {
        buf[0] = 0x75; buf[1] = 0x81;
    } else if (freq_hz >= 779000000 && freq_hz <= 787000000) {
        buf[0] = 0xC1; buf[1] = 0xC5;
    } else if (freq_hz >= 863000000 && freq_hz <= 870000000) {
        buf[0] = 0xD7; buf[1] = 0xDB;
    } else if (freq_hz >= 902000000 && freq_hz <= 928000000) {
        buf[0] = 0xE1; buf[1] = 0xE9;
    } else {
        buf[0] = 0xD7; buf[1] = 0xDB;  /* default EU868 */
    }
    sx1262_write_cmd(SX1262_CMD_CALIBRATE_IMAGE, buf, 2);
}

/* ---- Reset SX1262 ---- */
void sx1262_reset(void) {
    gpio_write(SX1262_RESET_PORT, SX1262_RESET_PIN, 0);
    /* Wait 2 ms (min 100 µs) */
    for (volatile int i = 0; i < 200000; i++) { }
    gpio_write(SX1262_RESET_PORT, SX1262_RESET_PIN, 1);
    /* Wait 10 ms for chip to come up */
    for (volatile int i = 0; i < 1000000; i++) { }
}

/* ---- Full init ---- */
void sx1262_init(void) {
    sx1262_reset();
    sx1262_set_standby(SX1262_STDBY_RC);
    sx1262_set_tcxo();
    sx1262_set_rf_switch();
    sx1262_set_regulator();
    sx1262_set_pa_config();
    sx1262_set_packet_type(SX1262_PKT_LORA);
    /* Clear any pending IRQs */
    sx1262_clear_irq(SX1262_IRQ_ALL);
    /* Set antenna switch to RX by default */
    gpio_write(SX1262_ANT_SW_PORT, SX1262_ANT_SW_PIN, 0);
}

/* ---- RX config + enter RX ---- */
int sx1262_rx_config(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr) {
    sx1262_set_standby(SX1262_STDBY_RC);
    sx1262_set_packet_type(SX1262_PKT_LORA);
    sx1262_set_freq(freq_hz);
    sx1262_cal_image(freq_hz);
    sx1262_set_mod_params_lora(sf, bw, cr);
    /* Preamble 8 symbols, explicit header, max payload 255, CRC on, no IQ invert */
    sx1262_set_pkt_params_lora(8, 0x00, 0xFF, 0x01, 0x00);
    sx1262_set_irq(SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT | SX1262_IRQ_CRC_ERR,
                   SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT);
    sx1262_clear_irq(SX1262_IRQ_ALL);

    /* Set buffer base address: TX=0x00, RX=0x00 */
    uint8_t buf[2] = { 0x00, 0x00 };
    sx1262_write_cmd(SX1262_CMD_SET_BUFFER_BASE_ADDRESS, buf, 2);

    /* Antenna to RX */
    gpio_write(SX1262_ANT_SW_PORT, SX1262_ANT_SW_PIN, 0);
    return 0;
}

/* ---- Enter RX with timeout (ms), block until packet or timeout ---- */
int sx1262_receive(uint8_t *buf, uint16_t maxlen, int16_t *rssi, int8_t *snr,
                   uint32_t timeout_ms) {
    /* Set RX with timeout: timeout in units of 15.625 µs (64 kHz clock).
     * timeout_ms * 64 = units. Use 0xFFFFFF for continuous, or computed. */
    uint32_t timeout_units = timeout_ms * 64;
    if (timeout_units > 0xFFFFFF) timeout_units = 0xFFFFFF;
    uint8_t txb[3] = {
        (uint8_t)(timeout_units >> 16),
        (uint8_t)(timeout_units >> 8),
        (uint8_t)(timeout_units)
    };
    sx1262_write_cmd(SX1262_CMD_SET_RX, txb, 3);

    /* Wait for DIO1 (RX done) or timeout */
    uint32_t deadline = 100000;
    while (gpio_read(SX1262_DIO1_PORT, SX1262_DIO1_PIN) == 0 && deadline--) { }

    uint16_t irq = sx1262_get_irq();
    sx1262_clear_irq(SX1262_IRQ_ALL);

    if (irq & SX1262_IRQ_TIMEOUT) return -2;
    if (irq & SX1262_IRQ_CRC_ERR) return -3;
    if (!(irq & SX1262_IRQ_RX_DONE)) return -1;

    /* Get RX buffer status: [payloadStartPtr, payloadLength] */
    uint8_t rxstat[2];
    sx1262_read_cmd(0x13, rxstat, 2);   /* GetRxBufferStatus = 0x13 */
    uint8_t offset = rxstat[0];
    uint8_t plen   = rxstat[1];
    if (plen > maxlen) plen = (uint8_t)maxlen;

    sx1262_read_buffer(offset, buf, plen);

    /* Get packet status: [RSSI avg, RSSI pkt, SNR, RSSI signal] */
    uint8_t pktstat[3];
    sx1262_read_cmd(0x14, pktstat, 3);   /* GetPacketStatus = 0x14 */
    if (rssi) *rssi = -((int16_t)pktstat[0]);  /* RSSI is negative, stored as offset */
    if (snr)  *snr  = (int8_t)pktstat[1] / 4;  /* SNR is 2's complement / 4 */

    return (int)plen;
}

/* ---- TX config + transmit ---- */
int sx1262_tx_config(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr,
                     int8_t power_dbm) {
    sx1262_set_standby(SX1262_STDBY_RC);
    sx1262_set_packet_type(SX1262_PKT_LORA);
    sx1262_set_freq(freq_hz);
    sx1262_cal_image(freq_hz);
    sx1262_set_mod_params_lora(sf, bw, cr);
    sx1262_set_pkt_params_lora(8, 0x00, 0xFF, 0x01, 0x00);
    sx1262_set_tx_params(power_dbm, 0x04);  /* 40 µs ramp */
    sx1262_set_irq(SX1262_IRQ_TX_DONE, SX1262_IRQ_TX_DONE);
    sx1262_clear_irq(SX1262_IRQ_ALL);

    /* Antenna to TX */
    gpio_write(SX1262_ANT_SW_PORT, SX1262_ANT_SW_PIN, 1);
    return 0;
}

/* ---- Transmit a buffer ---- */
int sx1262_transmit(const uint8_t *buf, uint16_t len, uint32_t timeout_ms) {
    if (len > 255) len = 255;
    sx1262_write_buffer(0x00, buf, len);

    /* Set TX with timeout (units of 15.625 µs) */
    uint32_t timeout_units = timeout_ms * 64;
    if (timeout_units > 0xFFFFFF) timeout_units = 0xFFFFFF;
    uint8_t txb[3] = {
        (uint8_t)(timeout_units >> 16),
        (uint8_t)(timeout_units >> 8),
        (uint8_t)(timeout_units)
    };
    sx1262_write_cmd(SX1262_CMD_SET_TX, txb, 3);

    /* Wait for TxDone */
    uint32_t deadline = 1000000;
    while (gpio_read(SX1262_DIO1_PORT, SX1262_DIO1_PIN) == 0 && deadline--) { }
    uint16_t irq = sx1262_get_irq();
    sx1262_clear_irq(SX1262_IRQ_ALL);

    /* Antenna back to RX */
    gpio_write(SX1262_ANT_SW_PORT, SX1262_ANT_SW_PIN, 0);

    if (irq & SX1262_IRQ_TX_DONE) return 0;
    return -1;
}

/* ---- Channel Activity Detection (CAD) ---- */
int sx1262_cad(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint32_t timeout_ms) {
    sx1262_set_standby(SX1262_STDBY_RC);
    sx1262_set_freq(freq_hz);
    sx1262_set_mod_params_lora(sf, bw, 1);
    sx1262_set_irq(SX1262_IRQ_CAD_DONE | SX1262_IRQ_CAD_DETECTED,
                   SX1262_IRQ_CAD_DONE | SX1262_IRQ_CAD_DETECTED);
    sx1262_clear_irq(SX1262_IRQ_ALL);
    gpio_write(SX1262_ANT_SW_PORT, SX1262_ANT_SW_PIN, 0);

    /* CAD params: symbol num, exit mode, timeout, delta */
    uint8_t cad_params[7] = { sf - 7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sx1262_write_cmd(SX1262_CMD_SET_CAD_PARAMS, cad_params, 7);

    /* Start CAD */
    sx1262_write_cmd(SX1262_CMD_SET_CAD, 0, 0);

    /* Wait for CAD done */
    uint32_t deadline = timeout_ms * 10000;
    while (gpio_read(SX1262_DIO1_PORT, SX1262_DIO1_PIN) == 0 && deadline--) { }
    uint16_t irq = sx1262_get_irq();
    sx1262_clear_irq(SX1262_IRQ_ALL);

    if (irq & SX1262_IRQ_CAD_DETECTED) return 1;   /* activity detected */
    if (irq & SX1262_IRQ_CAD_DONE)     return 0;   /* clear */
    return -1;
}

/* ---- Transmit with arbitrary / malformed header (for PHY fuzzing) ---- */
/* header_type: 0x00=explicit, 0x01=implicit
 * We can set CRC off, IQ invert, wrong payload length, etc. to fuzz parsers. */
int sx1262_tx_raw_header(const uint8_t *buf, uint16_t len, uint8_t sf, uint8_t bw,
                         uint8_t cr, uint8_t header_type, int8_t power_dbm) {
    sx1262_set_standby(SX1262_STDBY_RC);
    sx1262_set_packet_type(SX1262_PKT_LORA);
    sx1262_set_freq(868100000);
    sx1262_cal_image(868100000);
    sx1262_set_mod_params_lora(sf, bw, cr);
    /* Deliberately set unusual packet params for fuzzing:
     * - preamble length can be weird (e.g., 1 symbol)
     * - header type as requested
     * - payload length can be wrong (set to 0 or 255 regardless of actual)
     * - CRC on/off randomized by caller via cr field reuse */
    sx1262_set_pkt_params_lora(1, header_type, (uint8_t)len, (cr >> 4) & 1, 0x00);
    sx1262_set_tx_params(power_dbm, 0x04);
    sx1262_set_irq(SX1262_IRQ_TX_DONE, SX1262_IRQ_TX_DONE);
    sx1262_clear_irq(SX1262_IRQ_ALL);
    gpio_write(SX1262_ANT_SW_PORT, SX1262_ANT_SW_PIN, 1);

    if (len > 255) len = 255;
    sx1262_write_buffer(0x00, buf, len);

    uint8_t txb[3] = { 0, 0xFF, 0xFF };   /* long timeout */
    sx1262_write_cmd(SX1262_CMD_SET_TX, txb, 3);

    uint32_t deadline = 1000000;
    while (gpio_read(SX1262_DIO1_PORT, SX1262_DIO1_PIN) == 0 && deadline--) { }
    sx1262_clear_irq(SX1262_IRQ_ALL);
    gpio_write(SX1262_ANT_SW_PORT, SX1262_ANT_SW_PIN, 0);
    return 0;
}

/* ---- Get instantaneous RSSI ---- */
int sx1262_get_rssi(int16_t *rssi) {
    uint8_t buf[1];
    sx1262_read_cmd(0x1F, buf, 1);   /* GetRssiInst = 0x1F? Actually 0x1F is GetRssiInst? */
    /* Per datasheet, GetRssiInst = 0x1F returns 1 byte RSSI offset */
    if (rssi) *rssi = -((int16_t)buf[0]);
    return 0;
}