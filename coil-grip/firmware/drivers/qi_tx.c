/*
 * qi_tx.c — Qi wireless-power transmitter driver (downstream side of CoilGrip)
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * CoilGrip presents a Qi 1.1 Basic Power Profile transmitter to the
 * downstream target device (the thing being charged). The MWCT1011
 * handles the certified TX state machine; this driver configures it over
 * SPI2 and exposes high-level controls (start/stop, power percentage,
 * FOD override, and ASK packet injection toward the receiver).
 *
 * The driver also implements a small Qi packet encoder for the PT→PR
 * direction (amplitude-shift keying of the carrier by gating the
 * H-bridge enable pin PA1).
 */

#include "board.h"
#include "registers.h"

/* MWCT1011 SPI command opcodes (simplified subset for CoilGrip use) */
#define MWCT_CMD_READ_REG       0x00
#define MWCT_CMD_WRITE_REG      0x01
#define MWCT_CMD_START_TX       0x10
#define MWCT_CMD_STOP_TX        0x11
#define MWCT_CMD_SET_POWER      0x12
#define MWCT_CMD_FOD_OVERRIDE   0x20
#define MWCT_CMD_GET_STATE      0x30

/* MWCT1011 register map (relevant subset) */
#define MWCT_REG_STATE          0x0000  /* TX state machine state        */
#define MWCT_REG_VOLTAGE        0x0002  /* rectified voltage (raw)        */
#define MWCT_REG_CURRENT         0x0004  /* coil current (raw)             */
#define MWCT_REG_POWER          0x0006  /* delivered power (mW)           */
#define MWCT_REG_TEMP            0x0008  /* internal temp (°C)             */
#define MWCT_REG_FOD_OFFSET      0x0010  /* FOD calibration offset         */

/* Qi TX states */
enum {
    MWCT_STATE_IDLE = 0,
    MWCT_STATE_PING,
    MWCT_STATE_POWER_TRANSFER,
    MWCT_STATE_ERROR,
    MWCT_STATE_FOD_FAULT
};

static bool g_tx_running = false;
static uint32_t g_tx_freq_hz = 140000;
static uint8_t g_tx_duty_pct = 50;
static int32_t g_fod_offset_mw = 0;

/* ---- low-level SPI2 to MWCT1011 ---------------------------------------- */

static void spi2_cs_low(void)  { GPIOB->BSRR = BIT(PIN_PB12 + 16); }
static void spi2_cs_high(void) { GPIOB->BSRR = BIT(PIN_PB12); }

static uint8_t spi2_xfer(uint8_t tx)
{
    /* 8-bit master, mode 0 (CPOL=0, CPHA=0). */
    SPI2->CR1 = 0;
    SPI2->CFG1 = SPI_CFG1_DSIZE_8BIT;
    SPI2->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSOE;
    SPI2->CR1 |= SPI_CR1_SPE;

    /* Wait TXP, write, wait EOT, read. */
    while (!(SPI2->SR & SPI_SR_TXP)) { }
    SPI2->TXDR = tx;
    SPI2->CR1 |= SPI_CR1_CSTART;
    while (!(SPI2->SR & SPI_SR_RXP)) { }
    uint8_t rx = (uint8_t)SPI2->RXDR;
    while (!(SPI2->SR & SPI_SR_EOT)) { }
    SPI2->IFCR = SPI_IFCR_EOTC;
    SPI2->CR1 &= ~SPI_CR1_SPE;
    return rx;
}

static uint8_t mwct_read_reg(uint16_t reg)
{
    uint8_t v;
    spi2_cs_low();
    spi2_xfer(MWCT_CMD_READ_REG);
    spi2_xfer((uint8_t)(reg >> 8));
    spi2_xfer((uint8_t)(reg & 0xFF));
    v = spi2_xfer(0x00);  /* dummy read */
    spi2_cs_high();
    return v;
}

static void mwct_write_reg(uint16_t reg, uint8_t val)
{
    spi2_cs_low();
    spi2_xfer(MWCT_CMD_WRITE_REG);
    spi2_xfer((uint8_t)(reg >> 8));
    spi2_xfer((uint8_t)(reg & 0xFF));
    spi2_xfer(val);
    spi2_cs_high();
}

static void mwct_cmd(uint8_t cmd, uint8_t arg)
{
    spi2_cs_low();
    spi2_xfer(cmd);
    spi2_xfer(arg);
    spi2_cs_high();
}

