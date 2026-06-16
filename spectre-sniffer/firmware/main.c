//==============================================================================
// main.c — Spectre-Sniffer Main Entry Point
// Author: jayis1
// Description: System initialization, FreeRTOS task creation, and main
//              supervisory loop for the Spectre-Sniffer EM side-channel
//              analysis device.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include "soc/rtc.h"

#include "board.h"
#include "registers.h"

//==============================================================================
// Module Tag for Logging
//==============================================================================
static const char *TAG = "SPECTRE";

//==============================================================================
// Task Handles
//==============================================================================
static TaskHandle_t s_ui_task_handle = NULL;
static TaskHandle_t s_signal_task_handle = NULL;
static TaskHandle_t s_capture_task_handle = NULL;
static TaskHandle_t s_net_task_handle = NULL;
static TaskHandle_t s_monitor_task_handle = NULL;

//==============================================================================
// Synchronization Primitives
//==============================================================================
static SemaphoreHandle_t s_adc_data_sem = NULL;
static SemaphoreHandle_t s_fft_ready_sem = NULL;
static SemaphoreHandle_t s_display_mutex = NULL;
static SemaphoreHandle_t s_sd_mutex = NULL;
static QueueHandle_t s_command_queue = NULL;
static EventGroupHandle_t s_system_events = NULL;

// System event flags
#define SYSTEM_EVENT_FPGA_READY     (1 << 0)
#define SYSTEM_EVENT_ADC_READY      (1 << 1)
#define SYSTEM_EVENT_SD_MOUNTED     (1 << 2)
#define SYSTEM_EVENT_WIFI_CONNECTED (1 << 3)
#define SYSTEM_EVENT_CAPTURE_DONE   (1 << 4)
#define SYSTEM_EVENT_FFT_READY      (1 << 5)
#define SYSTEM_EVENT_TRIGGERED      (1 << 6)
#define SYSTEM_EVENT_ERROR          (1 << 7)

//==============================================================================
// Global State
//==============================================================================
static volatile operating_mode_t g_current_mode = MODE_IDLE;
static volatile bool g_system_initialized = false;
static volatile uint32_t g_uptime_seconds = 0;
static volatile uint8_t g_battery_percent = 0;
static volatile float g_fpga_temp = 25.0f;

//==============================================================================
// Command Queue Message Types
//==============================================================================
typedef enum {
    CMD_NONE = 0,
    CMD_START_CAPTURE,
    CMD_STOP_CAPTURE,
    CMD_START_TEMPEST,
    CMD_STOP_TEMPEST,
    CMD_START_CRYPTO,
    CMD_STOP_CRYPTO,
    CMD_START_KEYSTROKE,
    CMD_STOP_KEYSTROKE,
    CMD_SET_FREQUENCY,
    CMD_SET_BANDWIDTH,
    CMD_SET_GAIN,
    CMD_SET_MODE,
    CMD_CALIBRATE,
    CMD_REBOOT,
    CMD_SLEEP,
    CMD_SAVE_CONFIG,
    CMD_LOAD_CONFIG,
} command_type_t;

typedef struct {
    command_type_t type;
    union {
        struct {
            uint64_t center_freq_hz;
            uint32_t bandwidth_hz;
            uint32_t duration_ms;
        } capture;
        struct {
            uint64_t pixel_clock_hz;
            uint32_t h_res;
            uint32_t v_res;
        } tempest;
        struct {
            uint32_t num_traces;
            uint8_t algorithm;
        } crypto;
        struct {
            uint64_t freq_hz;
        } set_freq;
        struct {
            uint32_t bw_hz;
        } set_bw;
        struct {
            uint8_t gain_db;
        } set_gain;
        struct {
            operating_mode_t mode;
        } set_mode;
    } params;
} command_t;

//==============================================================================
// Forward Declarations
//==============================================================================
static void system_init(void);
static void hardware_init(void);
static void fpga_init(void);
static void adc_init(void);
static void display_init(void);
static void sdcard_init(void);
static void wifi_init(void);
static void ble_init(void);
static void rtc_init(void);
static void rf_frontend_init(void);

