/**
 * @file dsp_driver.c
 * @brief ESP32-S3 DSP co-processor driver implementation
 *
 * Communicates with the ESP32-S3 DSP over SPI to send frame data and
 * receive processed results (compressed frames, OCR text, credential alerts).
 *
 * The ESP32-S3 runs a custom firmware that implements:
 *   - Delta frame compression (8x8 block-based difference + RLE)
 *   - OCR text extraction (lightweight CNN text detector)
 *   - Credential pattern matching (regex-like pattern engine)
 *   - QR code and barcode decoding (ZXing port)
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "dsp_driver.h"
#include "flash_driver.h"
#include "registers.h"
#include <string.h>

/*===========================================================================
 * INTERNAL STATE
 *===========================================================================*/

static uint8_t s_dsp_status = DSP_STATUS_IDLE;
static uint16_t s_dsp_width = 0;
static uint16_t s_dsp_height = 0;
static uint8_t s_dsp_color_depth = 16;
static uint8_t s_dsp_mode = DSP_MODE_FULL_PIPELINE;

/* SPI transfer buffer */
static uint8_t s_spi_tx[512];
static uint8_t s_spi_rx[512];

/*===========================================================================
 * SPI HELPER FUNCTIONS
 *===========================================================================*/

static void dsp_spi_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    NRF_SPIM1->ENABLE = SPIM_ENABLE_DISABLE;
    NRF_SPIM1->TXD_PTR = (uint32_t)tx;
    NRF_SPIM1->RXD_PTR = (uint32_t)rx;
    NRF_SPIM1->TXD_MAXCNT = len;
    NRF_SPIM1->RXD_MAXCNT = len;
    NRF_SPIM1->ENABLE = SPIM_ENABLE_ENABLE;
    NRF_SPIM1->TASKS_START = 1;
    while (NRF_SPIM1->EVENTS_END == 0);
    NRF_SPIM1->EVENTS_END = 0;
    NRF_SPIM1->ENABLE = SPIM_ENABLE_DISABLE;
}

static void dsp_spi_command(uint8_t cmd, const uint8_t *params, uint8_t param_len)
{
    uint16_t total = 1 + param_len;
    if (total > sizeof(s_spi_tx)) return;

    s_spi_tx[0] = cmd;
    if (params && param_len > 0) {
        memcpy(&s_spi_tx[1], params, param_len);
    }

    GPIO_OUT_CLR(0, DSP_SPI_CSN_PIN);
    dsp_spi_transfer(s_spi_tx, s_spi_rx, total);
    GPIO_OUT_SET(0, DSP_SPI_CSN_PIN);
}

static uint8_t dsp_spi_read_status(void)
{
    s_spi_tx[0] = DSP_CMD_GET_STATUS;
    s_spi_tx[1] = 0xFF;

    GPIO_OUT_CLR(0, DSP_SPI_CSN_PIN);
    dsp_spi_transfer(s_spi_tx, s_spi_rx, 2);
    GPIO_OUT_SET(0, DSP_SPI_CSN_PIN);

    return s_spi_rx[1];
}

static uint16_t dsp_spi_read_data(uint8_t cmd, uint8_t *buffer, uint16_t max_size)
{
    uint16_t read_len = MIN(max_size, sizeof(s_spi_rx) - 2);

    s_spi_tx[0] = cmd;
    memset(&s_spi_tx[1], 0xFF, read_len + 1);

    GPIO_OUT_CLR(0, DSP_SPI_CSN_PIN);
    dsp_spi_transfer(s_spi_tx, s_spi_rx, read_len + 2);
    GPIO_OUT_SET(0, DSP_SPI_CSN_PIN);

    /* First byte after command is length, then data */
    uint16_t data_len = s_spi_rx[1];
    if (data_len > read_len) data_len = read_len;

    memcpy(buffer, &s_spi_rx[2], data_len);
    return data_len;
}

/*===========================================================================
 * DRIVER IMPLEMENTATION
 *===========================================================================*/

void dsp_driver_init(void)
{
    /* Initialize SPI1 for DSP communication */
    NRF_SPIM1->PSEL_SCK = BOARD_PIN_DSP_SPI_SCK;
    NRF_SPIM1->PSEL_MOSI = BOARD_PIN_DSP_SPI_MOSI;
    NRF_SPIM1->PSEL_MISO = BOARD_PIN_DSP_SPI_MISO;
    NRF_SPIM1->FREQUENCY = SPIM_FREQ_32M;
    NRF_SPIM1->CONFIG = SPIM_CONFIG_ORDER_MSB | SPIM_CONFIG_CPOL_ACTIVEHIGH |
                        SPIM_CONFIG_CPHA_LEADING;

    /* Set CS high (idle) */
    GPIO_OUT_SET(0, DSP_SPI_CSN_PIN);

    s_dsp_status = DSP_STATUS_IDLE;
}

