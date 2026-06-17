/**
 * @file    main.c
 * @brief   HDMI-Siphon ESP32-S3 main firmware — transparent HDMI interposer
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Main entry point for the HDMI-Siphon firmware. Initializes all hardware
 * subsystems, starts the FPGA configuration, begins WiFi/BLE services,
 * and enters the main event loop.
 *
 * Architecture:
 *   - FreeRTOS task-based scheduler
 *   - FPGA handles real-time pixel processing (SDRAM buffering, OSD, CEC)
 *   - ESP32-S3 handles control plane: WiFi, BLE, SD card, capture management
 *   - JSON wire protocol over WebSocket and BLE GATT
 *
 * Tasks:
 *   - main_task:         System initialization and health monitoring
 *   - wifi_task:         WiFi AP/STA management and HTTP/WebSocket server
 *   - capture_task:      Frame capture trigger and data transfer orchestration
 *   - cec_task:          CEC bus monitoring and command injection
 *   - battery_task:      Battery voltage monitoring and shutdown
 *   - ble_task:          BLE GATT service advertiser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_types.h"

#include "board.h"
#include "registers.h"
#include "spi_fpga.h"
#include "sd_card.h"
#include "capture_ctrl.h"
#include "cec_monitor.h"
#include "edid_manager.h"
#include "wifi_manager.h"
#include "ble_gatt.h"
#include "protocol.h"
#include "battery.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

/** Tag for ESP-IDF logging */
static const char *TAG = "HDMI-SIPHON";

/** Stack sizes for FreeRTOS tasks (bytes) */
#define TASK_STACK_MAIN              4096U
#define TASK_STACK_WIFI              8192U
#define TASK_STACK_CAPTURE           4096U
#define TASK_STACK_CEC               3072U
#define TASK_STACK_BATTERY           2048U
#define TASK_STACK_BLE               4096U

/** Task priorities */
#define PRIORITY_MAIN                 tskIDLE_PRIORITY + 5
#define PRIORITY_WIFI                 tskIDLE_PRIORITY + 4
#define PRIORITY_CAPTURE              tskIDLE_PRIORITY + 3
#define PRIORITY_CEC                  tskIDLE_PRIORITY + 2
#define PRIORITY_BATTERY              tskIDLE_PRIORITY + 1
#define PRIORITY_BLE                  tskIDLE_PRIORITY + 4

/** Main loop interval (ms) */
#define MAIN_LOOP_INTERVAL_MS             1000U

/** FPGA configuration timeout (ms) */
#define FPGA_CONFIG_TIMEOUT_MS            5000U

/** NVS namespace for device configuration */
#define NVS_NAMESPACE                  "hdmi_siphon"

/* =========================================================================
 * Global State
 * ========================================================================= */

/** Device operational mode */
typedef enum {
    MODE_PASSTHROUGH = 0,  /**< Transparent passthrough (default) */
    MODE_CAPTURE,          /**< Frame capture mode */
    MODE_COVERT,           /**< Covert capture mode (all LEDs off) */
    MODE_OSD,              /**< OSD overlay injection mode */
    MODE_CEC_MONITOR,      /**< CEC bus monitoring mode */
    MODE_BLANK             /**< Blank display (stealth disconnect) */
} device_mode_t;

/** Global device state structure */
typedef struct {
    device_mode_t   mode;           /**< Current operational mode */
    bool            fpga_ready;     /**< FPGA configured and ready */
    bool            hdmi_locked;    /**< HDMI RX signal locked */
    bool            display_connected; /**< Display detected on HDMI OUT */
    bool            hdcp_active;    /**< HDCP handshake active */
    bool            battery_critical; /**< Battery at critical level */
    bool            stealth_mode;   /**< All LEDs disabled */
    uint32_t        frame_count;    /**< Total frames captured */
    uint32_t        uptime_seconds; /**< Device uptime in seconds */
    uint16_t        h_active;       /**< Current horizontal active pixels */
    uint16_t        v_active;       /**< Current vertical active lines */
    uint32_t        pclk_khz;       /**< Current pixel clock in kHz */
    uint16_t        refresh_hz_x100;/**< Refresh rate * 100 */
    uint32_t        batt_mv;        /**< Current battery voltage in mV */
    uint8_t         batt_pct;       /**< Current battery percentage */
} device_state_t;