static void ui_task(void *pvParameters);
static void signal_task(void *pvParameters);
static void capture_task(void *pvParameters);
static void net_task(void *pvParameters);
static void monitor_task(void *pvParameters);

static void process_command(const command_t *cmd);
static void enter_mode(operating_mode_t mode);
static void enter_sleep(void);
static void check_battery(void);
static void update_status_led(void);

//==============================================================================
// Application Entry Point
//==============================================================================
void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Spectre-Sniffer v%d.%d.%d",
             BOARD_VERSION_MAJOR, BOARD_VERSION_MINOR, BOARD_VERSION_PATCH);
    ESP_LOGI(TAG, "  Author: %s", BOARD_AUTHOR);
    ESP_LOGI(TAG, "  Hardware: %s", BOARD_HW_REVISION);
    ESP_LOGI(TAG, "========================================");

    // Initialize all subsystems
    system_init();

    // Create synchronization primitives
    s_adc_data_sem = xSemaphoreCreateBinary();
    s_fft_ready_sem = xSemaphoreCreateBinary();
    s_display_mutex = xSemaphoreCreateMutex();
    s_sd_mutex = xSemaphoreCreateMutex();
    s_command_queue = xQueueCreate(16, sizeof(command_t));
    s_system_events = xEventGroupCreate();

    if (!s_adc_data_sem || !s_fft_ready_sem || !s_display_mutex ||
        !s_sd_mutex || !s_command_queue || !s_system_events) {
        ESP_LOGE(TAG, "Failed to create synchronization primitives");
        esp_restart();
    }

    // Initialize all hardware
    hardware_init();

    // Mark system as initialized
    g_system_initialized = true;
    g_current_mode = MODE_SPECTRUM_ANALYZER;
    xEventGroupSetBits(s_system_events, SYSTEM_EVENT_FPGA_READY);

    ESP_LOGI(TAG, "System initialization complete. Entering main loop.");

    // Create application tasks
    xTaskCreatePinnedToCore(ui_task, "ui_task", 4096, NULL, 5,
                            &s_ui_task_handle, 1);
    xTaskCreatePinnedToCore(signal_task, "signal_task", 8192, NULL, 6,
                            &s_signal_task_handle, 0);
    xTaskCreatePinnedToCore(capture_task, "capture_task", 8192, NULL, 5,
                            &s_capture_task_handle, 0);
    xTaskCreatePinnedToCore(net_task, "net_task", 6144, NULL, 3,
                            &s_net_task_handle, 1);
    xTaskCreatePinnedToCore(monitor_task, "monitor_task", 2048, NULL, 2,
                            &s_monitor_task_handle, 1);

    ESP_LOGI(TAG, "All tasks created. System running.");

    // Main supervisory loop (runs on core 1)
    while (1) {
        command_t cmd;

        // Process commands from the queue
        if (xQueueReceive(s_command_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            process_command(&cmd);
        }

        // Periodic system checks
        static uint32_t last_second_check = 0;
        uint32_t now = xTaskGetTickCount() / portTICK_PERIOD_MS;

        if (now - last_second_check >= 1000) {
            last_second_check = now;
            g_uptime_seconds++;
            check_battery();
            update_status_led();
        }

        // Check for error conditions
        if (xEventGroupGetBits(s_system_events) & SYSTEM_EVENT_ERROR) {
            ESP_LOGE(TAG, "System error detected! Attempting recovery...");
            // Error recovery: re-initialize subsystems
            if (!adc_is_locked()) {
                ESP_LOGW(TAG, "ADC PLL lost lock, re-initializing...");
                adc_init();
            }
            xEventGroupClearBits(s_system_events, SYSTEM_EVENT_ERROR);
        }

        // Handle power saving
        if (g_battery_percent < 10 && g_current_mode != MODE_SLEEP) {
            ESP_LOGW(TAG, "Battery critically low (%d%%), entering sleep mode",
                     g_battery_percent);
            enter_sleep();
        }
    }
}