bool dsp_wait_ready(uint32_t timeout_ms)
{
    /* Wait for handshake pin to go high indicating DSP is ready */
    uint32_t count = 0;
    uint32_t max_count = timeout_ms * 100;  /* approximate loop iterations */

    do {
        if (DSP_HANDSHAKE_IS_HIGH()) {
            /* Verify by reading status */
            uint8_t status = dsp_spi_read_status();
            if (status != 0xFF && status != DSP_STATUS_ERROR) {
                s_dsp_status = status;
                return true;
            }
        }
        for (volatile int i = 0; i < 100; i++);
        count++;
    } while (count < max_count);

    return false;
}

void dsp_set_mode(uint8_t mode)
{
    s_dsp_mode = mode;
    dsp_spi_command(DSP_CMD_SET_MODE, &mode, 1);
}

void dsp_set_power_mode(uint8_t mode)
{
    dsp_spi_command(DSP_CMD_SET_POWER_MODE, &mode, 1);
}

void dsp_set_resolution(uint16_t width, uint16_t height)
{
    s_dsp_width = width;
    s_dsp_height = height;
    uint8_t params[2] = { (uint8_t)(width >> 8), (uint8_t)(width & 0xFF) };
    /* Send width */
    dsp_spi_command(DSP_CMD_SET_RESOLUTION, params, 2);
    /* Send height in a second command (simplified protocol) */
    params[0] = (uint8_t)(height >> 8);
    params[1] = (uint8_t)(height & 0xFF);
    dsp_spi_command(DSP_CMD_SET_RESOLUTION, params, 2);
}

void dsp_set_color_depth(uint8_t depth)
{
    s_dsp_color_depth = depth;
    dsp_spi_command(DSP_CMD_SET_COLOR_DEPTH, &depth, 1);
}

void dsp_load_ocr_model(void)
{
    /* Read OCR model from flash and stream to DSP */
    uint8_t model_buf[256];
    uint32_t offset = 0;
    uint32_t model_size = NOR_FLASH_OCR_MODEL_SIZE;

    /* Signal start of model upload */
    dsp_spi_command(DSP_CMD_SET_OCR_MODEL, NULL, 0);

    while (offset < model_size) {
        uint16_t chunk = MIN(sizeof(model_buf), model_size - offset);
        flash_read(NOR_FLASH_OCR_MODEL + offset, model_buf, chunk);

        /* Send chunk to DSP */
        GPIO_OUT_CLR(0, DSP_SPI_CSN_PIN);
        dsp_spi_transfer(model_buf, s_spi_rx, chunk);
        GPIO_OUT_SET(0, DSP_SPI_CSN_PIN);

        offset += chunk;
    }
}

void dsp_load_patterns(void)
{
    /* Send built-in credential pattern definitions to DSP */
    /* Pattern definitions are hardcoded for common credential types */

    /* JWT token pattern: "eyJ" prefix */
    uint8_t jwt_pattern[] = { PATTERN_TYPE_JWT_TOKEN, 3, 'e', 'y', 'J' };
    dsp_spi_command(DSP_CMD_SET_PATTERNS, jwt_pattern, sizeof(jwt_pattern));

    /* API key pattern: 32+ hex characters */
    uint8_t apikey_pattern[] = { PATTERN_TYPE_API_KEY, 1, 0x01 };  /* Hex mode */
    dsp_spi_command(DSP_CMD_SET_PATTERNS, apikey_pattern, sizeof(apikey_pattern));

    /* OTP code pattern: 4-8 consecutive digits */
    uint8_t otp_pattern[] = { PATTERN_TYPE_OTP_CODE, 1, 0x02 };  /* Digit mode */
    dsp_spi_command(DSP_CMD_SET_PATTERNS, otp_pattern, sizeof(otp_pattern));

    /* URL pattern: "http" prefix */
    uint8_t url_pattern[] = { PATTERN_TYPE_URL, 4, 'h', 't', 't', 'p' };
    dsp_spi_command(DSP_CMD_SET_PATTERNS, url_pattern, sizeof(url_pattern));

    /* Email pattern: "@" symbol with surrounding text */
    uint8_t email_pattern[] = { PATTERN_TYPE_EMAIL, 1, '@' };
    dsp_spi_command(DSP_CMD_SET_PATTERNS, email_pattern, sizeof(email_pattern));

    /* IP address pattern: N.N.N.N */
    uint8_t ip_pattern[] = { PATTERN_TYPE_IP_ADDRESS, 1, 0x03 };  /* IP mode */
    dsp_spi_command(DSP_CMD_SET_PATTERNS, ip_pattern, sizeof(ip_pattern));

    /* Enable QR decoding */
    uint8_t qr_enable = 1;
    dsp_spi_command(DSP_CMD_ENABLE_QR_DECODE, &qr_enable, 1);
}

