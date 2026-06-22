/*
 * fpga.c — WireReaper iCE40-UP5K FPGA co-processor driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Manages the Lattice iCE40-UP5K FPGA for high-speed bus sampling
 * and complex trigger logic. Handles bitstream loading via SPI,
 * command/status registers, and sample FIFO drainage.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* FPGA SPI interface (SPI1 on the STM32H743) */
#define FPGA_SPI_BASE   0x40013000UL

/* FPGA register addresses (accessed via SPI command frame) */
#define FPGA_REG_VERSION    0x00
#define FPGA_REG_CTRL       0x01
#define FPGA_REG_STATUS     0x02
#define FPGA_REG_SAMPLERATE 0x03
#define FPGA_REG_TRIGGER    0x04
#define FPGA_REG_TRIGGERVAL 0x05
#define FPGA_REG_FIFO_DATA  0x10
#define FPGA_REG_FIFO_LEVEL 0x11
#define FPGA_REG_CHAN_CTRL  0x20
#define FPGA_REG_CHAN_MODE  0x21

/* FPGA control bits */
#define FPGA_CTRL_ENABLE    0x01
#define FPGA_CTRL_CAPTURE   0x02
#define FPGA_CTRL_RESET     0x04
#define FPGA_CTRL_TRIGGER   0x08

/* FPGA status bits */
#define FPGA_STAT_CDONE     0x01
#define FPGA_STAT_READY     0x02
#define FPGA_STAT_FIFO_FULL 0x04
#define FPGA_STAT_FIFO_EMPTY 0x08
#define FPGA_STAT_TRIGGERED 0x10

/* ---- FPGA GPIO control ---- */
static void fpga_gpio_init(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOD;
    gpio_t *g = GPIO(FPGA_GPIO_PORT);

    /* CDONE: input */
    g->MODER &= ~(3U << (FPGA_CDONE_PIN * 2));
    g->PUPDR |= (GPIO_PUPD_PULLDOWN << (FPGA_CDONE_PIN * 2));

    /* CRESET: output */
    g->MODER &= ~(3U << (FPGA_CRESET_PIN * 2));
    g->MODER |= (GPIO_MODE_OUTPUT << (FPGA_CRESET_PIN * 2));
    g->OTYPER &= ~(1U << FPGA_CRESET_PIN);

    /* INIT: input */
    g->MODER &= ~(3U << (FPGA_INIT_PIN * 2));
}

static void fpga_creset_low(void) {
    GPIO(FPGA_GPIO_PORT)->BSRR = (1U << (FPGA_CRESET_PIN + 16));
}

static void fpga_creset_high(void) {
    GPIO(FPGA_GPIO_PORT)->BSRR = (1U << FPGA_CRESET_PIN);
}

static int fpga_cdone(void) {
    return (GPIO(FPGA_GPIO_PORT)->IDR &
            (1U << FPGA_CDONE_PIN)) ? 1 : 0;
}

/* ---- SPI to FPGA ---- */
static void fpga_spi_begin(void) {
    GPIO(GPIOA_BASE)->BSRR = (1U << (SPI_FPGA_NCS_PIN + 16)); /* NCS low */
}

static void fpga_spi_end(void) {
    GPIO(GPIOA_BASE)->BSRR = (1U << SPI_FPGA_NCS_PIN); /* NCS high */
}

static uint8_t fpga_spi_xfer(uint8_t tx) {
    /* Use SPI1 (simplified register access) */
    volatile uint32_t *spi_dr = (volatile uint32_t *)(SPI_FPGA_BASE + 0x0C);
    volatile uint32_t *spi_sr = (volatile uint32_t *)(SPI_FPGA_BASE + 0x08);

    while (!(*spi_sr & SPI_SR_TXE));
    *spi_dr = tx;
    while (!(*spi_sr & SPI_SR_RXNE));
    return (uint8_t)*spi_dr;
}

/* ---- Write to FPGA register ---- */
static void fpga_write_reg(uint8_t reg, uint32_t value) {
    fpga_spi_begin();
    fpga_spi_xfer(0x80 | reg); /* Write command */
    fpga_spi_xfer((value >> 0) & 0xFF);
    fpga_spi_xfer((value >> 8) & 0xFF);
    fpga_spi_xfer((value >> 16) & 0xFF);
    fpga_spi_xfer((value >> 24) & 0xFF);
    fpga_spi_end();
}

/* ---- Read from FPGA register ---- */
static uint32_t fpga_read_reg(uint8_t reg) {
    fpga_spi_begin();
    fpga_spi_xfer(reg); /* Read command */
    uint32_t val = 0;
    val |= fpga_spi_xfer(0x00);
    val |= ((uint32_t)fpga_spi_xfer(0x00)) << 8;
    val |= ((uint32_t)fpga_spi_xfer(0x00)) << 16;
    val |= ((uint32_t)fpga_spi_xfer(0x00)) << 24;
    fpga_spi_end();
    return val;
}

/* ---- Load FPGA bitstream ---- */
/* The iCE40 configuration protocol:
 * 1. Pull CRESET low for >200 ns
 * 2. Release CRESET
 * 3. Wait for CDONE to go low (or just delay)
 * 4. Send bitstream on SPI with SS_B low
 * 5. Send extra 49 dummy clocks
 * 6. Wait for CDONE to go high
 */