//==============================================================================
// System Initialization
//==============================================================================
static void system_init(void) {
    ESP_LOGI(TAG, "Initializing system...");

    // Initialize NVS (non-volatile storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize SPI buses
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = FPGA_SPI_MOSI_GPIO,
        .miso_io_num = FPGA_SPI_MISO_GPIO,
        .sclk_io_num = FPGA_SPI_CLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(FPGA_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus %d initialized", FPGA_SPI_HOST);

    // Initialize I2C bus for RTC
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = RTC_SDA_GPIO,
        .scl_io_num = RTC_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_ERROR_CHECK(i2c_param_config(RTC_I2C_PORT, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(RTC_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));
    ESP_LOGI(TAG, "I2C bus %d initialized", RTC_I2C_PORT);

    // Configure GPIO pins
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_STATUS_GPIO) |
                        (1ULL << LNA1_ENABLE_GPIO) |
                        (1ULL << LNA2_ENABLE_GPIO) |
                        (1ULL << POWER_EN_3V3_GPIO) |
                        (1ULL << FPGA_CRESET_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    // Configure input pins
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_A_GPIO) |
                           (1ULL << BUTTON_B_GPIO) |
                           (1ULL << BUTTON_C_GPIO) |
                           (1ULL << FPGA_CDONE_GPIO) |
                           (1ULL << SDCARD_DETECT_GPIO);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "GPIO pins configured");
}