void dsp_start_processing(void)
{
    /* Start the DSP processing pipeline */
    dsp_spi_command(DSP_CMD_START_COMPRESSION, NULL, 0);
    if (s_dsp_mode == DSP_MODE_FULL_PIPELINE || s_dsp_mode == DSP_MODE_OCR_ONLY ||
        s_dsp_mode == DSP_MODE_COMPRESS_OCR) {
        dsp_spi_command(DSP_CMD_START_OCR, NULL, 0);
    }
    if (s_dsp_mode == DSP_MODE_FULL_PIPELINE) {
        dsp_spi_command(DSP_CMD_START_PATTERN, NULL, 0);
    }
}

void dsp_stop_processing(void)
{
    /* Stop all DSP processing */
    dsp_spi_command(DSP_CMD_RESET, NULL, 0);
    s_dsp_status = DSP_STATUS_IDLE;
}

void dsp_write_frame(const uint8_t *frame_data, uint32_t size)
{
    uint32_t offset = 0;

    while (offset < size) {
        uint16_t chunk = MIN(sizeof(s_spi_tx), size - offset);
        memcpy(s_spi_tx, frame_data + offset, chunk);

        GPIO_OUT_CLR(0, DSP_SPI_CSN_PIN);
        dsp_spi_transfer(s_spi_tx, s_spi_rx, chunk);
        GPIO_OUT_SET(0, DSP_SPI_CSN_PIN);

        offset += chunk;
    }
}

void dsp_write_reference_frame(const uint8_t *frame_data, uint32_t size)
{
    /* Use a different command to distinguish reference frame */
    dsp_spi_command(0x20, NULL, 0);  /* 0x20 = SET_REFERENCE_FRAME */
    dsp_write_frame(frame_data, size);
}

void dsp_start_compression(uint16_t threshold)
{
    uint8_t params[2] = { (uint8_t)(threshold >> 8), (uint8_t)(threshold & 0xFF) };
    dsp_spi_command(DSP_CMD_SET_THRESHOLD, params, 2);
    dsp_spi_command(DSP_CMD_START_COMPRESSION, NULL, 0);
    s_dsp_status = DSP_STATUS_COMPRESSING;
}

void dsp_start_ocr(void)
{
    dsp_spi_command(DSP_CMD_START_OCR, NULL, 0);
    s_dsp_status |= DSP_STATUS_OCR_RUNNING;
}

void dsp_start_pattern_matching(void)
{
    dsp_spi_command(DSP_CMD_START_PATTERN, NULL, 0);
    s_dsp_status |= DSP_STATUS_PATTERN_RUNNING;
}

void dsp_start_qr_decode(void)
{
    uint8_t enable = 1;
    dsp_spi_command(DSP_CMD_ENABLE_QR_DECODE, &enable, 1);
}

bool dsp_is_processing_complete(void)
{
    uint8_t status = dsp_spi_read_status();
    s_dsp_status = status;

    /* Complete when no processing flags are set and data is ready */
    if ((status & (DSP_STATUS_COMPRESSING | DSP_STATUS_OCR_RUNNING |
                   DSP_STATUS_PATTERN_RUNNING)) == 0) {
        return true;
    }
    return false;
}

uint32_t dsp_read_compressed(uint8_t *buffer, uint32_t max_size)
{
    uint32_t total_read = 0;

    while (total_read < max_size) {
        uint16_t chunk = MIN(sizeof(s_spi_rx) - 2, max_size - total_read);
        uint16_t read = dsp_spi_read_data(DSP_CMD_GET_COMPRESSED, 
                                          buffer + total_read, chunk);
        if (read == 0) break;
        total_read += read;
        if (read < chunk) break;  /* No more data */
    }

    return total_read;
}

uint16_t dsp_read_ocr_text(uint8_t *buffer, uint16_t max_size)
{
    return dsp_spi_read_data(DSP_CMD_GET_OCR_TEXT, buffer, max_size);
}

uint16_t dsp_read_alerts(void)
{
    uint8_t alert_buf[4];
    uint16_t count = dsp_spi_read_data(DSP_CMD_GET_PATTERNS, alert_buf, sizeof(alert_buf));
    if (count >= 2) {
        return (alert_buf[0] << 8) | alert_buf[1];
    }
    return 0;
}

uint16_t dsp_read_alert_data(uint8_t *buffer, uint16_t max_size)
{
    return dsp_spi_read_data(DSP_CMD_GET_PATTERNS, buffer, max_size);
}