/**
 * @file fpga_driver.c
 * @brief iCE40UP5K FPGA driver implementation
 *
 * Manages configuration and communication with the Lattice iCE40UP5K FPGA
 * that handles high-speed display bus capture for GLYPH-REAPER.
 *
 * The FPGA is configured via SPI from bitstreams stored in external NOR flash.
 * Once configured, it captures display bus traffic and provides pixel data
 * to the MCU via a parallel data bus with sync signals.
 *
 * Supported protocols (each with a separate bitstream):
 *   - MIPI DSI (1-4 lanes, D-PHY, up to 1.5 Gbps/lane)
 *   - RGB Parallel (8/16/18/24-bit, up to 100 MHz PCLK)
 *   - SPI LCD (3-wire/4-wire, ILI9341/ST7789/SSD1306 command sets)
 *   - LVDS (single/dual link, up to 135 MHz PCLK)
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "fpga_driver.h"
#include "flash_driver.h"
#include "registers.h"

/*===========================================================================
 * INTERNAL STATE
 *===========================================================================*/

static display_protocol_t s_current_protocol = DISPLAY_PROTO_NONE;
static bool s_capture_enabled = false;
static uint16_t s_frame_width = 0;
static uint16_t s_frame_height = 0;
static uint8_t s_color_depth = 16;
static uint8_t s_status = 0;

/*===========================================================================
 * SPI HELPER FUNCTIONS
 *===========================================================================*/

static void spi_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    /* Configure SPIM0 for FPGA communication */
    NRF_SPIM0->ENABLE = SPIM_ENABLE_DISABLE;

    /* Set up TX and RX buffers */
    NRF_SPIM0->TXD_PTR = (uint32_t)tx;
    NRF_SPIM0->RXD_PTR = (uint32_t)rx;
    NRF_SPIM0->TXD_MAXCNT = len;
    NRF_SPIM0->RXD_MAXCNT = len;

    /* Enable SPIM */
    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE;

    /* Start transfer */
    NRF_SPIM0->TASKS_START = 1;

    /* Wait for completion */
    while (NRF_SPIM0->EVENTS_END == 0);
    NRF_SPIM0->EVENTS_END = 0;

    /* Disable SPIM */
    NRF_SPIM0->ENABLE = SPIM_ENABLE_DISABLE;
}

static void spi_write_byte(uint8_t cmd, uint8_t data)
{
    uint8_t tx[2] = { cmd, data };
    uint8_t rx[2];

    /* Assert CS */
    GPIO_OUT_CLR(0, FPGA_SPI_CSN_PIN);
    spi_transfer(tx, rx, 2);
    /* Deassert CS */
    GPIO_OUT_SET(0, FPGA_SPI_CSN_PIN);
}

static uint8_t spi_read_byte(uint8_t cmd)
{
    uint8_t tx[2] = { cmd, 0xFF };
    uint8_t rx[2];

    GPIO_OUT_CLR(0, FPGA_SPI_CSN_PIN);
    spi_transfer(tx, rx, 2);
    GPIO_OUT_SET(0, FPGA_SPI_CSN_PIN);

    return rx[1];
}

static void spi_write_burst(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint8_t tx[256];
    uint8_t rx[256];

    if (len + 1 > sizeof(tx)) return;

    tx[0] = cmd;
    memcpy(&tx[1], data, len);

    GPIO_OUT_CLR(0, FPGA_SPI_CSN_PIN);
    spi_transfer(tx, rx, len + 1);
    GPIO_OUT_SET(0, FPGA_SPI_CSN_PIN);
}

static void spi_read_burst(uint8_t cmd, uint8_t *data, uint16_t len)
{
    uint8_t tx[256];
    uint8_t rx[256];

    if (len + 1 > sizeof(tx)) return;

    memset(tx, 0xFF, len + 1);
    tx[0] = cmd;

    GPIO_OUT_CLR(0, FPGA_SPI_CSN_PIN);
    spi_transfer(tx, rx, len + 1);
    GPIO_OUT_SET(0, FPGA_SPI_CSN_PIN);

    memcpy(data, &rx[1], len);
}

/*===========================================================================
 * FPGA CONFIGURATION
 *===========================================================================*/