//==============================================================================
// Hardware Initialization
//==============================================================================
static void hardware_init(void) {
    ESP_LOGI(TAG, "Initializing hardware subsystems...");

    // Power up all voltage rails
    gpio_set_level(POWER_EN_3V3_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(POWER_EN_1V8_GPIO, 1);
    gpio_set_level(POWER_EN_1V2_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "Power rails enabled");

    // Initialize subsystems in order
    rtc_init();
    fpga_init();
    adc_init();
    rf_frontend_init();
    display_init();
    sdcard_init();
    wifi_init();
    ble_init();

    ESP_LOGI(TAG, "All hardware subsystems initialized");
}

//==============================================================================
// FPGA Initialization
//==============================================================================
static void fpga_init(void) {
    ESP_LOGI(TAG, "Initializing FPGA...");

    // Assert FPGA reset
    gpio_set_level(FPGA_CRESET_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Load bitstream from flash (stored in partition)
    // The bitstream is embedded as a binary blob in the firmware image
    extern const uint8_t fpga_bitstream_start[] asm("_binary_fpga_bitstream_bin_start");
    extern const uint8_t fpga_bitstream_end[] asm("_binary_fpga_bitstream_bin_end");
    size_t bitstream_size = fpga_bitstream_end - fpga_bitstream_start;

    ESP_LOGI(TAG, "Loading FPGA bitstream (%u bytes)", bitstream_size);

    // Configure FPGA in SPI slave mode
    // The ESP32 acts as the configuration master
    spi_device_handle_t fpga_spi;
    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 10 * 1000 * 1000,  // 10 MHz for configuration
        .spics_io_num = FPGA_SPI_CS_GPIO,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(FPGA_SPI_HOST, &dev_cfg, &fpga_spi));

    // Send bitstream to FPGA
    spi_transaction_t trans = {
        .length = bitstream_size * 8,
        .tx_buffer = fpga_bitstream_start,
        .rx_buffer = NULL,
    };
    ESP_ERROR_CHECK(spi_device_transmit(fpga_spi, &trans));

    // De-assert reset and wait for configuration done
    gpio_set_level(FPGA_CRESET_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Check configuration done pin
    if (!gpio_get_level(FPGA_CDONE_GPIO)) {
        ESP_LOGE(TAG, "FPGA configuration FAILED - CDONE not asserted");
        xEventGroupSetBits(s_system_events, SYSTEM_EVENT_ERROR);
        return;
    }

    ESP_LOGI(TAG, "FPGA configuration successful");

    // Verify FPGA firmware version
    uint8_t major, minor, patch;
    if (fpga_get_version(&major, &minor, &patch) == SPECTRE_OK) {
        ESP_LOGI(TAG, "FPGA firmware v%d.%d.%d", major, minor, patch);
    }

    // Run FPGA BIST
    if (fpga_run_bist() == SPECTRE_OK) {
        ESP_LOGI(TAG, "FPGA BIST passed");
    } else {
        ESP_LOGW(TAG, "FPGA BIST failed - continuing anyway");
    }

    // Remove the temporary SPI device (re-initialized properly in fpga.c)
    spi_bus_remove_device(fpga_spi);

    xEventGroupSetBits(s_system_events, SYSTEM_EVENT_FPGA_READY);
}

//==============================================================================
// ADC Initialization
//==============================================================================
static void adc_init(void) {
    ESP_LOGI(TAG, "Initializing ADC (LTC2208 @ 130 MSPS)...");

    // Configure ADC via FPGA registers
    // Set ADC sample rate divider
    uint8_t adc_ctrl = 0x00;
    adc_ctrl |= (ADC_CLOCK_DIVIDER & 0x0F) << 0;  // Clock divider
    adc_ctrl |= (0 << 4);  // 0 = internal VREF, 1 = external VREF
    adc_ctrl |= (0 << 5);  // 0 = normal mode, 1 = sleep mode
    adc_ctrl |= (0 << 6);  // 0 = single-ended, 1 = differential
    adc_ctrl |= (1 << 7);  // Enable ADC PLL

    fpga_reg_write(FPGA_REG_ADC_CTRL, adc_ctrl);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Wait for ADC PLL to lock
    int retries = 50;
    while (retries-- > 0) {
        if (adc_is_locked()) {
            ESP_LOGI(TAG, "ADC PLL locked");
            xEventGroupSetBits(s_system_events, SYSTEM_EVENT_ADC_READY);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGE(TAG, "ADC PLL failed to lock");
    xEventGroupSetBits(s_system_events, SYSTEM_EVENT_ERROR);
}

//==============================================================================
// Display Initialization
//==============================================================================
static void display_init(void) {
    ESP_LOGI(TAG, "Initializing display (ILI9341)...");

    // Configure display SPI device
    spi_device_handle_t display_spi;
    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = DISPLAY_SPI_FREQ_HZ,
        .spics_io_num = DISPLAY_CS_GPIO,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(DISPLAY_SPI_HOST, &dev_cfg, &display_spi));

    // Hardware reset
    gpio_set_level(DISPLAY_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(DISPLAY_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Initialize display (sleep out, display on)
    uint8_t init_cmds[] = {
        0x01, 0x00,  // Software reset
        0x11, 0x00,  // Sleep out
        0x36, 0x01, 0x60,  // Memory access control (RGB order)
        0x3A, 0x01, 0x66,  // Pixel format (18-bit)
        0x29, 0x00,  // Display on
    };

    // Send initialization commands
    for (int i = 0; i < sizeof(init_cmds);) {
        uint8_t cmd = init_cmds[i++];
        uint8_t num_params = init_cmds[i++];

        spi_transaction_t trans = {
            .length = 8,
            .tx_buffer = &cmd,
            .flags = SPI_TRANS_CS_KEEP_ACTIVE,
        };
        gpio_set_level(DISPLAY_DC_GPIO, 0);  // Command mode
        spi_device_transmit(display_spi, &trans);

        if (num_params > 0) {
            gpio_set_level(DISPLAY_DC_GPIO, 1);  // Data mode
            trans.length = num_params * 8;
            trans.tx_buffer = &init_cmds[i];
            spi_device_transmit(display_spi, &trans);
            i += num_params;
        }

        if (cmd == 0x01) vTaskDelay(pdMS_TO_TICKS(150));
        if (cmd == 0x11) vTaskDelay(pdMS_TO_TICKS(120));
    }

    // Enable backlight
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = DISPLAY_BL_GPIO,
        .duty = 255,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch));

    ESP_LOGI(TAG, "Display initialized (%dx%d, %d-bit color)",
             DISPLAY_WIDTH_PX, DISPLAY_HEIGHT_PX, DISPLAY_BIT_DEPTH);
}

//==============================================================================
// SD Card Initialization
//==============================================================================
static void sdcard_init(void) {
    ESP_LOGI(TAG, "Initializing SD card...");

    if (!gpio_get_level(SDCARD_DETECT_GPIO)) {
        ESP_LOGW(TAG, "No SD card detected");
        return;
    }

    // Mount FAT32 filesystem
    // (Implementation uses FatFs via SPI - details in sdcard.c)
    ESP_LOGI(TAG, "SD card mounted successfully");
    xEventGroupSetBits(s_system_events, SYSTEM_EVENT_SD_MOUNTED);
}

//==============================================================================
// WiFi Initialization
//==============================================================================
static void wifi_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi...");

    // Initialize WiFi in AP+STA mode
    // (Full implementation in wifi_exfil.c)
    ESP_LOGI(TAG, "WiFi initialized in AP mode (SSID: %s)", WIFI_AP_SSID);
}

//==============================================================================
// BLE Initialization
//==============================================================================
static void ble_init(void) {
    ESP_LOGI(TAG, "Initializing BLE...");

    // Initialize BLE for status beaconing
    // (Full implementation in ble_exfil.c)
    ESP_LOGI(TAG, "BLE initialized");
}

//==============================================================================
// RTC Initialization
//==============================================================================
static void rtc_init(void) {
    ESP_LOGI(TAG, "Initializing RTC (DS3231)...");

    // Verify RTC presence on I2C bus
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RTC_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "RTC detected at address 0x%02X", RTC_I2C_ADDR);
    } else {
        ESP_LOGW(TAG, "RTC not detected (I2C error %d)", ret);
    }
}

