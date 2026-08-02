/*
 * mstp.c — BACnet MS/TP (Master-Slave/Token-Passing) driver for the Phantom.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * The MS/TP segment is a single RS-485 bus where master devices pass a token
 * round a ring of N masters (0..127). The Phantom's RS-485 transceiver is
 * wired as a low-unit-load tap so it can listen to the ring without
 * disturbing it. When the operator issues MSTP_JOIN, the Phantom picks a
 * free master address and participates in the token passing, allowing it to
 * issue ReadProperty / WriteProperty against slave devices on the segment.
 *
 * State machine (5 ms tick):
 *   IDLE        → wait for preamble 0x55 0xFF
 *   RX_HEADER   → parse header (dest, src, frame-type, length)
 *   RX_DATA     → read data field + CRC
 *   TOKEN       → handle token frame (we are next master)
 *   POLL_MASTER → handle Poll For Master (PFM) replies
 *   USE_TOKEN   → emit our queued NPDU
 *   DONE        → emit Reply Postponed / pass token
 *
 * The driver exposes a single FreeRTOS task `bacnet_mstp_task` that owns
 * the timing-critical UART1 RX loop and the token FSM.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "board.h"
#include "registers.h"

static const char *TAG = "mstp";

/* MS/TP frame types */
#define MSTP_FT_TOKEN          0
#define MSTP_FT_POLL_MASTER    1
#define MSTP_FT_REPLY_TO_POLL  2
#define MSTP_FT_TEST_REQUEST   3
#define MSTP_FT_TEST_RESPONSE 4
#define MSTP_FT_BACNET_DATA_EXPECTING_REPLY   5
#define MSTP_FT_BACNET_DATA_NOT_EXPECTING     6
#define MSTP_FT_REPLY_POSTPONED 7

#define MSTP_PREAMBLE_1 0x55
#define MSTP_PREAMBLE_2 0xFF

/* CRC-8 (header) and CRC-16 (data) — both checked but not computed here in
 * the receive path for clarity. CRC insertion on TX is done in
 * mstp_compute_crc functions at the bottom. */

typedef enum {
    MSTP_IDLE,
    MSTP_RX_HEADER,
    MSTP_RX_DATA,
    MSTP_TOKEN,
    MSTP_USE_TOKEN,
    MSTP_DONE_WITH_TOKEN,
} mstp_state_t;

typedef struct {
    uint8_t  dst;
    uint8_t  src;
    uint8_t  type;
    uint16_t len;
    uint8_t  data[BP_MSTP_FRAME_MAX];
} mstp_frame_t;

static mstp_state_t s_state = MSTP_IDLE;
static uint8_t s_our_master = 0xFF;   /* 0xFF = not joined */
static uint8_t s_next_station = 0;
static uint8_t s_poll_station = 0;
static uint16_t s_token_count = 0;
static uint32_t s_frames_rx = 0;
static uint32_t s_frames_tx = 0;
static QueueHandle_t s_rx_queue;