/* ---- Qi packet encoder (PT→PR via ASK on carrier) ----------------------
 * The Qi 1.1 PT→PR packet format is:
 *   [header(11 bits) | data(1..32 bytes) | CRC-8]
 * The header carries: 2 bits type + 3 bits length-1 + 6 bits (reserved/CRC).
 * We use the simplified 1-byte header + data + CRC-8 form that the MWCT
 * accepts for injection. */

static uint8_t qi_crc8(const uint8_t *data, uint8_t len)
{
    /* Qi uses polynomial 0x07, init 0x00, MSB-first. */
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
    return crc;
}

int qi_tx_send_packet(const uint8_t *pkt, uint8_t len)
{
    /* The MWCT1011 has a TX packet FIFO at reg 0x0020..0x002F (16 bytes).
     * We write header(1) + data + CRC8 and then issue a "send packet" command.
     * For longer packets the MWCT auto-segments; we cap at 16 bytes total. */
    if (len == 0 || len > 14) return -1;

    uint8_t frame[16];
    frame[0] = (uint8_t)(len);  /* header = length */
    for (uint8_t i = 0; i < len; i++)
        frame[1 + i] = pkt[i];
    frame[1 + len] = qi_crc8(pkt, len);

    spi2_cs_low();
    spi2_xfer(MWCT_CMD_WRITE_REG);
    spi2_xfer(0x00);
    spi2_xfer(0x20);  /* TX packet FIFO base */
    for (uint8_t i = 0; i < (len + 2); i++)
        spi2_xfer(frame[i]);
    spi2_cs_high();

    mwct_cmd(0x13, len + 2);  /* send-packet command, arg = frame length */
    return 0;
}

/* ---- Public API -------------------------------------------------------- */

int qi_tx_start(uint32_t freq_hz, uint8_t duty_pct)
{
    if (freq_hz < 110000 || freq_hz > 205000)
        return -1;
    if (duty_pct < 10 || duty_pct > 90)
        return -1;

    g_tx_freq_hz = freq_hz;
    g_tx_duty_pct = duty_pct;

    /* Update HRTIM period/compare for the requested frequency.
     * HRTIM effective clock = HRTIM_CLK_HZ * 8. */
    uint32_t per = (HRTIM_CLK_HZ * 8U) / freq_hz;
    HRTIM1->PERER = per;
    HRTIM1->CMP1ER = (per * duty_pct) / 100U;

    /* Tell MWCT1011 to begin the ping → digital ping → power transfer state
     * machine. It will energize the coil and negotiate with the downstream
     * receiver. */
    mwct_cmd(MWCT_CMD_START_TX, 0x01);
    g_tx_running = true;
    return 0;
}

void qi_tx_stop(void)
{
    mwct_cmd(MWCT_CMD_STOP_TX, 0x00);
    /* Gate the H-bridge enable (PA1) low to guarantee the coil is de-energized. */
    GPIOA->BSRR = BIT(PIN_PA1 + 16);
    g_tx_running = false;
}

void qi_tx_set_power_pct(uint8_t pct)
{
    pct = (uint8_t)CLAMP(pct, 0, 100);
    /* Scale the HRTIM duty cycle; the MWCT closed loop will track it. */
    uint32_t per = HRTIM1->PERER;
    if (per == 0) per = (HRTIM_CLK_HZ * 8U) / 140000U;
    HRTIM1->CMP1ER = (per * pct) / 100U;
    /* Also inform the MWCT so it adjusts its demand loop. */
    mwct_cmd(MWCT_CMD_SET_POWER, pct);
    g_tx_duty_pct = pct;
}

int qi_tx_fod_override(int32_t offset_mw)
{
    /* FOD is safety-critical. Refuse unless the operator has explicitly
     * unlocked safety-research mode (see fod.c). */
    if (!fod_safety_unlocked())
        return -1;
    if (offset_mw < -2000 || offset_mw > 2000)
        return -1;
    g_fod_offset_mw = offset_mw;
    mwct_write_reg(MWCT_REG_FOD_OFFSET, (uint8_t)(offset_mw & 0xFF));
    return 0;
}

/* ---- Telemetry accessor (used by main loop + profiler) ----------------- */

int qi_tx_get_power_mw(void)
{
    return (int32_t)mwct_read_reg(MWCT_REG_POWER) * 10;  /* raw *10 mW */
}

int qi_tx_get_state(void)
{
    return (int)mwct_read_reg(MWCT_REG_STATE);
}

bool qi_tx_is_running(void) { return g_tx_running; }
uint32_t qi_tx_get_freq(void) { return g_tx_freq_hz; }
uint8_t qi_tx_get_duty(void) { return g_tx_duty_pct; }