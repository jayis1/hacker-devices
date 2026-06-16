/**
 * @file display_task.c
 * @author jayis1
 * @brief e-Paper Display Management Task
 *
 * Controls the 1.54" GDEH0154D67 e-Paper display for showing
 * SATA Phantom status, operation mode, battery level, link state,
 * and alerts. The display uses zero power when static.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "board.h"

static const char *TAG = "display";

/* e-Paper display dimensions (GDEH0154D67) */
#define EPD_WIDTH       200
#define EPD_HEIGHT      200
#define EPD_BYTES       ((EPD_WIDTH * EPD_HEIGHT) / 8)

/* Display buffer */
static uint8_t display_buffer[EPD_BYTES];

/* SPI device handle for e-Paper */
static spi_device_handle_t epd_spi;

/* ===================================================================
 * e-Paper Command Set (GDEH0154D67)
 * =================================================================== */
#define EPD_CMD_DRIVER_OUTPUT_CTRL      0x01
#define EPD_CMD_BOOSTER_SOFT_START      0x0C
#define EPD_CMD_GATE_SCAN_START         0x0F
#define EPD_CMD_DEEP_SLEEP              0x10
#define EPD_CMD_DATA_ENTRY_MODE         0x11
#define EPD_CMD_SW_RESET                0x12
#define EPD_CMD_TEMP_SENSOR_CTRL        0x18
#define EPD_CMD_MASTER_ACTIVATION       0x20
#define EPD_CMD_DISPLAY_UPDATE_CTRL1    0x21
#define EPD_CMD_DISPLAY_UPDATE_CTRL2    0x22
#define EPD_CMD_WRITE_RAM               0x24
#define EPD_CMD_WRITE_VCOM_REGISTER     0x2C
#define EPD_CMD_WRITE_LUT_REGISTER      0x32
#define EPD_CMD_DUMMY_LINE_PERIOD       0x3A
#define EPD_CMD_GATE_LINE_WIDTH         0x3B
#define EPD_CMD_BORDER_WAVEFORM_CTRL    0x3C
#define EPD_CMD_SET_RAM_X_ADDR          0x44
#define EPD_CMD_SET_RAM_Y_ADDR          0x45
#define EPD_CMD_SET_RAM_X_COUNTER       0x4E
#define EPD_CMD_SET_RAM_Y_COUNTER       0x4F
#define EPD_CMD_NOP                     0xFF

/* ===================================================================
 * Low-Level SPI Functions
 * =================================================================== */

static void epd_write_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_EPD_DC, 0);  /* Command mode */
    gpio_set_level(PIN_EPD_CS, 0);

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(epd_spi, &trans);

    gpio_set_level(PIN_EPD_CS, 1);
}

static void epd_write_data(uint8_t data)
{
    gpio_set_level(PIN_EPD_DC, 1);  /* Data mode */
    gpio_set_level(PIN_EPD_CS, 0);

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_transmit(epd_spi, &trans);

    gpio_set_level(PIN_EPD_CS, 1);
}

static void epd_write_data_bulk(const uint8_t *data, uint32_t len)
{
    gpio_set_level(PIN_EPD_DC, 1);
    gpio_set_level(PIN_EPD_CS, 0);

    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(epd_spi, &trans);

    gpio_set_level(PIN_EPD_CS, 1);
}

static void epd_wait_busy(void)
{
    int timeout_ms = 1000;
    while (gpio_get_level(PIN_EPD_BUSY) == 1 && timeout_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout_ms -= 10;
    }
    if (timeout_ms <= 0) {
        ESP_LOGW(TAG, "e-Paper busy timeout");
    }
}

/* ===================================================================
 * Display Initialization
 * =================================================================== */

