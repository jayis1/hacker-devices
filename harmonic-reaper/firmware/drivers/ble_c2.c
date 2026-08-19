/*
 * ble_c2.c — BLE command/control
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Uses a simplified Nordic UART Service (NUS) implementation. The full
 * SoftDevice S140 stack handles the BLE link; this file implements the
 * application-layer frame parser on top of a RX ring buffer and a TX
 * notifier. We don't include the SoftDevice headers here (they are
 * provided by Nordic's nRF5 SDK); we declare the minimal SoftDevice
 * interface we use and stub the SoftDevice calls that would normally
 * be provided by the SDK linkage.
 *
 * Frame format (NUS-style, binary):
 *   [START:0xAA][OP:1][LEN:1][PAYLOAD:LEN][CRC:1]
 *   CRC = XOR of all bytes from OP..PAYLOAD.
 */
#include "../registers.h"
#include "../board.h"
#include "ble_c2.h"
#include "storage.h"

/* Minimal SoftDevice interface stubs (provided by SDK at link time).
 * We declare them so the file compiles standalone for syntax checking. */
#define SD_BLE_ENABLE              0x10
#define SD_BLE_GAP_ADDR_SET         0x11
#define SD_BLE_GAP_ADV_START        0x12
#define SD_BLE_GATTS_VALUE_SET      0x13
#define SD_BLE_GATTS_HVX           0x14

typedef struct { uint32_t field1; uint32_t field2; } sd_ble_enable_t;
typedef struct { uint8_t addr[6]; } ble_gap_addr_t;
typedef struct { uint8_t len; uint8_t data[31]; } ble_data_t;

extern uint32_t sd_ble_enable(sd_ble_enable_t *p);
extern uint32_t sd_ble_gap_addr_set(const ble_gap_addr_t *p);
extern uint32_t sd_ble_gap_adv_start(uint8_t adv_handle, uint8_t conn_cfg_tag);
extern uint32_t sd_ble_gatts_value_set(uint16_t svc, uint16_t char,
                                       uint16_t *len, const uint8_t *data);
extern uint32_t sd_ble_gatts_hvx(uint16_t conn, uint16_t *len, uint8_t *data);

/* Ring buffer for received commands (single producer: BLE IRQ, single
 * consumer: scheduler task). Capacity = 8 commands. */
#define CMD_QUEUE_LEN 8
static ble_cmd_t cmd_queue[CMD_QUEUE_LEN];
static volatile uint8_t cmd_head, cmd_tail;

/* Assembled-frame RX state machine */
static uint8_t rx_state;
static uint8_t rx_op;
static uint8_t rx_len;
static uint8_t rx_idx;
static uint8_t rx_buf[16];
static uint8_t rx_crc;

#define RX_STATE_START   0
#define RX_STATE_OP      1
#define RX_STATE_LEN    2
#define RX_STATE_PAYLOAD 3
#define RX_STATE_CRC     4

/* TX notification queue */
#define TX_QUEUE_LEN 32
static uint8_t tx_queue[TX_QUEUE_LEN];
static volatile uint8_t tx_head, tx_tail;

/* BLE connection handle (from SoftDevice event handler) */
static uint16_t g_conn_handle = 0xFFFFu;
static uint8_t  g_connected = 0;

/* NUS service & characteristic handles (assigned during service registration) */
static uint16_t g_nus_rx_handle = 0;   /* app writes here */
static uint16_t g_nus_tx_handle = 0;   /* wand notifies here */

/* ---- enqueue a command from the parsed frame ---- */
static void enqueue_cmd(uint8_t op, uint8_t a0, uint8_t a1)
{
    uint8_t next = (cmd_tail + 1) & (CMD_QUEUE_LEN - 1);
    if (next == cmd_head) return;  /* overflow — drop */
    cmd_queue[cmd_tail].op = op;
    cmd_queue[cmd_tail].arg0 = a0 | ((uint16_t)rx_buf[1] << 8);
    cmd_queue[cmd_tail].arg1 = a1 | ((uint16_t)rx_buf[3] << 8);
    cmd_tail = next;
}