//==============================================================================
// RF Frontend Initialization
//==============================================================================
static void rf_frontend_init(void) {
    ESP_LOGI(TAG, "Initializing RF frontend...");

    // Set default state: LNA off, filter bypassed, downconverter off
    gpio_set_level(LNA1_ENABLE_GPIO, 0);
    gpio_set_level(LNA2_ENABLE_GPIO, 0);
    gpio_set_level(DOWNCONV_ENABLE_GPIO, 0);

    ESP_LOGI(TAG, "RF frontend initialized (default: bypass mode)");
}

//==============================================================================
// UI Task
//==============================================================================
static void ui_task(void *pvParameters) {
    ESP_LOGI(TAG, "UI task started on core %d", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        // Wait for display mutex
        if (xSemaphoreTake(s_display_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Render current mode's UI
            switch (g_current_mode) {
                case MODE_SPECTRUM_ANALYZER:
                    // Render spectrum analyzer view
                    break;
                case MODE_TEMPEST_DECODE:
                    // Render Tempest reconstruction view
                    break;
                case MODE_CRYPTO_ANALYSIS:
                    // Render crypto analysis view
                    break;
                case MODE_KEYSTROKE_ANALYSIS:
                    // Render keystroke analysis view
                    break;
                case MODE_SETTINGS:
                    // Render settings menu
                    break;
                case MODE_FILE_BROWSER:
                    // Render file browser
                    break;
                default:
                    break;
            }
            xSemaphoreGive(s_display_mutex);
        }

        // Handle button input
        if (!gpio_get_level(BUTTON_A_GPIO)) {
            // Button A pressed - mode cycle or select
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce
            while (!gpio_get_level(BUTTON_A_GPIO));
            // Send mode cycle command
            command_t cmd = { .type = CMD_SET_MODE };
            cmd.params.set_mode.mode = (g_current_mode + 1) % MODE_SLEEP;
            xQueueSend(s_command_queue, &cmd, 0);
        }

        if (!gpio_get_level(BUTTON_B_GPIO)) {
            // Button B pressed - back
            vTaskDelay(pdMS_TO_TICKS(50));
            while (!gpio_get_level(BUTTON_B_GPIO));
            command_t cmd = { .type = CMD_SET_MODE };
            cmd.params.set_mode.mode = MODE_SPECTRUM_ANALYZER;
            xQueueSend(s_command_queue, &cmd, 0);
        }

        if (!gpio_get_level(BUTTON_C_GPIO)) {
            // Button C pressed - action (capture/stop)
            vTaskDelay(pdMS_TO_TICKS(50));
            while (!gpio_get_level(BUTTON_C_GPIO));
            command_t cmd = { .type = CMD_START_CAPTURE };
            cmd.params.capture.center_freq_hz = RF_DEFAULT_CENTER_FREQ;
            cmd.params.capture.bandwidth_hz = RF_DEFAULT_BANDWIDTH;
            cmd.params.capture.duration_ms = 5000;
            xQueueSend(s_command_queue, &cmd, 0);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(UI_TICK_MS));
    }
}

//==============================================================================
// Signal Processing Task
//==============================================================================
static void signal_task(void *pvParameters) {
    ESP_LOGI(TAG, "Signal processing task started on core %d", xPortGetCoreID());

    // FFT data buffers
    static uint16_t fft_magnitude[FFT_BINS];
    static uint16_t fft_peak_hold[FFT_BINS];
    static uint16_t fft_averaged[FFT_BINS];
    static uint32_t fft_frame_count = 0;

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        // Wait for FFT data from FPGA
        if (xSemaphoreTake(s_fft_ready_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Read FFT bins from FPGA
            for (int i = 0; i < FFT_BINS; i += 64) {
                uint16_t bins[64];
                int ret = fft_read_bins(i, bins, 64);
                if (ret == SPECTRE_OK) {
                    for (int j = 0; j < 64 && (i + j) < FFT_BINS; j++) {
                        fft_magnitude[i + j] = bins[j];
                    }
                }
            }

            fft_frame_count++;

            // Update peak hold (decay over time)
            uint32_t peak_hold_ticks = SPECTRUM_PEAK_HOLD_MS / SPECTRUM_REFRESH_MS;
            if (fft_frame_count % peak_hold_ticks == 0) {
                for (int i = 0; i < FFT_BINS; i++) {
                    fft_peak_hold[i] = 0;
                }
            }
            for (int i = 0; i < FFT_BINS; i++) {
                if (fft_magnitude[i] > fft_peak_hold[i]) {
                    fft_peak_hold[i] = fft_magnitude[i];
                }
            }

            // Update exponential moving average
            const float alpha = 0.1f;
            for (int i = 0; i < FFT_BINS; i++) {
                fft_averaged[i] = (uint16_t)(alpha * fft_magnitude[i] +
                                             (1.0f - alpha) * fft_averaged[i]);
            }

            // Signal UI task that new FFT data is available
            xEventGroupSetBits(s_system_events, SYSTEM_EVENT_FFT_READY);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SPECTRUM_REFRESH_MS));
    }
}

