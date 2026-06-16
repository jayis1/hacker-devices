//==============================================================================
// display.c — Spectre-Sniffer ILI9341 TFT Display Driver
// Author: jayis1
// Description: Low-level display controller for the 2.8" 320x240 TFT,
//              including framebuffer management, font rendering, and
//              waveform/spectrum visualization primitives.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "board.h"

static const char *TAG = "DISPLAY";

//==============================================================================
// Display State
//==============================================================================
typedef struct {
    spi_device_handle_t spi;
    uint16_t            width;
    uint16_t            height;
    uint8_t             rotation;
    bool                initialized;
    uint16_t            cursor_x;
    uint16_t            cursor_y;
    uint16_t            text_color;
    uint16_t            bg_color;
} display_state_t;

static display_state_t s_display = {
    .spi = NULL,
    .width = DISPLAY_WIDTH_PX,
    .height = DISPLAY_HEIGHT_PX,
    .rotation = 0,
    .initialized = false,
    .cursor_x = 0,
    .cursor_y = 0,
    .text_color = 0xFFFF,
    .bg_color = 0x0000,
};

//==============================================================================
// Color Definitions (RGB565)
//==============================================================================
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_ORANGE        0xFC00
#define COLOR_DARK_GREEN    0x03E0
#define COLOR_DARK_BLUE     0x0010
#define COLOR_GRAY          0x8410
#define COLOR_DARK_GRAY     0x4208

// Spectrum analyzer color palette (heat map)
static const uint16_t SPECTRUM_PALETTE[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
    0x041F, 0x081F, 0x0C1F, 0x101F, 0x141F, 0x181F, 0x1C1F, 0x201F,
    0x241F, 0x281F, 0x2C1F, 0x301F, 0x341F, 0x381F, 0x3C1F, 0x401F,
    0x441F, 0x481F, 0x4C1F, 0x501F, 0x541F, 0x581F, 0x5C1F, 0x601F,
    0x641F, 0x681F, 0x6C1F, 0x701F, 0x741F, 0x781F, 0x7C1F, 0x801F,
    0x841F, 0x881F, 0x8C1F, 0x901F, 0x941F, 0x981F, 0x9C1F, 0xA01F,
    0xA41F, 0xA81F, 0xAC1F, 0xB01F, 0xB41F, 0xB81F, 0xBC1F, 0xC01F,
    0xC41F, 0xC81F, 0xCC1F, 0xD01F, 0xD41F, 0xD81F, 0xDC1F, 0xE01F,
    0xE41F, 0xE81F, 0xEC1F, 0xF01F, 0xF41F, 0xF81F, 0xFC1F, 0xFC20,
    0xFC40, 0xFC60, 0xFC80, 0xFCA0, 0xFCC0, 0xFCE0, 0xFD00, 0xFD20,
    0xFD40, 0xFD60, 0xFD80, 0xFDA0, 0xFDC0, 0xFDE0, 0xFE00, 0xFE20,
    0xFE40, 0xFE60, 0xFE80, 0xFEA0, 0xFEC0, 0xFEE0, 0xFF00, 0xFF20,
    0xFF40, 0xFF60, 0xFF80, 0xFFA0, 0xFFC0, 0xFFE0, 0xFFE0, 0xFFE0,
    0xFFE1, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8,
    0xFFE9, 0xFFEA, 0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0,
    0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8,
    0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
};

//==============================================================================
// SPI Command/Data Helpers
//==============================================================================
static void display_send_cmd(uint8_t cmd) {
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
    };
    gpio_set_level(DISPLAY_DC_GPIO, 0);  // Command mode
    spi_device_transmit(s_display.spi, &trans);
}

static void display_send_data(uint8_t data) {
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &data,
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
    };
    gpio_set_level(DISPLAY_DC_GPIO, 1);  // Data mode
    spi_device_transmit(s_display.spi, &trans);
}

static void display_send_data16(uint16_t data) {
    uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) };
    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = buf,
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
    };
    gpio_set_level(DISPLAY_DC_GPIO, 1);  // Data mode
    spi_device_transmit(s_display.spi, &trans);
}

static void display_send_data_bulk(const uint8_t *data, uint32_t len) {
    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    gpio_set_level(DISPLAY_DC_GPIO, 1);  // Data mode
    spi_device_transmit(s_display.spi, &trans);
}