static void epd_hardware_reset(void)
{
    gpio_set_level(PIN_EPD_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_EPD_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    epd_wait_busy();
}

static void epd_init(void)
{
    epd_hardware_reset();

    /* Booster soft start */
    epd_write_cmd(EPD_CMD_BOOSTER_SOFT_START);
    epd_write_data(0x17);
    epd_write_data(0x17);
    epd_write_data(0x17);

    /* Power settings */
    epd_write_cmd(EPD_CMD_DRIVER_OUTPUT_CTRL);
    epd_write_data((EPD_HEIGHT - 1) & 0xFF);
    epd_write_data(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    epd_write_data(0x00);

    epd_write_cmd(EPD_CMD_GATE_SCAN_START);
    epd_write_data(0x00);

    epd_write_cmd(EPD_CMD_DATA_ENTRY_MODE);
    epd_write_data(0x03);  /* X increment, Y increment */

    /* RAM window */
    epd_write_cmd(EPD_CMD_SET_RAM_X_ADDR);
    epd_write_data(0x00);
    epd_write_data((EPD_WIDTH / 8) - 1);

    epd_write_cmd(EPD_CMD_SET_RAM_Y_ADDR);
    epd_write_data(0x00);
    epd_write_data(0x00);
    epd_write_data((EPD_HEIGHT - 1) & 0xFF);
    epd_write_data(((EPD_HEIGHT - 1) >> 8) & 0xFF);

    /* Border waveform */
    epd_write_cmd(EPD_CMD_BORDER_WAVEFORM_CTRL);
    epd_write_data(0x05);

    /* Temperature sensor */
    epd_write_cmd(EPD_CMD_TEMP_SENSOR_CTRL);
    epd_write_data(0x80);  /* Internal temperature sensor */

    /* Display update control */
    epd_write_cmd(EPD_CMD_DISPLAY_UPDATE_CTRL2);
    epd_write_data(0xB1);  /* Enable clock, enable analog, load LUT */

    /* Master activation */
    epd_write_cmd(EPD_CMD_MASTER_ACTIVATION);
    epd_wait_busy();

    ESP_LOGI(TAG, "e-Paper initialized");
}

/* ===================================================================
 * Display Buffer Drawing Primitives
 * =================================================================== */

static void epd_clear_display(void)
{
    memset(display_buffer, 0xFF, EPD_BYTES);  /* All white */
}

static void epd_set_pixel(uint16_t x, uint16_t y, bool black)
{
    if (x >= EPD_WIDTH || y >= EPD_HEIGHT) return;
    uint32_t byte_idx = ((EPD_WIDTH / 8) * y) + (x / 8);
    if (byte_idx >= EPD_BYTES) return;

    if (black) {
        display_buffer[byte_idx] &= ~(0x80 >> (x % 8));
    } else {
        display_buffer[byte_idx] |= (0x80 >> (x % 8));
    }
}

static void epd_draw_char(uint16_t x, uint16_t y, char c, bool black)
{
    /* Simple 8x8 font stub — draws the character as blocks */
    /* In production, this would use a proper font table */
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 5; col++) {
            /* Minimal 5x8 character rendering */
            epd_set_pixel(x + col, y + row, black);
        }
        /* Column gap */
        epd_set_pixel(x + 5, y, false);
    }
}

static void epd_draw_string(uint16_t x, uint16_t y, const char *str, bool black)
{
    while (*str) {
        epd_draw_char(x, y, *str, black);
        x += 7;  /* 5px char + 2px spacing */
        str++;
    }
}

static void epd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool black)
{
    for (uint16_t i = x; i < x + w; i++) {
        epd_set_pixel(i, y, black);
        epd_set_pixel(i, y + h - 1, black);
    }
    for (uint16_t i = y; i < y + h; i++) {
        epd_set_pixel(x, i, black);
        epd_set_pixel(x + w - 1, i, black);
    }
}

static void epd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool black)
{
    for (uint16_t i = y; i < y + h; i++) {
        for (uint16_t j = x; j < x + w; j++) {
            epd_set_pixel(j, i, black);
        }
    }
}

/* ===================================================================
 * UI Layout Functions
 * =================================================================== */

/**
 * @brief Draw the main status screen on the display buffer.
 */