/* ---- feed a received byte from the SoftDevice NUS RX char into the parser ---- */
void ble_on_rx_byte(uint8_t b)
{
    switch (rx_state) {
    case RX_STATE_START:
        if (b == 0xAAu) { rx_state = RX_STATE_OP; rx_crc = 0; }
        break;
    case RX_STATE_OP:
        rx_op = b; rx_crc = b; rx_state = RX_STATE_LEN;
        break;
    case RX_STATE_LEN:
        rx_len = b; rx_crc ^= b;
        rx_idx = 0;
        rx_state = (rx_len == 0) ? RX_STATE_CRC : RX_STATE_PAYLOAD;
        break;
    case RX_STATE_PAYLOAD:
        rx_buf[rx_idx++] = b; rx_crc ^= b;
        if (rx_idx >= rx_len) rx_state = RX_STATE_CRC;
        break;
    case RX_STATE_CRC:
        if (b == rx_crc) {
            /* valid frame — dispatch */
            switch (rx_op) {
            case BLE_CMD_SET_MODE:
                enqueue_cmd(rx_op, rx_buf[0], 0);
                break;
            case BLE_CMD_SET_TX_POWER:
                enqueue_cmd(rx_op, rx_buf[0], 0);
                break;
            case BLE_CMD_SET_PULSE:
                enqueue_cmd(rx_op, rx_buf[0], rx_buf[1]);
                break;
            case BLE_CMD_SET_THRESH:
                enqueue_cmd(rx_op, rx_buf[0], rx_buf[1]);
                break;
            case BLE_CMD_SET_QUIET:
                enqueue_cmd(rx_op, rx_buf[0], 0);
                break;
            case BLE_CMD_QUERY_STATUS:
                enqueue_cmd(rx_op, 0, 0);
                break;
            case BLE_CMD_EXPORT:
                enqueue_cmd(rx_op, 0, 0);
                break;
            default:
                break;
            }
        }
        rx_state = RX_STATE_START;
        break;
    }
}

uint8_t ble_dequeue_cmd(ble_cmd_t *out)
{
    if (cmd_head == cmd_tail) return 0;
    *out = cmd_queue[cmd_head];
    cmd_head = (cmd_head + 1) & (CMD_QUEUE_LEN - 1);
    return 1;
}

/* ---- build & send a notification frame ---- */
static void ble_send_frame(uint8_t op, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[32];
    frame[0] = 0xAAu;
    frame[1] = op;
    frame[2] = len;
    uint8_t crc = op ^ len;
    for (uint8_t i = 0; i < len; i++) {
        frame[3 + i] = payload[i];
        crc ^= payload[i];
    }
    frame[3 + len] = crc;

    if (g_connected && g_conn_handle != 0xFFFFu) {
        uint16_t total = (uint16_t)(3 + len + 1);
        sd_ble_gatts_hvx(g_conn_handle, &total, frame);
    }
    /* Also echo to USB CDC for debugging */
    extern void usb_cdc_write(const uint8_t *data, uint16_t len);
    usb_cdc_write(frame, (uint16_t)(3 + len + 1));
}

void ble_send_status(const ble_status_t *s)
{
    uint8_t payload[16];
    uint8_t i = 0;
    payload[i++] = (uint8_t)s->mode;
    payload[i++] = (uint8_t)(s->p2_dbfs & 0xFFu);
    payload[i++] = (uint8_t)(s->p2_dbfs >> 8);
    payload[i++] = (uint8_t)(s->p3_dbfs & 0xFFu);
    payload[i++] = (uint8_t)(s->p3_dbfs >> 8);
    payload[i++] = (uint8_t)s->ratio_db;
    payload[i++] = s->classify;
    payload[i++] = (uint8_t)s->tx_power_dbm;
    payload[i++] = s->batt_pct;
    payload[i++] = (uint8_t)(s->hit_count & 0xFFu);
    payload[i++] = (uint8_t)(s->hit_count >> 8);
    ble_send_frame(BLE_NOTIF_STATUS, payload, i);
}