//==============================================================================
// Display Initialization
//==============================================================================
int display_init_driver(void) {
    ESP_LOGI(TAG, "Initializing display driver...");

    // Add SPI device
    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = DISPLAY_SPI_FREQ_HZ,
        .spics_io_num = DISPLAY_CS_GPIO,
        .queue_size = 2,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };

    esp_err_t ret = spi_bus_add_device(DISPLAY_SPI_HOST, &dev_cfg, &s_display.spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add display SPI device: %d", ret);
        return SPECTRE_ERR_IO;
    }

    // Hardware reset
    gpio_set_level(DISPLAY_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(DISPLAY_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Software reset
    display_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    display_send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Memory access control: RGB order, mirror X/Y
    display_send_cmd(0x36);
    display_send_data(0x60);  // RGB order, top-left origin

    // Pixel format: 18-bit (RGB666)
    display_send_cmd(0x3A);
    display_send_data(0x66);

    // Frame rate control
    display_send_cmd(0xB1);
    display_send_data(0x00);
    display_send_data(0x18);

    // Display inversion control
    display_send_cmd(0xB4);
    display_send_data(0x00);

    // Power control
    display_send_cmd(0xC0);
    display_send_data(0x23);

    display_send_cmd(0xC1);
    display_send_data(0x10);

    display_send_cmd(0xC5);
    display_send_data(0x3E);
    display_send_data(0x28);

    display_send_cmd(0xC7);
    display_send_data(0x86);

    // Display on
    display_send_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(50));

    s_display.initialized = true;

    // Clear screen
    display_clear(COLOR_BLACK);

    ESP_LOGI(TAG, "Display initialized (%dx%d)", s_display.width, s_display.height);
    return SPECTRE_OK;
}

//==============================================================================
// Basic Drawing Primitives
//==============================================================================
void display_set_rotation(uint8_t rotation) {
    s_display.rotation = rotation % 4;
    display_send_cmd(0x36);
    switch (rotation) {
        case 0: display_send_data(0x60); break;
        case 1: display_send_data(0xA0); break;
        case 2: display_send_data(0x00); break;
        case 3: display_send_data(0xC0); break;
    }
}

void display_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    display_send_cmd(0x2A);  // Column address set
    display_send_data16(x0);
    display_send_data16(x1);

    display_send_cmd(0x2B);  // Row address set
    display_send_data16(y0);
    display_send_data16(y1);

    display_send_cmd(0x2C);  // Memory write
}

void display_clear(uint16_t color) {
    display_set_addr_window(0, 0, s_display.width - 1, s_display.height - 1);

    uint32_t total_pixels = s_display.width * s_display.height;
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);

    // Fill in chunks
    uint8_t buffer[1024];
    for (uint32_t i = 0; i < 1024; i += 2) {
        buffer[i] = hi;
        buffer[i + 1] = lo;
    }

    uint32_t remaining = total_pixels * 2;
    while (remaining > 0) {
        uint32_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        display_send_data_bulk(buffer, chunk);
        remaining -= chunk;
    }
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= s_display.width || y >= s_display.height) return;
    display_set_addr_window(x, y, x, y);
    display_send_data16(color);
}

void display_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    if (y >= s_display.height) return;
    if (x + w > s_display.width) w = s_display.width - x;

    display_set_addr_window(x, y, x + w - 1, y);

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    uint8_t buffer[512];

    for (uint32_t i = 0; i < w * 2 && i < sizeof(buffer); i += 2) {
        buffer[i] = hi;
        buffer[i + 1] = lo;
    }

    uint32_t remaining = w * 2;
    while (remaining > 0) {
        uint32_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        display_send_data_bulk(buffer, chunk);
        remaining -= chunk;
    }
}

void display_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    if (x >= s_display.width) return;
    if (y + h > s_display.height) h = s_display.height - y;

    display_set_addr_window(x, y, x, y + h - 1);

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    uint8_t buffer[512];

    for (uint32_t i = 0; i < h * 2 && i < sizeof(buffer); i += 2) {
        buffer[i] = hi;
        buffer[i + 1] = lo;
    }

    uint32_t remaining = h * 2;
    while (remaining > 0) {
        uint32_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        display_send_data_bulk(buffer, chunk);
        remaining -= chunk;
    }
}

void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    for (uint16_t i = 0; i < h; i++) {
        display_draw_hline(x, y + i, w, color);
    }
}

