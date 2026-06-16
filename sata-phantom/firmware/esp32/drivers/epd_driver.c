/**
 * @file epd_driver.c
 * @author jayis1
 * @brief e-Paper Display Driver (GDEH0154D67 / 1.54" 200×200)
 *
 * Low-level driver for the Waveshare-compatible 1.54" e-Paper display.
 * Handles SPI communication, display initialization, image data upload,
 * and refresh control. The e-Paper maintains its image with zero power.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "board.h"

static const char *TAG = "epd";

/* Display dimensions */
#define EPD_W           200
#define EPB_H           200
#define EPD_BUF_SIZE    ((EPD_W * EPB_H) / 8)  /* 5000 bytes */

/* SPI handle */
static spi_device_handle_t epd_spi_dev;
static bool epd_initialized = false;

/* ===================================================================
 * Low-Level SPI Primitives
 * =================================================================== */

static void epd_cs_low(void)
{
    gpio_set_level(PIN_EPD_CS, 0);
}

static void epd_cs_high(void)
{
    gpio_set_level(PIN_EPD_CS, 1);
}

static void epd_dc_cmd(void)
{
    gpio_set_level(PIN_EPD_DC, 0);
}

static void epd_dc_data(void)
{
    gpio_set_level(PIN_EPD_DC, 1);
}

static void epd_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/**
 * @brief Send a single command byte.
 */
static void epd_send_cmd(uint8_t cmd)
{
    epd_dc_cmd();
    epd_cs_low();

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(epd_spi_dev, &trans);

    epd_cs_high();
}

/**
 * @brief Send a single data byte.
 */
static void epd_send_data(uint8_t data)
{
    epd_dc_data();
    epd_cs_low();

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_transmit(epd_spi_dev, &trans);

    epd_cs_high();
}

/**
 * @brief Send multiple data bytes.
 */
static void epd_send_data_bulk(const uint8_t *data, uint32_t len)
{
    epd_dc_data();
    epd_cs_low();

    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(epd_spi_dev, &trans);

    epd_cs_high();
}

/**
 * @brief Wait for the e-Paper busy pin to go high (not busy).
 */
static void epd_wait_idle(void)
{
    int timeout = 1000;
    while (gpio_get_level(PIN_EPD_BUSY) == 1 && timeout-- > 0) {
        epd_delay_ms(1);
    }
    if (timeout <= 0) {
        ESP_LOGW(TAG, "e-Paper busy timeout");
    }
}

/* ===================================================================
 * Initialization Sequence
 * =================================================================== */

/**
 * @brief Initialize the e-Paper display controller.
 */
void epd_init_display(void)
{
    if (epd_initialized) return;

    ESP_LOGI(TAG, "Initializing e-Paper display");

    /* Hardware reset */
    gpio_set_level(PIN_EPD_RESET, 0);
    epd_delay_ms(10);
    gpio_set_level(PIN_EPD_RESET, 1);
    epd_delay_ms(10);
    epd_wait_idle();

    /* Booster soft start */
    epd_send_cmd(0x0C);
    epd_send_data(0xAE);
    epd_send_data(0xC7);
    epd_send_data(0xC3);
    epd_send_data(0xC0);
    epd_send_data(0x40);

    /* Driver output control */
    epd_send_cmd(0x01);
    epd_send_data((EPB_H - 1) & 0xFF);
    epd_send_data(((EPB_H - 1) >> 8) & 0xFF);
    epd_send_data(0x00);  /* Normal scan */

    /* Data entry mode */
    epd_send_cmd(0x11);
    epd_send_data(0x03);  /* X increment, Y increment */

    /* RAM X address range */
    epd_send_cmd(0x44);
    epd_send_data(0x00);
    epd_send_data((EPD_W / 8) - 1);

    /* RAM Y address range */
    epd_send_cmd(0x45);
    epd_send_data(0x00);
    epd_send_data(0x00);
    epd_send_data((EPB_H - 1) & 0xFF);
    epd_send_data(((EPB_H - 1) >> 8) & 0xFF);

    /* Border waveform */
    epd_send_cmd(0x3C);
    epd_send_data(0x01);  /* Border floating */

    /* Temperature sensor control */
    epd_send_cmd(0x18);
    epd_send_data(0x80);  /* Internal temperature sensor */

    /* Display update control 1 */
    epd_send_cmd(0x21);
    epd_send_data(0x80);  /* Enable clock */
    epd_send_data(0x80);  /* Enable analog */

    /* Display update control 2 */
    epd_send_cmd(0x22);
    epd_send_data(0xB1);  /* Load LUT from OTP */

    /* Master activation */
    epd_send_cmd(0x20);
    epd_wait_idle();

    epd_initialized = true;
    ESP_LOGI(TAG, "e-Paper initialized (%dx%d)", EPD_W, EPB_H);
}

/**
 * @brief Enter deep sleep mode on the display.
 */
void epd_sleep(void)
{
    epd_send_cmd(0x10);
    epd_send_data(0x01);
    epd_delay_ms(100);
    epd_initialized = false;
}

/* ===================================================================
 * Image Buffer Upload
 * =================================================================== */

/**
 * @brief Upload a full image buffer to the display RAM.
 * @param buffer 200×200 monochrome image (1 bit per pixel, 1=white, 0=black)
 *               Organized as column-major: byte 0 = column 0 pixels 0-7, etc.
 */
void epd_upload_image(const uint8_t *buffer)
{
    if (!buffer) return;

    /* Set RAM X address counter */
    epd_send_cmd(0x4E);
    epd_send_data(0x00);

    /* Set RAM Y address counter */
    epd_send_cmd(0x4F);
    epd_send_data(0x00);
    epd_send_data(0x00);

    /* Write RAM (black/white) */
    epd_send_cmd(0x24);
    epd_send_data_bulk(buffer, EPD_BUF_SIZE);

    /* Write RAM (red — not used, set to all white) */
    epd_send_cmd(0x26);
    uint8_t white_byte = 0xFF;
    for (uint32_t i = 0; i < EPD_BUF_SIZE; i++) {
        epd_send_data(white_byte);
    }
}

/* ===================================================================
 * Display Refresh
 * =================================================================== */

/**
 * @brief Trigger a full display refresh from RAM to the panel.
 */
void epd_refresh(void)
{
    /* Display update control 2 */
    epd_send_cmd(0x22);
    epd_send_data(0xF7);  /* Enable clock, analog, load LUT, display mode */

    /* Master activation */
    epd_send_cmd(0x20);
    epd_wait_idle();
}

/**
 * @brief High-level display function: clear, upload, and refresh.
 * @param buffer Monochrome image buffer (5000 bytes)
 */
void epd_display(const uint8_t *buffer)
{
    if (!epd_initialized) {
        epd_init_display();
    }
    epd_upload_image(buffer);
    epd_refresh();
}

/**
 * @brief Clear the display to white.
 */
void epd_clear(void)
{
    uint8_t clear_buf[EPD_BUF_SIZE];
    memset(clear_buf, 0xFF, EPD_BUF_SIZE);
    epd_display(clear_buf);
    ESP_LOGI(TAG, "Display cleared to white");
}

/**
 * @brief Fill the entire display black (for testing).
 */
void epd_fill_black(void)
{
    uint8_t black_buf[EPD_BUF_SIZE];
    memset(black_buf, 0x00, EPD_BUF_SIZE);
    epd_display(black_buf);
    ESP_LOGI(TAG, "Display filled black");
}