void ble_notify_hit(const hit_record_t *h)
{
    uint8_t payload[20];
    uint8_t i = 0;
    payload[i++] = (uint8_t)(h->timestamp_ms & 0xFFu);
    payload[i++] = (uint8_t)(h->timestamp_ms >> 8);
    payload[i++] = (uint8_t)(h->timestamp_ms >> 16);
    payload[i++] = (uint8_t)(h->pitch_deg & 0xFFu);
    payload[i++] = (uint8_t)(h->pitch_deg >> 8);
    payload[i++] = (uint8_t)(h->yaw_deg & 0xFFu);
    payload[i++] = (uint8_t)(h->yaw_deg >> 8);
    payload[i++] = (uint8_t)(h->p2_dbfs & 0xFFu);
    payload[i++] = (uint8_t)(h->p2_dbfs >> 8);
    payload[i++] = (uint8_t)(h->p3_dbfs & 0xFFu);
    payload[i++] = (uint8_t)(h->p3_dbfs >> 8);
    payload[i++] = (uint8_t)h->ratio_db;
    payload[i++] = h->classify;
    payload[i++] = (uint8_t)h->tx_power_dbm;
    ble_send_frame(BLE_NOTIF_HIT, payload, i);
}

void ble_notify_export_done(uint32_t count)
{
    uint8_t payload[4];
    payload[0] = (uint8_t)(count & 0xFFu);
    payload[1] = (uint8_t)(count >> 8);
    payload[2] = (uint8_t)(count >> 16);
    payload[3] = (uint8_t)(count >> 24);
    ble_send_frame(BLE_NOTIF_EXPORT_DONE, payload, 4);
}

/* ---- init: start SoftDevice + NUS + advertising ---- */
void ble_init(void)
{
    /* Enable SoftDevice (would normally pass clock config) */
    sd_ble_enable(0);

    /* Set MAC address from FICR */
    ble_gap_addr_t addr;
    addr.addr[0] = (uint8_t)(FICR_DEVICEADDR1 >> 8)  | 0xC0u;  /* static random */
    addr.addr[1] = (uint8_t)(FICR_DEVICEADDR1);
    addr.addr[2] = (uint8_t)(FICR_DEVICEADDR0 >> 24);
    addr.addr[3] = (uint8_t)(FICR_DEVICEADDR0 >> 16);
    addr.addr[4] = (uint8_t)(FICR_DEVICEADDR0 >> 8);
    addr.addr[5] = (uint8_t)(FICR_DEVICEADDR0);
    sd_ble_gap_addr_set(&addr);

    /* Build advertisement: "HR-NLJD" + service UUID */
    uint8_t adv[31];
    uint8_t ai = 0;
    adv[ai++] = 0x02; adv[ai++] = 0x01; adv[ai++] = 0x06;  /* flags: LE gen */
    adv[ai++] = 0x08; adv[ai++] = 0x09;                    /* name */
    const char *name = "HR-NLJD";
    for (uint8_t k = 0; name[k] && ai < 20; k++) adv[ai++] = name[k];
    adv[1] = ai - 2;  /* fix length */

    ble_data_t adv_data;
    adv_data.len = ai;
    for (uint8_t k = 0; k < ai; k++) adv_data.data[k] = adv[k];

    /* (In a full SDK build, sd_ble_gap_adv_data_set would be called here,
     * then sd_ble_gap_adv_start.) We stub the start: */
    sd_ble_gap_adv_start(0, 0);

    cmd_head = cmd_tail = 0;
    rx_state = RX_STATE_START;
    tx_head = tx_tail = 0;
}

/* EOF — ble_c2.c — jayis1 */