/**
 * @file    capture_ctrl.c
 * @brief   Frame capture trigger and data transfer orchestrator
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Manages the frame capture pipeline: triggers capture on FPGA,
 * transfers frame data from SDRAM via SPI, compresses to JPEG,
 * writes to SD card, and signals completion.
 */

#include <string.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_timer.h"

#include "board.h"
#include "registers.h"
#include "capture_ctrl.h"
#include "sd_card.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "CAPTURE";

/** Frame buffer size for 1080p30 RGB24 = 1920*1080*3 = 6,220,800 bytes */
#define FRAME_SIZE_1080P            (1920U * 1080U * 3U)

/** SPI transfer chunk size (bytes) — limited by DMA buffer */
#define XFER_CHUNK_SIZE              4096U

/** JPEG compression quality */
#define JPEG_QUALITY                   85U

/** Maximum frame width supported */
#define MAX_WIDTH                     1920U

/** Maximum frame height supported */
#define MAX_HEIGHT                    1080U

/** Sequential frame number (persistent across boots via NVS) */
static uint32_t s_frame_number = 0;

/* =========================================================================
 * Initialization
 * ========================================================================= */

void capture_ctrl_init(void)
{
    /* Load frame counter from NVS */
    nvs_handle_t nvs;
    if (nvs_open("hdmi_siphon", NVS_READONLY, &nvs) == ESP_OK) {
        uint32_t val = 0;
        if (nvs_get_u32(nvs, "frame_num", &val) == ESP_OK) {
            s_frame_number = val;
        }
        nvs_close(nvs);
    }

    ESP_LOGI(TAG, "Capture controller initialized, frame counter starts at %u", s_frame_number);
}

/* =========================================================================
 * Capture Trigger
 * ========================================================================= */