void display_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    display_draw_pixel(x0, y0 + r, color);
    display_draw_pixel(x0, y0 - r, color);
    display_draw_pixel(x0 + r, y0, color);
    display_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        display_draw_pixel(x0 + x, y0 + y, color);
        display_draw_pixel(x0 - x, y0 + y, color);
        display_draw_pixel(x0 + x, y0 - y, color);
        display_draw_pixel(x0 - x, y0 - y, color);
        display_draw_pixel(x0 + y, y0 + x, color);
        display_draw_pixel(x0 - y, y0 + x, color);
        display_draw_pixel(x0 + y, y0 - x, color);
        display_draw_pixel(x0 - y, y0 - x, color);
    }
}

//==============================================================================
// Spectrum Visualization
//==============================================================================
void display_draw_spectrum(const uint16_t *fft_data, uint16_t num_bins,
                           uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint16_t max_val) {
    if (!fft_data || num_bins == 0) return;

    // Draw background
    display_draw_rect(x, y, w, h, COLOR_BLACK);

    // Draw grid lines
    for (uint16_t gy = y; gy < y + h; gy += h / 4) {
        display_draw_hline(x, gy, w, COLOR_DARK_GRAY);
    }

    // Draw FFT data as vertical bars
    for (uint16_t i = 0; i < num_bins && i < w; i++) {
        uint32_t val = fft_data[i];
        if (val > max_val) val = max_val;

        uint16_t bar_height = (uint16_t)((uint64_t)val * h / max_val);
        if (bar_height > h) bar_height = h;

        // Color based on amplitude (heat map)
        uint8_t intensity = (uint8_t)((uint64_t)val * 255 / max_val);
        uint16_t color = SPECTRUM_PALETTE[intensity];

        display_draw_vline(x + i, y + h - bar_height, bar_height, color);
    }
}

void display_draw_waterfall(const uint16_t *waterfall_data, uint16_t num_bins,
                            uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                            uint16_t max_val) {
    // Waterfall display: each row is one FFT frame, scrolling upward
    // (Simplified: draws the latest row at the bottom)
    if (!waterfall_data || num_bins == 0) return;

    for (uint16_t i = 0; i < num_bins && i < w; i++) {
        uint8_t intensity = (uint8_t)((uint64_t)waterfall_data[i] * 255 / max_val);
        display_draw_pixel(x + i, y + h - 1, SPECTRUM_PALETTE[intensity]);
    }
}

//==============================================================================
// Text Rendering (Simple 5x7 Font)
//==============================================================================
static const uint8_t FONT_5x7[96][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

void display_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg) {
    if (c < 0x20 || c > 0x7A) c = ' ';
    c -= 0x20;

    for (int i = 0; i < 5; i++) {
        uint8_t line = FONT_5x7[(uint8_t)c][i];
        for (int j = 0; j < 7; j++) {
            if (line & (1 << j)) {
                display_draw_pixel(x + i, y + j, color);
            } else {
                display_draw_pixel(x + i, y + j, bg);
            }
        }
    }
}

void display_draw_string(uint16_t x, uint16_t y, const char *str,
                         uint16_t color, uint16_t bg) {
    while (*str) {
        display_draw_char(x, y, *str, color, bg);
        x += 6;  // 5px char + 1px spacing
        if (x + 6 > s_display.width) {
            x = 0;
            y += 8;
        }
        str++;
    }
}

void display_printf(uint16_t x, uint16_t y, uint16_t color, uint16_t bg,
                    const char *fmt, ...) {
    char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    display_draw_string(x, y, buffer, color, bg);
}

//==============================================================================
// Status Bar
//==============================================================================
void display_draw_status_bar(const char *mode_name, uint8_t battery_pct,
                              float freq_mhz, bool recording) {
    // Draw top status bar
    display_draw_rect(0, 0, s_display.width, 12, COLOR_DARK_BLUE);

    // Mode name
    display_draw_string(2, 2, mode_name, COLOR_WHITE, COLOR_DARK_BLUE);

    // Frequency
    char freq_str[16];
    snprintf(freq_str, sizeof(freq_str), "%.1f MHz", freq_mhz);
    display_draw_string(100, 2, freq_str, COLOR_CYAN, COLOR_DARK_BLUE);

    // Battery
    char batt_str[8];
    snprintf(batt_str, sizeof(batt_str), "%d%%", battery_pct);
    display_draw_string(260, 2, batt_str,
                        battery_pct > 20 ? COLOR_GREEN : COLOR_RED,
                        COLOR_DARK_BLUE);

    // Recording indicator
    if (recording) {
        display_draw_circle(310, 6, 3, COLOR_RED);
    }
}