static void epd_draw_status_screen(operation_mode_t mode, bool link_up,
                                    sata_speed_t speed, uint32_t battery_mv,
                                    uint32_t frames_captured, uint32_t rules_active)
{
    epd_clear_display();

    /* Title bar */
    epd_fill_rect(0, 0, EPD_WIDTH, 16, true);
    epd_draw_string(5, 3, "SATA PHANTOM v1.0", false);

    /* Mode indicator */
    const char *mode_str = "TRANS";
    switch (mode) {
        case MODE_TRANSPARENT: mode_str = "TRANSPARENT"; break;
        case MODE_MONITOR:     mode_str = "MONITOR";     break;
        case MODE_ACTIVE:      mode_str = "ACTIVE";      break;
        case MODE_EXFIL:       mode_str = "EXFIL";       break;
        case MODE_DEEP_SLEEP:  mode_str = "SLEEP";       break;
        case MODE_USB_CONFIG:  mode_str = "USB CFG";     break;
    }

    char line[64];
    snprintf(line, sizeof(line), "Mode: %s", mode_str);
    epd_draw_string(5, 25, line, true);

    /* Link status */
    if (link_up) {
        const char *speed_str = "1.5G";
        if (speed == SATA_SPEED_3_0) speed_str = "3.0G";
        else if (speed == SATA_SPEED_6_0) speed_str = "6.0G";
        snprintf(line, sizeof(line), "Link: UP @ %s", speed_str);
        epd_draw_string(5, 40, line, true);
    } else {
        epd_draw_string(5, 40, "Link: DOWN", true);
    }

    /* Battery */
    snprintf(line, sizeof(line), "Batt: %d mV", battery_mv);
    epd_draw_string(5, 55, line, true);

    /* Stats */
    snprintf(line, sizeof(line), "Frames: %lu", (unsigned long)frames_captured);
    epd_draw_string(5, 75, line, true);

    snprintf(line, sizeof(line), "Rules: %lu", (unsigned long)rules_active);
    epd_draw_string(5, 90, line, true);

    /* Author credit */
    epd_draw_string(5, EPD_HEIGHT - 15, "jayis1 (c) 2025", true);

    /* Status bar at bottom */
    epd_fill_rect(0, EPD_HEIGHT - 3, EPD_WIDTH, 3, true);
}

/**
 * @brief Flash the display buffer to the e-Paper.
 */
static void epd_update_display(void)
{
    /* Set RAM counters to origin */
    epd_write_cmd(EPD_CMD_SET_RAM_X_COUNTER);
    epd_write_data(0x00);
    epd_write_cmd(EPD_CMD_SET_RAM_Y_COUNTER);
    epd_write_data(0x00);
    epd_write_data(0x00);

    /* Write image data to RAM */
    epd_write_cmd(EPD_CMD_WRITE_RAM);
    epd_write_data_bulk(display_buffer, EPD_BYTES);

    /* Trigger display update */
    epd_write_cmd(EPD_CMD_DISPLAY_UPDATE_CTRL2);
    epd_write_data(0xC7);
    epd_write_cmd(EPD_CMD_MASTER_ACTIVATION);
    epd_wait_busy();
}

/* ===================================================================
 * Display Task Main Loop
 * =================================================================== */

void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task started");

    /* Wait for FPGA ready before touching SPI */
    xEventGroupWaitBits(system_events, EVENT_BIT_FPGA_READY,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    /* Initialize SPI for e-Paper */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_EPD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_EPD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EPD_BYTES + 10,
    };
    spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 4 * 1000 * 1000,  /* 4 MHz */
        .spics_io_num = PIN_EPD_CS,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI3_HOST, &dev_cfg, &epd_spi);

    /* Initialize the e-Paper display */
    epd_init();
    epd_clear_display();
    epd_draw_status_screen(MODE_TRANSPARENT, false, SATA_SPEED_UNKNOWN,
                            3700, 0, 0);
    epd_update_display();
    ESP_LOGI(TAG, "Display initialized and showing splash");

    /* Status tracking for debouncing display updates */
    operation_mode_t last_mode = MODE_TRANSPARENT;
    bool last_link = false;
    uint32_t update_counter = 0;

    while (1) {
        /* Read current system state */
        operation_mode_t mode = current_mode;
        bool link_up = (xEventGroupGetBits(system_events) & EVENT_BIT_LINK_UP) != 0;
        sata_speed_t speed = SATA_SPEED_UNKNOWN;  /* TODO: read from FPGA */

        /* Read battery voltage */
        uint32_t battery_mv = 3700;  /* Default nominal */
        /* In real impl: read MAX17048 via I2C */

        /* Read frame count from FPGA */
        uint32_t frames = 0;
        /* In real impl: read REG_STAT_COUNTER_READ + REG_STAT_COUNTER_WRITE */

        /* Read active rule count */
        uint32_t rules = 0;
        /* In real impl: query policy_engine */

        /* Update display only if state has changed (or every 60 cycles) */
        if (mode != last_mode || link_up != last_link || update_counter >= 60) {
            epd_draw_status_screen(mode, link_up, speed, battery_mv, frames, rules);
            epd_update_display();
            last_mode = mode;
            last_link = link_up;
            update_counter = 0;
            ESP_LOGD(TAG, "Display updated (mode=%d, link=%d)", mode, link_up);
        }

        update_counter++;
        vTaskDelay(pdMS_TO_TICKS(5000));  /* Check every 5 seconds */
    }
}