/** Singleton device state */
static device_state_t g_state = {0};

/** Mutex for protecting device state access between tasks */
static SemaphoreHandle_t g_state_mutex = NULL;

/** Queue for inter-task event notifications */
static QueueHandle_t g_event_queue = NULL;

/** Event types for inter-task communication */
typedef enum {
    EVENT_CAPTURE_DONE    = 0x01,   /**< Frame capture completed */
    EVENT_CEC_MESSAGE     = 0x02,   /**< New CEC message received */
    EVENT_HDMI_LOCK       = 0x03,   /**< HDMI RX signal lock changed */
    EVENT_BATTERY_LOW     = 0x04,   /**< Battery at low threshold */
    EVENT_BATTERY_CRIT    = 0x05,   /**< Battery at critical threshold */
    EVENT_BUTTON_CONFIG   = 0x06,   /**< Config button pressed */
    EVENT_BUTTON_CAPTURE  = 0x07,   /**< Capture button pressed */
    EVENT_COMMAND         = 0x08,   /**< Protocol command received */
    EVENT_STATUS_UPDATE   = 0x09    /**< Periodic status update request */
} event_type_t;

/** Event structure for queue */
typedef struct {
    event_type_t type;
    uint32_t     data;
    void        *payload;
} event_t;

/* =========================================================================
 * Forward Declarations
 * ========================================================================= */

static void system_init(void);
static void fpga_init(void);
static void peripherals_init(void);
static void tasks_create(void);
static void gpio_isr_handler(void *arg);
static void main_loop_iteration(void);
static void handle_event(const event_t *evt);
static bool check_battery_shutdown(void);

/* =========================================================================
 * Initialization Sequence
 * ========================================================================= */

/**
 * @brief Complete system initialization
 *
 * Order: NVS → GPIO → SPI → I²C → FPGA → SD card → WiFi → BLE → Tasks
 * Each step must succeed before proceeding; critical failures trigger reboot.
 */
static void system_init(void)
{
    ESP_LOGI(TAG, "HDMI-Siphon v%s — Author: jayis1", FIRMWARE_VERSION_STRING);
    ESP_LOGI(TAG, "Board init starting...");

    /* Initialize NVS flash storage */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase, retrying...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    /* Initialize GPIO pins */
    board_gpio_init();
    ESP_LOGI(TAG, "GPIO initialized");

    /* Initialize SPI bus for FPGA communication */
    spi_fpga_init();
    ESP_LOGI(TAG, "SPI-FPGA bus initialized");

    /* Initialize I²C bus for EDID management */
    i2c_init();
    ESP_LOGI(TAG, "I²C bus initialized");

    /* Initialize event queue */
    g_event_queue = xQueueCreate(32, sizeof(event_t));
    configASSERT(g_event_queue != NULL);

    /* Initialize state mutex */
    g_state_mutex = xSemaphoreCreateMutex();
    configASSERT(g_state_mutex != NULL);

    /* Register GPIO interrupt handlers for buttons */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BTN_CONFIG, gpio_isr_handler, (void *)EVENT_BUTTON_CONFIG);
    gpio_isr_handler_add(PIN_BTN_CAPTURE, gpio_isr_handler, (void *)EVENT_BUTTON_CAPTURE);
    gpio_set_intr_type(PIN_BTN_CONFIG, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(PIN_BTN_CAPTURE, GPIO_INTR_NEGEDGE);

    ESP_LOGI(TAG, "System init complete");
}

/**
 * @brief Initialize FPGA via SPI configuration
 *
 * Configures the iCE40UP5K FPGA by loading the bitstream from an 
 * embedded array or from SD card. Uses the SPI master interface 
 * to program the FPGA's SRAM configuration cells.
 */