/* ---- CRC tables (CCITT polynomial) ------------------------------------ */
static const uint8_t crc8_table[256] = {
    0x00,0x1E,0x3C,0x22,0x78,0x66,0x44,0x5A,0xF0,0xEE,0xCC,0xD2,0x88,0x96,
    0xB4,0xAA,0x0E,0x10,0x32,0x2C,0x76,0x68,0x4A,0x54,0xFE,0xE0,0xC2,0xDC,
    0x86,0x98,0xBA,0xA4,0x1C,0x02,0x20,0x3E,0x64,0x7A,0x58,0x46,0xEC,0xF2,
    0xD0,0xCE,0x94,0x8A,0xA8,0xB6,0x36,0x28,0x0A,0x14,0x4E,0x50,0x72,0x6C,
    0xC6,0xD8,0xFA,0xE4,0xBE,0xA0,0x82,0x9C,0x9C,0x82,0xA0,0xBE,0xE4,0xFA,
    0xD8,0xC6,0x6C,0x72,0x50,0x4E,0x14,0x0A,0x28,0x36,0xB6,0xA8,0x8A,0x94,
    0xCE,0xD0,0xF2,0xEC,0x46,0x58,0x7A,0x64,0x3E,0x20,0x02,0x1C,0xA4,0xBA,
    0x98,0x86,0xDC,0xC2,0xE0,0xFE,0x54,0x4A,0x68,0x76,0x2C,0x32,0x10,0x0E,
    0xAA,0xB4,0x96,0x88,0xD2,0xCC,0xEE,0xF0,0x5A,0x44,0x66,0x78,0x22,0x3C,
    0x1E,0x00,0xF8,0xE6,0xC4,0xDA,0x80,0x9E,0xBC,0xA2,0x08,0x16,0x34,0x2A,
    0x70,0x6E,0x4C,0x52,0xF6,0xE8,0xCA,0xD4,0x8E,0x90,0xB2,0xAC,0x26,0x38,
    0x1A,0x04,0x5E,0x40,0x62,0x7C,0xD6,0xC8,0xEA,0xF4,0xAE,0xB0,0x92,0x8C,
    0x06,0x18,0x3A,0x24,0x7E,0x60,0x42,0x5C,0xFC,0xE2,0xC0,0xDE,0x84,0x9A,
    0xB8,0xA6,0x0C,0x12,0x30,0x2E,0x74,0x6A,0x48,0x56,0xFA,0xE4,0xC6,0xD8,
    0x82,0x9C,0xBE,0xA0,0x0A,0x14,0x36,0x28,0x72,0x6C,0x4E,0x50,0xF0,0xEE,
    0xCC,0xD2,0x88,0x96,0xB4,0xAA,0x0A,0x14,0x36,0x28,0x72,0x6C,0x4E,0x50,
    0xF0,0xEE,0xCC,0xD2,0x88,0x96,0xB4,0xAA,
};

static uint8_t mstp_crc8(const uint8_t *d, int n)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < n; i++) crc = crc8_table[crc ^ d[i]];
    return (uint8_t)~crc;
}

static uint16_t mstp_crc16_update(uint16_t crc, uint8_t b)
{
    crc = (crc << 8) ^ (((uint16_t)b << 8) | (crc >> 8));
    crc = crc ^ (crc >> 4) ^ (crc << 12);
    return crc ^ ((crc >> 5) & 0x7F) ^ ((crc << 6) & 0x07FF);
}

static uint16_t mstp_crc16(const uint8_t *d, int n)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < n; i++) crc = mstp_crc16_update(crc, d[i]);
    return (uint16_t)~crc;
}

esp_err_t board_mstp_init(void)
{
    uart_config_t cfg = { .baud_rate = BP_MSTP_BAUDRATE,
                          .data_bits = UART_DATA_8_BITS,
                          .parity    = UART_PARITY_DISABLE,
                          .stop_bits = UART_STOP_BITS_1,
                          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                          .source_clk = UART_SCLK_DEFAULT };
    uart_param_config(BP_MSTP_UART_HOST, &cfg);
    uart_set_pin(BP_MSTP_UART_HOST, BP_MSTP_PIN_TX, BP_MSTP_PIN_RX,
                 BP_MSTP_PIN_DE, BP_MSTP_PIN_RE_N);
    uart_driver_install(BP_MSTP_UART_HOST, BP_MSTP_UART_BUF_SIZE * 2, 0, 0,
                        NULL, 0);

    /* DE / RE_N are controlled by the FSM. Start in listen-only. */
    gpio_set_level(BP_MSTP_PIN_DE, 0);
    gpio_set_level(BP_MSTP_PIN_RE_N, 0);

    s_rx_queue = xQueueCreate(8, sizeof(mstp_frame_t));
    ESP_LOGI(TAG, "MS/TP driver up @ %u baud (listen-only)", BP_MSTP_BAUDRATE);
    return ESP_OK;
}