void fpga_driver_init(void)
{
    /* Initialize SPI pins for FPGA communication */
    NRF_SPIM0->PSEL_SCK = BOARD_PIN_FPGA_SPI_SCK;
    NRF_SPIM0->PSEL_MOSI = BOARD_PIN_FPGA_SPI_MOSI;
    NRF_SPIM0->PSEL_MISO = BOARD_PIN_FPGA_SPI_MISO;
    NRF_SPIM0->FREQUENCY = SPIM_FREQ_16M;
    NRF_SPIM0->CONFIG = SPIM_CONFIG_ORDER_MSB | SPIM_CONFIG_CPOL_ACTIVEHIGH |
                        SPIM_CONFIG_CPHA_LEADING;

    /* Set CS high (idle) */
    GPIO_OUT_SET(0, FPGA_SPI_CSN_PIN);

    /* Reset FPGA */
    FPGA_RESET_ASSERT();
    for (volatile int i = 0; i < 10000; i++);
    FPGA_RESET_RELEASE();

    s_current_protocol = DISPLAY_PROTO_NONE;
    s_capture_enabled = false;
    s_status = 0;
}

bool fpga_configure_bitstream(display_protocol_t protocol)
{
    uint32_t bitstream_addr = 0;
    uint8_t bitstream_buf[256];
    uint32_t offset = 0;
    uint32_t bitstream_size = FPGA_BITSTREAM_SIZE;

    /* Select bitstream address based on protocol */
    switch (protocol) {
    case DISPLAY_PROTO_MIPI_DSI:
        bitstream_addr = FPGA_BITSTREAM_MIPI_DSI;
        break;
    case DISPLAY_PROTO_RGB_PARALLEL:
        bitstream_addr = FPGA_BITSTREAM_RGB_PARALLEL;
        break;
    case DISPLAY_PROTO_SPI_LCD:
        bitstream_addr = FPGA_BITSTREAM_SPI_LCD;
        break;
    case DISPLAY_PROTO_LVDS:
        bitstream_addr = FPGA_BITSTREAM_LVDS;
        break;
    default:
        return false;
    }

    /* Assert FPGA reset to enter configuration mode */
    FPGA_RESET_ASSERT();

    /* Wait minimum reset time (800ns for iCE40) */
    for (volatile int i = 0; i < 10; i++);

    /* Release reset — FPGA now expects configuration clock on SCK */
    FPGA_RESET_RELEASE();

    /* Wait for CRSTB to be high and check CDONE is low */
    if (FPGA_CDONE_IS_HIGH()) {
        /* CDONE should be low during configuration */
        return false;
    }

    /* Configure SPI for bitstream upload (slower speed for reliability) */
    NRF_SPIM0->FREQUENCY = SPIM_FREQ_8M;

    /* Assert CS */
    GPIO_OUT_CLR(0, FPGA_SPI_CSN_PIN);

    /* Stream bitstream from flash to FPGA */
    while (offset < bitstream_size) {
        uint16_t chunk = MIN(sizeof(bitstream_buf), bitstream_size - offset);

        /* Read bitstream chunk from flash */
        flash_read(bitstream_addr + offset, bitstream_buf, chunk);

        /* Send to FPGA via SPI */
        uint8_t rx[256];
        NRF_SPIM0->TXD_PTR = (uint32_t)bitstream_buf;
        NRF_SPIM0->RXD_PTR = (uint32_t)rx;
        NRF_SPIM0->TXD_MAXCNT = chunk;
        NRF_SPIM0->RXD_MAXCNT = chunk;
        NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE;
        NRF_SPIM0->TASKS_START = 1;
        while (NRF_SPIM0->EVENTS_END == 0);
        NRF_SPIM0->EVENTS_END = 0;
        NRF_SPIM0->ENABLE = SPIM_ENABLE_DISABLE;

        offset += chunk;
    }

    /* Deassert CS */
    GPIO_OUT_SET(0, FPGA_SPI_CSN_PIN);

    /* Send additional dummy clocks for configuration completion */
    /* iCE40 requires at least 49 additional dummy clocks after bitstream */
    uint8_t dummy_tx[8] = { 0 };
    uint8_t dummy_rx[8];
    GPIO_OUT_CLR(0, FPGA_SPI_CSN_PIN);
    spi_transfer(dummy_tx, dummy_rx, sizeof(dummy_tx));
    GPIO_OUT_SET(0, FPGA_SPI_CSN_PIN);

    /* Wait for CDONE to go high */
    uint32_t timeout = 10000;
    while (!FPGA_CDONE_IS_HIGH()) {
        if (--timeout == 0) {
            /* Configuration failed */
            NRF_SPIM0->FREQUENCY = SPIM_FREQ_16M;
            return false;
        }
    }

    /* Restore SPI speed for normal communication */
    NRF_SPIM0->FREQUENCY = SPIM_FREQ_16M;

    /* Send initial dummy byte to synchronize */
    spi_write_byte(0x00, 0x00);

    s_current_protocol = protocol;
    s_status = 0;

    return true;
}