static void fpga_init(void)
{
    ESP_LOGI(TAG, "Configuring FPGA...");

    /* Assert reset to FPGA */
    gpio_set_level(PIN_FPGA_CRESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Release reset and begin configuration */
    gpio_set_level(PIN_FPGA_CRESET, 1);
    vTaskDelay(pdMS_TO_TICKS(1));

    /* Check FPGA DONE signal */
    uint32_t timeout = 0;
    bool configured = false;

    while (timeout < FPGA_CONFIG_TIMEOUT_MS) {
        if (gpio_get_level(PIN_FPGA_DONE)) {
            configured = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout += 10;
    }

    if (configured) {
        ESP_LOGI(TAG, "FPGA configured successfully");
        g_state.fpga_ready = true;

        /* Initialize FPGA registers to defaults */
        reg_write(REG_SYS_CTRL, SYS_CTRL_PASSTHROUGH);
        reg_write(REG_VIDEO_MODE, VIDEO_MODE_PASSTHROUGH);
        reg_write(REG_OSD_CTRL, 0);
        reg_write(REG_CEC_CTRL, CEC_CTRL_ENABLE);
        reg_write(REG_CAPTURE_CTRL, 0);
        reg_write(REG_SDRAM_CTRL, SDRAM_CTRL_INIT | SDRAM_CTRL_REFRESH_EN);

        /* Wait for SDRAM initialization */
        reg_wait_for_bit(REG_SDRAM_STATUS, SDRAM_STATUS_INIT_DONE, 100);
        ESP_LOGI(TAG, "SDRAM initialized");
    } else {
        ESP_LOGE(TAG, "FPGA configuration FAILED (timeout)");
        g_state.fpga_ready = false;
    }
}

/**
 * @brief Initialize remaining peripherals
 */
static void peripherals_init(void)
{
    /* Initialize SD card */
    sd_card_init();
    ESP_LOGI(TAG, "SD card initialized");

    /* Initialize EDID manager */
    edid_manager_init();
    ESP_LOGI(TAG, "EDID manager initialized");

    /* Read initial video timing info */
    if (g_state.fpga_ready) {
        g_state.h_active    = (uint16_t)reg_read(REG_VIDEO_HACTIVE);
        g_state.v_active    = (uint16_t)reg_read(REG_VIDEO_VACTIVE);
        g_state.pclk_khz    = reg_read(REG_VIDEO_PCLK_KHZ);
        g_state.refresh_hz_x100 = (uint16_t)reg_read(REG_VIDEO_REFRESH_HZ);
        g_state.hdmi_locked = (reg_read(REG_SYS_STATUS) & SYS_STATUS_HDMI_IN_LOCKED) != 0;
        g_state.display_connected = (reg_read(REG_SYS_STATUS) & SYS_STATUS_HDMI_OUT_HOTPLUG) != 0;
    }

    /* Initialize battery monitoring */
    battery_init();

    ESP_LOGI(TAG, "Peripherals initialized");
    ESP_LOGI(TAG, "Video: %ux%u @ %u Hz (pclk: %u kHz)",
             g_state.h_active, g_state.v_active,
             g_state.refresh_hz_x100 / 100, g_state.pclk_khz);
}

/**
 * @brief Create all FreeRTOS tasks
 */
static void tasks_create(void)
{
    /* WiFi/HTTP/WebSocket server task */
    xTaskCreate(
        wifi_task, "wifi_task", TASK_STACK_WIFI,
        NULL, PRIORITY_WIFI, NULL
    );

    /* BLE GATT task */
    xTaskCreate(
        ble_task, "ble_task", TASK_STACK_BLE,
        NULL, PRIORITY_BLE, NULL
    );

    /* Capture control task */
    xTaskCreate(
        capture_task, "capture_task", TASK_STACK_CAPTURE,
        NULL, PRIORITY_CAPTURE, NULL
    );

    /* CEC monitor task */
    xTaskCreate(
        cec_task, "cec_task", TASK_STACK_CEC,
        NULL, PRIORITY_CEC, NULL
    );

    /* Battery monitoring task */
    xTaskCreate(
        battery_task, "battery_task", TASK_STACK_BATTERY,
        NULL, PRIORITY_BATTERY, NULL
    );

    ESP_LOGI(TAG, "All tasks created");
}

/* =========================================================================
 * GPIO Interrupt Handler
 * ========================================================================= */

/**
 * @brief GPIO interrupt handler for button presses
 *
 * Called from ISR context. Posts event to queue for processing
 * in task context to avoid doing heavy work in the ISR.
 */
static void gpio_isr_handler(void *arg)
{
    uint32_t event_type = (uint32_t)arg;
    BaseType_t higher_woken = pdFALSE;

    event_t evt = {
        .type = (event_type_t)event_type,
        .data = 0,
        .payload = NULL
    };

    /* Debounce: ignore if within 200ms of last press */
    static uint32_t last_press_time = 0;
    uint32_t now = xTaskGetTickCountFromISR();
    if ((now - last_press_time) < pdMS_TO_TICKS(200)) {
        return;
    }
    last_press_time = now;

    xQueueSendFromISR(g_event_queue, &evt, &higher_woken);
    if (higher_woken) {
        portYIELD_FROM_ISR();
    }
}

/* =========================================================================
 * Main Loop
 * ========================================================================= */

/**
 * @brief Main loop iteration — periodic status update and housekeeping
 *
 * Called every MAIN_LOOP_INTERVAL_MS. Reads FPGA status, checks battery,
 * processes any pending events from the queue, and updates global state.
 */
static void main_loop_iteration(void)
{
    event_t evt;

    /* Process all pending events (non-blocking) */
    while (xQueueReceive(g_event_queue, &evt, 0) == pdTRUE) {
        handle_event(&evt);
    }

    /* Read current FPGA status */
    if (g_state.fpga_ready) {
        uint32_t status = reg_read(REG_SYS_STATUS);

        xSemaphoreTake(g_state_mutex, portMAX_DELAY);

        bool was_locked = g_state.hdmi_locked;
        g_state.hdmi_locked     = (status & SYS_STATUS_HDMI_IN_LOCKED) != 0;
        g_state.display_connected = (status & SYS_STATUS_HDMI_OUT_HOTPLUG) != 0;
        g_state.hdcp_active     = (status & SYS_STATUS_HDCP_ACTIVE) != 0;

        /* Detect HDMI lock change */
        if (was_locked != g_state.hdmi_locked) {
            event_t evt_new = {
                .type = EVENT_HDMI_LOCK,
                .data = g_state.hdmi_locked ? 1 : 0
            };
            xQueueSend(g_event_queue, &evt_new, 0);
        }

        /* Update video timing (lazy: only when locked) */
        if (g_state.hdmi_locked) {
            g_state.h_active    = (uint16_t)reg_read(REG_VIDEO_HACTIVE);
            g_state.v_active    = (uint16_t)reg_read(REG_VIDEO_VACTIVE);
            g_state.pclk_khz    = reg_read(REG_VIDEO_PCLK_KHZ);
            g_state.refresh_hz_x100 = (uint16_t)reg_read(REG_VIDEO_REFRESH_HZ);
        }

        xSemaphoreGive(g_state_mutex);
    }

    /* Read battery */
    g_state.batt_mv = battery_read_mv();
    g_state.batt_pct = board_batt_percent(g_state.batt_mv);

    if (board_batt_critical(g_state.batt_mv) && !g_state.battery_critical) {
        g_state.battery_critical = true;
        event_t evt_new = { .type = EVENT_BATTERY_CRIT, .data = g_state.batt_mv };
        xQueueSend(g_event_queue, &evt_new, 0);
    }

    /* Increment uptime */
    g_state.uptime_seconds += MAIN_LOOP_INTERVAL_MS / 1000;
}

/**
 * @brief Handle a single event from the queue
 * @param evt Event to process
 */
static void handle_event(const event_t *evt)
{
    switch (evt->type) {
        case EVENT_BUTTON_CONFIG:
            ESP_LOGI(TAG, "Config button pressed");
            /* Toggle between passthrough and capture mode */
            if (g_state.mode == MODE_PASSTHROUGH) {
                g_state.mode = MODE_CAPTURE;
                reg_set_bits(REG_SYS_CTRL, SYS_CTRL_PASSTHROUGH);
                ESP_LOGI(TAG, "Switched to CAPTURE mode");
            } else {
                g_state.mode = MODE_PASSTHROUGH;
                reg_clear_bits(REG_SYS_CTRL, SYS_CTRL_PASSTHROUGH);
                ESP_LOGI(TAG, "Switched to PASSTHROUGH mode");
            }
            break;

        case EVENT_BUTTON_CAPTURE:
            ESP_LOGI(TAG, "Capture button pressed");
            /* Trigger a single frame capture */
            {
                uint32_t result = capture_ctrl_trigger();
                if (result == 0) {
                    g_state.frame_count++;
                    ESP_LOGI(TAG, "Frame #%u captured", g_state.frame_count);
                } else {
                    ESP_LOGW(TAG, "Capture trigger failed: %" PRIu32, result);
                }
            }
            break;

        case EVENT_CAPTURE_DONE:
            ESP_LOGI(TAG, "Capture complete, frame #%u ready", evt->data);
            /* Frame transfer will be handled by capture_task */
            break;

        case EVENT_CEC_MESSAGE:
            ESP_LOGI(TAG, "CEC message received (len=%" PRIu32 ")", evt->data);
            break;

        case EVENT_HDMI_LOCK:
            ESP_LOGI(TAG, "HDMI RX signal %s",
                     evt->data ? "LOCKED" : "UNLOCKED");
            break;

        case EVENT_BATTERY_LOW:
            ESP_LOGW(TAG, "Battery low: %" PRIu32 " mV", evt->data);
            break;

        case EVENT_BATTERY_CRIT:
            ESP_LOGE(TAG, "Battery CRITICAL: %" PRIu32 " mV — shutting down", evt->data);
            /* Schedule graceful shutdown */
            check_battery_shutdown();
            break;

        case EVENT_STATUS_UPDATE:
            /* Status update requested by WebSocket/BLE client */
            /* Response is sent by the requesting task */
            break;

        default:
            ESP_LOGW(TAG, "Unknown event type: %d", evt->type);
            break;
    }
}

/**
 * @brief Check battery and initiate safe shutdown if critical
 * @return true if shutdown initiated
 */
static bool check_battery_shutdown(void)
{
    uint32_t mv = battery_read_mv();
    if (board_batt_critical(mv)) {
        ESP_LOGW(TAG, "Battery at %" PRIu32 " mV — saving state and shutting down", mv);

        /* Save current mode to NVS for recovery */
        nvs_handle_t nvs;
        if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
            int32_t saved_mode = (int32_t)g_state.mode;
            nvs_set_i32(nvs, "last_mode", saved_mode);
            nvs_commit(nvs);
            nvs_close(nvs);
        }

        /* Wait 500ms for pending SD writes to complete */
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Put FPGA into lowest power state */
        if (g_state.fpga_ready) {
            reg_write(REG_SYS_CTRL, 0);  /* Clear all control bits */
        }

        /* Enter deep sleep */
        ESP_LOGW(TAG, "Entering deep sleep until power restored or USB connected");
        esp_deep_sleep_start();

        return true;
    }
    return false;
}

/* =========================================================================
 * Main Entry Point
 * ========================================================================= */

/**
 * @brief FreeRTOS application entry point
 *
 * This is called by the ESP-IDF startup code after FreeRTOS is initialized.
 * It performs system init, then enters the main loop.
 */
void app_main(void)
{
    /* === Initialization === */
    system_init();
    fpga_init();
    peripherals_init();
    tasks_create();

    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   HDMI-Siphon v%s — READY           ║", FIRMWARE_VERSION_STRING);
    ESP_LOGI(TAG, "║   Author: jayis1                      ║");
    ESP_LOGI(TAG, "║   Authorized Security Research Only   ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");

    /* === Main Loop === */
    while (1) {
        main_loop_iteration();

        /* Check for battery critical on every iteration */
        if (g_state.battery_critical) {
            check_battery_shutdown();
        }

        /* Sleep until next iteration */
        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_INTERVAL_MS));
    }
}