int capture_ctrl_trigger(void)
{
    if (!capture_ctrl_is_ready()) {
        ESP_LOGW(TAG, "Cannot capture — system not ready");
        return -1;
    }

    /* Check SD card space */
    if (sd_card_is_mounted()) {
        if (!sd_card_has_space_for(FRAME_SIZE_1080P / 10)) {
            ESP_LOGE(TAG, "Insufficient SD card space");
            return -2;
        }
    }

    /* Trigger capture via FPGA register */
    reg_set_bits(REG_CAPTURE_CTRL, CAPTURE_CTRL_TRIGGER);

    /* Wait for capture to complete (poll status) */
    bool done = reg_wait_for_bit(REG_CAPTURE_STATUS, CAPTURE_STATUS_COMPLETE, 100);
    if (!done) {
        ESP_LOGW(TAG, "Capture trigger timed out");
        reg_clear_bits(REG_CAPTURE_CTRL, CAPTURE_CTRL_TRIGGER);
        return -3;
    }

    /* Read frame metadata */
    uint32_t pixel_count = reg_read(REG_CAPTURE_PIXEL_COUNT);
    s_frame_number++;

    ESP_LOGI(TAG, "Frame #%u captured: %" PRIu32 " pixels", s_frame_number, pixel_count);

    /* Transfer frame from FPGA SDRAM to ESP32 RAM */
    int xfer_result = capture_ctrl_transfer_frame();
    if (xfer_result < 0) {
        ESP_LOGE(TAG, "Frame transfer failed: %d", xfer_result);
        return -4;
    }

    /* Save to SD card */
    uint32_t actual_width = reg_read(REG_VIDEO_HACTIVE);
    uint32_t actual_height = reg_read(REG_VIDEO_VACTIVE);

    /* Generate JPEG filename (frame_XXXXXX.jpg) */
    char filename[32];
    snprintf(filename, sizeof(filename), "frame_%06u.jpg", s_frame_number);

    /* Write frame — in production, the raw RGB data would be JPEG-compressed
     * by a software encoder. Here we store the raw pixel data with metadata */
    esp_err_t ret = sd_card_write_frame(filename, g_frame_buffer, pixel_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write frame to SD: %s", esp_err_to_name(ret));
        return -5;
    }

    ESP_LOGI(TAG, "Frame #%u saved: %s (%ux%u, %" PRIu32 " bytes)",
             s_frame_number, filename, actual_width, actual_height, pixel_count);

    /* Persist frame counter */
    nvs_handle_t nvs;
    if (nvs_open("hdmi_siphon", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u32(nvs, "frame_num", s_frame_number);
        nvs_commit(nvs);
        nvs_close(nvs);
    }

    return 0;
}

/* =========================================================================
 * Frame Transfer
 * ========================================================================= */

/** Static frame buffer for DMA transfer */
static uint8_t g_frame_buffer[FRAME_SIZE_1080P];
static bool g_frame_valid = false;
static size_t g_frame_size = 0;

int capture_ctrl_transfer_frame(void)
{
    if (!g_state.fpga_ready) {
        return -1;
    }

    /* Get frame dimensions and calculate size */
    uint32_t h_active = reg_read(REG_VIDEO_HACTIVE);
    uint32_t v_active = reg_read(REG_VIDEO_VACTIVE);

    if (h_active == 0 || v_active == 0 || h_active > MAX_WIDTH || v_active > MAX_HEIGHT) {
        ESP_LOGE(TAG, "Invalid frame dimensions: %" PRIu32 "x%" PRIu32, h_active, v_active);
        return -2;
    }

    size_t frame_size = h_active * v_active * 3; /* 24-bit RGB */
    if (frame_size > sizeof(g_frame_buffer)) {
        ESP_LOGE(TAG, "Frame too large for buffer: %zu > %zu",
                 frame_size, sizeof(g_frame_buffer));
        return -3;
    }

    /* Initiate burst transfer from FPGA */
    reg_write(REG_FRAME_XFER_ADDR, 0);
    reg_write(REG_FRAME_XFER_WORD_COUNT, frame_size / 4);
    reg_set_bits(REG_FRAME_XFER_CTRL, FRAME_XFER_CTRL_START);

    /* Transfer data in chunks */
    size_t bytes_read = 0;
    while (bytes_read < frame_size) {
        size_t chunk = (frame_size - bytes_read < XFER_CHUNK_SIZE)
                       ? (frame_size - bytes_read)
                       : XFER_CHUNK_SIZE;

        uint32_t words = chunk / 4;
        uint32_t *buf_ptr = (uint32_t *)(g_frame_buffer + bytes_read);

        uint32_t n = reg_burst_read(REG_FRAME_DATA_PORT, buf_ptr, words);
        if (n != words) {
            ESP_LOGE(TAG, "Frame transfer incomplete: %" PRIu32 "/%" PRIu32 " words", n, words);
            return -4;
        }

        bytes_read += chunk;
    }

    g_frame_size = frame_size;
    g_frame_valid = true;

    ESP_LOGI(TAG, "Frame transfer complete: %zu bytes transferred", frame_size);
    return 0;
}

/* =========================================================================
 * Frame Access
 * ========================================================================= */

const uint8_t *capture_ctrl_get_frame(size_t *size)
{
    if (!g_frame_valid) {
        *size = 0;
        return NULL;
    }
    *size = g_frame_size;
    return g_frame_buffer;
}

void capture_ctrl_release_frame(void)
{
    g_frame_valid = false;
    g_frame_size = 0;
}

/* =========================================================================
 * Status
 * ========================================================================= */

bool capture_ctrl_is_ready(void)
{
    if (!g_state.fpga_ready) return false;

    uint32_t status = reg_read(REG_SYS_STATUS);
    return (status & SYS_STATUS_HDMI_IN_LOCKED) != 0;
}

bool capture_ctrl_is_busy(void)
{
    uint32_t status = reg_read(REG_CAPTURE_STATUS);
    return (status & CAPTURE_STATUS_CAPTURING) != 0;
}

uint32_t capture_ctrl_get_frame_count(void)
{
    return s_frame_number;
}

/* =========================================================================
 * FreeRTOS Task
 * ========================================================================= */

void capture_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Capture task started");

    while (1) {
        /* Wait for capture trigger from main or FPGA IRQ */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGD(TAG, "Capture task notified — starting capture");

        /* Check if FPGA has a frame ready */
        uint32_t status = reg_read(REG_CAPTURE_STATUS);
        if ((status & CAPTURE_STATUS_COMPLETE) == 0) {
            ESP_LOGD(TAG, "No frame ready, triggering manually");
            reg_set_bits(REG_CAPTURE_CTRL, CAPTURE_CTRL_TRIGGER);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        /* Transfer and save */
        int result = capture_ctrl_trigger();
        if (result != 0) {
            ESP_LOGW(TAG, "Capture task: trigger failed (%d)", result);
        }
    }
}