/*===========================================================================
 * CAPTURE CONTROL
 *===========================================================================*/

void fpga_set_resolution(uint16_t width, uint16_t height, uint8_t color_depth)
{
    s_frame_width = width;
    s_frame_height = height;
    s_color_depth = color_depth;

    /* Send resolution configuration to FPGA */
    uint8_t config[6];
    config[0] = (width >> 8) & 0xFF;
    config[1] = width & 0xFF;
    config[2] = (height >> 8) & 0xFF;
    config[3] = height & 0xFF;
    config[4] = color_depth;
    config[5] = 0;  /* Reserved */

    spi_write_burst(0x42, config, sizeof(config));  /* 0x42 = SET_RESOLUTION */
}

void fpga_enable_capture(bool enable)
{
    if (enable) {
        spi_write_byte(0x40, FPGA_CTRL_ENABLE_CAPTURE | FPGA_CTRL_RESET_CAPTURE);
        s_capture_enabled = true;
    } else {
        spi_write_byte(0x40, FPGA_CTRL_DISABLE_CAPTURE);
        s_capture_enabled = false;
    }
}

bool fpga_is_frame_ready(void)
{
    s_status = spi_read_byte(0x44);  /* 0x44 = GET_STATUS */
    return (s_status & FPGA_STATUS_FRAME_READY) != 0;
}

uint32_t fpga_read_frame(uint8_t *buffer, uint16_t width, uint16_t height,
                         uint8_t color_depth)
{
    uint32_t frame_size = (uint32_t)width * height * (color_depth / 8);
    uint32_t offset = 0;
    uint16_t chunk_size;

    if (buffer == NULL || frame_size == 0) {
        return 0;
    }

    /* Request frame read from FPGA */
    spi_write_byte(0x46, 0x01);  /* 0x46 = READ_FRAME_CMD */

    /* Read frame data in chunks */
    while (offset < frame_size) {
        chunk_size = MIN(255, frame_size - offset);
        spi_read_burst(0x47, buffer + offset, chunk_size);  /* 0x47 = READ_FRAME_DATA */
        offset += chunk_size;
    }

    /* Clear frame ready flag */
    spi_write_byte(0x40, FPGA_CTRL_CLEAR_OVERFLOW);

    return frame_size;
}

uint8_t fpga_get_status(void)
{
    s_status = spi_read_byte(0x44);
    return s_status;
}

void fpga_clear_overflow(void)
{
    spi_write_byte(0x40, FPGA_CTRL_CLEAR_OVERFLOW);
}

void fpga_reset_capture(void)
{
    spi_write_byte(0x40, FPGA_CTRL_RESET_CAPTURE);
    if (s_capture_enabled) {
        spi_write_byte(0x40, FPGA_CTRL_ENABLE_CAPTURE);
    }
}

void fpga_power_down(void)
{
    /* Disable capture first */
    fpga_enable_capture(false);

    /* Send power-down command */
    spi_write_byte(0x48, 0x01);  /* 0x48 = POWER_DOWN */

    /* Assert reset to save power */
    FPGA_RESET_ASSERT();

    s_status = FPGA_STATUS_POWER_DOWN;
}

void fpga_wake_up(void)
{
    /* Release reset */
    FPGA_RESET_RELEASE();

    /* Reconfigure with last protocol */
    if (s_current_protocol != DISPLAY_PROTO_NONE) {
        fpga_configure_bitstream(s_current_protocol);
        fpga_set_resolution(s_frame_width, s_frame_height, s_color_depth);
    }
}

bool fpga_reconfigure(display_protocol_t protocol)
{
    /* Full reconfiguration cycle */
    fpga_power_down();
    for (volatile int i = 0; i < 10000; i++);

    if (!fpga_configure_bitstream(protocol)) {
        return false;
    }

    fpga_set_resolution(s_frame_width, s_frame_height, s_color_depth);
    return true;
}