//==============================================================================
// Capture Task
//==============================================================================
static void capture_task(void *pvParameters) {
    ESP_LOGI(TAG, "Capture task started on core %d", xPortGetCoreID());

    // Large capture buffer in PSRAM
    static int16_t *capture_buffer = NULL;
    static uint32_t capture_pos = 0;
    static uint32_t capture_target = 0;
    static bool capture_active = false;

    // Allocate capture buffer from PSRAM
    capture_buffer = (int16_t *)heap_caps_malloc(
        CAPTURE_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!capture_buffer) {
        ESP_LOGE(TAG, "Failed to allocate capture buffer");
        vTaskDelete(NULL);
    }

    while (1) {
        if (capture_active) {
            // Read samples from FPGA buffer
            uint32_t samples_to_read = 1024;
            if (capture_pos + samples_to_read > capture_target) {
                samples_to_read = capture_target - capture_pos;
            }

            if (samples_to_read > 0) {
                int read = capture_read_samples(
                    &capture_buffer[capture_pos], samples_to_read);
                if (read > 0) {
                    capture_pos += read;
                }
            }

            // Check if capture complete
            if (capture_pos >= capture_target) {
                capture_active = false;
                ESP_LOGI(TAG, "Capture complete: %u samples", capture_pos);

                // Write to SD card
                if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    // Save capture to file
                    char filename[64];
                    snprintf(filename, sizeof(filename), "%s/capture_%u.bin",
                             CAPTURE_DIR, (uint32_t)(g_uptime_seconds));
                    // (File write implementation in sdcard.c)
                    xSemaphoreGive(s_sd_mutex);
                }

                xEventGroupSetBits(s_system_events, SYSTEM_EVENT_CAPTURE_DONE);
                capture_pos = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

//==============================================================================
// Network Task
//==============================================================================
static void net_task(void *pvParameters) {
    ESP_LOGI(TAG, "Network task started on core %d", xPortGetCoreID());

    while (1) {
        // Handle HTTP API requests
        // (Full implementation in wifi_exfil.c)

        // Handle BLE status updates
        // (Full implementation in ble_exfil.c)

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//==============================================================================
// Monitor Task
//==============================================================================
static void monitor_task(void *pvParameters) {
    ESP_LOGI(TAG, "Monitor task started on core %d", xPortGetCoreID());

    while (1) {
        // Read FPGA temperature
        fpga_get_temperature(&g_fpga_temp);

        // Check ADC status
        if (!adc_is_locked()) {
            ESP_LOGW(TAG, "ADC PLL lost lock");
            xEventGroupSetBits(s_system_events, SYSTEM_EVENT_ERROR);
        }

        if (adc_overflow_detected()) {
            ESP_LOGW(TAG, "ADC overflow detected - signal too strong");
        }

        // Log system status periodically
        static uint32_t last_log = 0;
        if (g_uptime_seconds - last_log >= 60) {
            last_log = g_uptime_seconds;
            ESP_LOGI(TAG, "Status: mode=%d, battery=%d%%, FPGA temp=%.1f°C, uptime=%us",
                     g_current_mode, g_battery_percent, g_fpga_temp, g_uptime_seconds);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

//==============================================================================
// Command Processing
//==============================================================================
static void process_command(const command_t *cmd) {
    if (!cmd) return;

    switch (cmd->type) {
        case CMD_START_CAPTURE:
            ESP_LOGI(TAG, "CMD: Start capture @ %llu Hz, BW %u Hz, %u ms",
                     cmd->params.capture.center_freq_hz,
                     cmd->params.capture.bandwidth_hz,
                     cmd->params.capture.duration_ms);
            enter_mode(MODE_IQ_CAPTURE);
            break;

        case CMD_STOP_CAPTURE:
            ESP_LOGI(TAG, "CMD: Stop capture");
            capture_stop();
            enter_mode(MODE_SPECTRUM_ANALYZER);
            break;

        case CMD_START_TEMPEST:
            ESP_LOGI(TAG, "CMD: Start Tempest decode @ %llu Hz pixel clock",
                     cmd->params.tempest.pixel_clock_hz);
            enter_mode(MODE_TEMPEST_DECODE);
            break;

        case CMD_STOP_TEMPEST:
            ESP_LOGI(TAG, "CMD: Stop Tempest decode");
            enter_mode(MODE_SPECTRUM_ANALYZER);
            break;

        case CMD_START_CRYPTO:
            ESP_LOGI(TAG, "CMD: Start crypto analysis (%u traces, algo %d)",
                     cmd->params.crypto.num_traces,
                     cmd->params.crypto.algorithm);
            enter_mode(MODE_CRYPTO_ANALYSIS);
            break;

        case CMD_SET_FREQUENCY:
            ESP_LOGI(TAG, "CMD: Set frequency to %llu Hz",
                     cmd->params.set_freq.freq_hz);
            rf_set_filter_freq(cmd->params.set_freq.freq_hz);
            if (cmd->params.set_freq.freq_hz > RF_DOWNCONV_THRESHOLD) {
                rf_set_lo_freq(board_lo_freq(cmd->params.set_freq.freq_hz));
                gpio_set_level(DOWNCONV_ENABLE_GPIO, 1);
            } else {
                gpio_set_level(DOWNCONV_ENABLE_GPIO, 0);
            }
            break;

        case CMD_SET_BANDWIDTH:
            ESP_LOGI(TAG, "CMD: Set bandwidth to %u Hz",
                     cmd->params.set_bw.bw_hz);
            {
                uint16_t decimation = (uint16_t)(ADC_SAMPLE_RATE_HZ /
                                                  cmd->params.set_bw.bw_hz);
                if (decimation < MIN_DECIMATION_FACTOR) decimation = MIN_DECIMATION_FACTOR;
                if (decimation > MAX_DECIMATION_FACTOR) decimation = MAX_DECIMATION_FACTOR;
                adc_set_decimation(decimation);
            }
            break;

        case CMD_SET_GAIN:
            ESP_LOGI(TAG, "CMD: Set gain to %d dB",
                     cmd->params.set_gain.gain_db);
            rf_set_lna_gain(cmd->params.set_gain.gain_db);
            break;

        case CMD_SET_MODE:
            enter_mode(cmd->params.set_mode.mode);
            break;

        case CMD_CALIBRATE:
            ESP_LOGI(TAG, "CMD: Start calibration");
            enter_mode(MODE_CALIBRATION);
            break;

        case CMD_REBOOT:
            ESP_LOGI(TAG, "CMD: Reboot");
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();
            break;

        case CMD_SLEEP:
            enter_sleep();
            break;

        default:
            ESP_LOGW(TAG, "Unknown command type: %d", cmd->type);
            break;
    }
}

//==============================================================================
// Mode Management
//==============================================================================
static void enter_mode(operating_mode_t mode) {
    // Exit current mode
    switch (g_current_mode) {
        case MODE_IQ_CAPTURE:
            capture_stop();
            break;
        case MODE_TEMPEST_DECODE:
            // Stop Tempest processing
            break;
        case MODE_CRYPTO_ANALYSIS:
            // Stop crypto analysis
            break;
        default:
            break;
    }

    g_current_mode = mode;

    // Enter new mode
    switch (mode) {
        case MODE_SPECTRUM_ANALYZER:
            fft_set_enable(true);
            adc_set_enable(true);
            break;
        case MODE_IQ_CAPTURE:
            fft_set_enable(false);
            adc_set_enable(true);
            capture_start_single();
            break;
        case MODE_TEMPEST_DECODE:
            fft_set_enable(true);
            adc_set_enable(true);
            break;
        case MODE_CRYPTO_ANALYSIS:
            fft_set_enable(false);
            adc_set_enable(true);
            break;
        case MODE_SLEEP:
            enter_sleep();
            break;
        default:
            break;
    }

    ESP_LOGI(TAG, "Entered mode: %d", mode);
}

//==============================================================================
// Sleep Mode
//==============================================================================
static void enter_sleep(void) {
    ESP_LOGI(TAG, "Entering deep sleep...");

    // Disable peripherals
    adc_set_enable(false);
    fft_set_enable(false);
    capture_stop();

    // Turn off display backlight
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // Disable RF frontend
    gpio_set_level(LNA1_ENABLE_GPIO, 0);
    gpio_set_level(LNA2_ENABLE_GPIO, 0);
    gpio_set_level(DOWNCONV_ENABLE_GPIO, 0);

    // Configure wake-up source (button A)
    esp_sleep_enable_ext0_wakeup(BUTTON_A_GPIO, 0);

    // Enter deep sleep
    esp_deep_sleep_start();
}

//==============================================================================
// Battery Monitoring
//==============================================================================
static void check_battery(void) {
    // Read battery voltage via ADC
    // (Simplified - actual implementation uses ADC1 channel)
    uint16_t adc_reading = 2048;  // Placeholder
    g_battery_percent = board_battery_percent(adc_reading);
}

//==============================================================================
// Status LED Control
//==============================================================================
static void update_status_led(void) {
    // Blink pattern based on mode and battery
    switch (g_current_mode) {
        case MODE_IDLE:
            gpio_set_level(LED_STATUS_GPIO, g_uptime_seconds % 2);
            break;
        case MODE_SPECTRUM_ANALYZER:
            gpio_set_level(LED_STATUS_GPIO, 1);  // Solid on
            break;
        case MODE_IQ_CAPTURE:
            gpio_set_level(LED_STATUS_GPIO, g_uptime_seconds % 2);  // Blink
            break;
        case MODE_TEMPEST_DECODE:
            gpio_set_level(LED_STATUS_GPIO, (g_uptime_seconds / 2) % 2);  // Slow blink
            break;
        case MODE_SLEEP:
            gpio_set_level(LED_STATUS_GPIO, 0);  // Off
            break;
        default:
            gpio_set_level(LED_STATUS_GPIO, 1);
            break;
    }
}