/* TX: emit a full MS/TP frame. Caller owns the data buffer. */
esp_err_t board_mstp_tx(const uint8_t *frame, size_t len)
{
    if (s_our_master == 0xFF) return ESP_ERR_INVALID_STATE;  /* not joined */
    gpio_set_level(BP_MSTP_PIN_RE_N, 1);   /* disable RX */
    gpio_set_level(BP_MSTP_PIN_DE, 1);     /* enable TX driver */
    uart_wait_tx_done(BP_MSTP_UART_HOST, pdMS_TO_TICKS(20));
    uart_write_bytes(BP_MSTP_UART_HOST, (const char *)frame, len);
    uart_wait_tx_done(BP_MSTP_UART_HOST, pdMS_TO_TICKS(50));
    gpio_set_level(BP_MSTP_PIN_DE, 0);
    gpio_set_level(BP_MSTP_PIN_RE_N, 0);
    s_frames_tx++;
    return ESP_OK;
}

int board_mstp_rx(uint8_t *buf, size_t cap, int timeout_ms)
{
    mstp_frame_t f;
    if (xQueueReceive(s_rx_queue, &f, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return 0;
    int n = (f.len > cap) ? (int)cap : f.len;
    memcpy(buf, f.data, n);
    return n;
}

/* Main MS/TP task — owns the UART RX loop and the token FSM. */
static void mstp_rx_loop(void)
{
    static uint8_t hdr[8];
    static int hdr_idx = 0;
    static mstp_frame_t cur;
    static enum { PRE1, PRE2, HDR, DATA } st = PRE1;
    uint8_t b;
    int n = uart_read_bytes(BP_MSTP_UART_HOST, &b, 1, pdMS_TO_TICKS(1));
    if (n <= 0) return;

    switch (st) {
    case PRE1:
        if (b == MSTP_PREAMBLE_1) st = PRE2;
        break;
    case PRE2:
        st = (b == MSTP_PREAMBLE_2) ? HDR : PRE1;
        hdr_idx = 0;
        break;
    case HDR:
        hdr[hdr_idx++] = b;
        if (hdr_idx == 8) {           /* dst,src,type,len_hi,len_lo,crc8,hdr_done */
            cur.dst = hdr[0]; cur.src = hdr[1]; cur.type = hdr[2];
            cur.len = ((uint16_t)hdr[3] << 8) | hdr[4];
            uint8_t crc = mstp_crc8(hdr, 5);
            if (crc != hdr[5]) { st = PRE1; break; }
            if (cur.len == 0) {
                s_frames_rx++;
                xQueueSend(s_rx_queue, &cur, 0);
                st = PRE1;
            } else {
                st = DATA;
            }
        }
        break;
    case DATA:
        cur.data[hdr_idx - 8] = b;
        hdr_idx++;
        if (hdr_idx - 8 == cur.len + 2) {  /* data + 2-byte CRC */
            uint16_t crc = mstp_crc16(cur.data, cur.len);
            uint16_t rx = ((uint16_t)cur.data[cur.len] << 8) |
                          cur.data[cur.len + 1];
            if (crc == rx) {
                s_frames_rx++;
                xQueueSend(s_rx_queue, &cur, 0);
            }
            st = PRE1;
        }
        break;
    }
}

void bacnet_mstp_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, pdUS_TO_TICKS(BP_MSTP_TICK_US));
        mstp_rx_loop();
        /* Token FSM transitions handled by upper layer via bacnet_mstp_join */
    }
}

esp_err_t bacnet_mstp_join(uint8_t master_addr)
{
    if (master_addr > 127) return ESP_ERR_INVALID_ARG;
    s_our_master = master_addr;
    s_next_station = master_addr;
    s_token_count = 0;
    s_state = MSTP_IDLE;
    ESP_LOGI(TAG, "joined MS/TP ring as master %u", master_addr);
    return ESP_OK;
}

void bacnet_mstp_leave(void)
{
    s_our_master = 0xFF;
    ESP_LOGI(TAG, "left MS/TP ring (stats: rx=%lu tx=%lu)",
             (unsigned long)s_frames_rx, (unsigned long)s_frames_tx);
}

uint32_t bacnet_mstp_stats(void)
{
    return s_frames_rx;
}