void wr_fpga_load_bitstream(const uint8_t *bits, int len) {
    fpga_gpio_init();

    /* Reset FPGA */
    fpga_creset_low();
    /* Delay > 1 us (use DWT) */
    volatile uint32_t *dwt = (volatile uint32_t *)0xE0001004;
    volatile uint32_t *dwt_ctrl = (volatile uint32_t *)0xE0001000;
    *dwt_ctrl |= 1;
    uint32_t start = *dwt;
    while ((*dwt - start) < (HCLK_HZ / 1000000)); /* 1 us */
    fpga_creset_high();

    /* Wait a bit for FPGA to be ready */
    start = *dwt;
    while ((*dwt - start) < (HCLK_HZ / 10000)); /* 100 us */

    /* Send bitstream */
    fpga_spi_begin();
    for (int i = 0; i < len; i++)
        fpga_spi_xfer(bits[i]);
    /* Send 49+ dummy clocks (7 bytes) */
    for (int i = 0; i < 8; i++)
        fpga_spi_xfer(0x00);
    fpga_spi_end();

    /* Wait for CDONE */
    uint32_t timeout = 100000;
    while (!fpga_cdone() && timeout > 0) timeout--;
}

/* ---- Initialize FPGA interface ---- */
void wr_fpga_init(void) {
    /* Enable SPI1 clock */
    RCC_APB2ENR |= RCC_APB2ENR_SPI1;
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOB | RCC_AHB1ENR_GPIOA;

    /* Configure SPI1 pins: SCK=PB3, MISO=PB4, MOSI=PB5, NCS=PA15 */
    gpio_t *b = GPIO(GPIOB_BASE);
    uint32_t af = 5; /* SPI1 AF5 */
    int pins[] = {SPI_FPGA_SCK_PIN, SPI_FPGA_MISO_PIN, SPI_FPGA_MOSI_PIN};
    for (int i = 0; i < 3; i++) {
        int p = pins[i];
        b->MODER &= ~(3U << (p * 2));
        b->MODER |= (GPIO_MODE_AF << (p * 2));
        b->OSPEEDR |= (GPIO_OSPEED_VHIGH << (p * 2));
        b->AFR[p / 8] &= ~(0xFU << ((p % 8) * 4));
        b->AFR[p / 8] |= (af << ((p % 8) * 4));
    }

    /* NCS as GPIO output */
    gpio_t *a = GPIO(GPIOA_BASE);
    a->MODER &= ~(3U << (SPI_FPGA_NCS_PIN * 2));
    a->MODER |= (GPIO_MODE_OUTPUT << (SPI_FPGA_NCS_PIN * 2));
    a->BSRR = (1U << SPI_FPGA_NCS_PIN); /* High (deselected) */

    /* Configure SPI1: master, 8-bit, high speed */
    volatile uint32_t *spi_cr1 = (volatile uint32_t *)(SPI_FPGA_BASE + 0x00);
    volatile uint32_t *spi_cr2 = (volatile uint32_t *)(SPI_FPGA_BASE + 0x04);

    *spi_cr1 = 0;
    *spi_cr1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (2U << 3); /* BR=/8 */
    *spi_cr2 = 0x00070000; /* 8-bit FIFO */
    *spi_cr1 |= SPI_CR1_SPE;

    fpga_gpio_init();

    /* If CDONE is high, FPGA is already configured (e.g., from flash) */
    if (fpga_cdone()) {
        /* Reset internal logic */
        fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_RESET);
        fpga_write_reg(FPGA_REG_CTRL, 0);
    }
}

/* ---- Set trigger condition ---- */
void wr_fpga_set_trigger(int ch, uint32_t mask, uint32_t value) {
    fpga_write_reg(FPGA_REG_TRIGGER, mask);
    fpga_write_reg(FPGA_REG_TRIGGERVAL, value);
    fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_TRIGGER | FPGA_CTRL_ENABLE);
}

/* ---- Start high-speed sampling on a channel ---- */
int wr_fpga_start_sampling(int ch, uint32_t sample_rate_hz) {
    if (ch < 0 || ch >= NUM_SPI_CHANNELS)
        return WR_ERR_PARAM;

    /* Set sample rate */
    fpga_write_reg(FPGA_REG_SAMPLERATE, sample_rate_hz);
    /* Select channel */
    fpga_write_reg(FPGA_REG_CHAN_CTRL, (1U << ch));
    /* Set mode: sniff */
    fpga_write_reg(FPGA_REG_CHAN_MODE, 0x01);
    /* Start capture */
    fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_ENABLE | FPGA_CTRL_CAPTURE);

    return WR_OK;
}

/* ---- Stop sampling ---- */
void wr_fpga_stop_sampling(void) {
    fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_ENABLE);
}

/* ---- Read samples from FIFO ---- */
int wr_fpga_read_samples(uint8_t *buf, int maxlen) {
    uint32_t level = fpga_read_reg(FPGA_REG_FIFO_LEVEL);
    int count = (int)level;
    if (count > maxlen)
        count = maxlen;

    fpga_spi_begin();
    fpga_spi_xfer(FPGA_REG_FIFO_DATA);
    for (int i = 0; i < count; i++)
        buf[i] = fpga_spi_xfer(0x00);
    fpga_spi_end();

    return count;
}

/* ---- Get FPGA status ---- */
uint32_t wr_fpga_status(void) {
    return fpga_read_reg(FPGA_REG_STATUS);
}

/* ---- Get FPGA version ---- */
uint32_t wr_fpga_version(void) {
    return fpga_read_reg(FPGA_REG_VERSION);
}