/*
 * drivers/ble_uart.c — BLE 5.0 control link over USART3 (nRF52840)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The nRF52840 companion runs a transparent UART-BLE bridge. We speak
 * a binary, CRC-8-protected frame protocol to it for operator control.
 */
#include "../board.h"
#include "../registers.h"

/* ---- Frame protocol ----------------------------------------------- */
/*  [STX][LEN][OP][PAYLOAD...][CRC8][ETX]
 *  STX=0xA5, ETX=0x5A, LEN = OP+PAYLOAD length (1..62)
 */
#define BLE_STX   0xA5
#define BLE_ETX   0x5A

static uint8_t rx_buf[BLE_BUF_SZ];
static uint16_t rx_len;
static uint8_t rx_state;   /* 0=idle,1=got STX,2=got LEN,3=got payload,4=got CRC */
static uint8_t rx_plen;

static uint8_t crc8_update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++)
        crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    return crc;
}

uint8_t ble_crc8(const uint8_t *p, uint8_t n)
{
    uint8_t c = 0;
    while (n--) c = crc8_update(c, *p++);
    return c;
}

/* ---- TX ------------------------------------------------------------ */
static void ble_tx_raw(uint8_t c)
{
    uart_putc(USART3_BASE, c);
}

void ble_send_frame(uint8_t op, const uint8_t *payload, uint8_t plen)
{
    if (plen > 60) plen = 60;
    uint8_t crc = 0;
    crc = crc8_update(crc, op);
    for (uint8_t i = 0; i < plen; i++) crc = crc8_update(crc, payload[i]);
    ble_tx_raw(BLE_STX);
    ble_tx_raw(plen + 1);   /* len includes opcode */
    ble_tx_raw(op);
    for (uint8_t i = 0; i < plen; i++) ble_tx_raw(payload[i]);
    ble_tx_raw(crc);
    ble_tx_raw(BLE_ETX);
}

/* ---- RX state machine --------------------------------------------- */
/* Called from the main poll loop with each new byte from USART3. */
void ble_poll_rx(void)
{
    uint8_t c;
    while (uart_getc_nb(USART3_BASE, &c)) {
        switch (rx_state) {
        case 0:
            if (c == BLE_STX) rx_state = 1;
            break;
        case 1:
            if (c == 0 || c > 63) { rx_state = 0; break; }
            rx_plen = c;
            rx_len = 0;
            rx_state = 2;
            break;
        case 2:
            rx_buf[rx_len++] = c;
            if (rx_len >= rx_plen) rx_state = 3;
            break;
        case 3:
            /* CRC byte */
            rx_buf[rx_len] = c;     /* store for check */
            rx_state = 4;
            break;
        case 4:
            if (c == BLE_ETX) {
                uint8_t crc = ble_crc8(rx_buf, rx_plen);
                if (crc == rx_buf[rx_plen]) {
                    ble_handle_frame(rx_buf[0], &rx_buf[1], rx_plen - 1);
                }
            }
            rx_state = 0;
            break;
        }
    }
}

/* ---- Frame handler (dispatches to subsystems) ------------------- */
/* Opcodes match the protocol documented in the README. */
void ble_handle_frame(uint8_t op, const uint8_t *payload, uint8_t plen)
{
    switch (op) {
    case 0x01: { /* PING */
        uint8_t rsp[4] = {
            (uint8_t)(RP_FW_VERSION & 0xFF),
            (uint8_t)(RP_FW_VERSION >> 8),
            power_battery_percent(),
            drfm_status()
        };
        ble_send_frame(0x81, rsp, 4);
        break;
    }
    case 0x02: { /* SET_RANGE */
        if (plen >= 4) {
            uint32_t r = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8) |
                         ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
            scenario_override_range(r);
            ble_send_frame(0x82, 0, 0);
        }
        break;
    }
    case 0x03: { /* SET_VEL */
        if (plen >= 4) {
            int32_t v = (int32_t)((uint32_t)payload[0] |
                        ((uint32_t)payload[1] << 8) |
                        ((uint32_t)payload[2] << 16) |
                        ((uint32_t)payload[3] << 24));
            scenario_override_vel(v);
            ble_send_frame(0x83, 0, 0);
        }
        break;
    }
    case 0x04: { /* SET_RCS */
        if (plen >= 2) {
            int16_t r = (int16_t)((uint16_t)payload[0] |
                        ((uint16_t)payload[1] << 8));
            scenario_override_rcs(r);
            ble_send_frame(0x84, 0, 0);
        }
        break;
    }
    case 0x06: { /* LOAD_SCENARIO */
        if (plen >= 1) scenario_load_builtin(payload[0]);
        ble_send_frame(0x86, 0, 0);
        break;
    }
    case 0x07: { /* SNIFF */
        uint8_t t = plen ? payload[0] : 10;
        uint8_t ok = radar_sniff(t);
        uint8_t rsp[12];
        rsp[0] = ok;
        uint64_t fs = radar_band_start_hz();
        uint64_t bw = radar_band_bw_hz();
        for (int i = 0; i < 5; i++) rsp[1 + i] = (uint8_t)(fs >> (i*8));
        for (int i = 0; i < 5; i++) rsp[6 + i] = (uint8_t)(bw >> (i*8));
        ble_send_frame(0x87, rsp, 11);
        break;
    }
    case 0x08: { /* ARM (stage 1) */
        /* Two-step interlock handled by main.c timer */
        rp_arm_request();
        ble_send_frame(0x88, 0, 0);
        break;
    }
    case 0x09: { /* DISARM */
        scenario_stop();
        ble_send_frame(0x89, 0, 0);
        break;
    }
    case 0x0B: { /* SET_POWER */
        if (plen >= 1) {
            uint8_t lvl = payload[0];
            if (lvl > RP_MAX_POWER_CODE) lvl = RP_MAX_POWER_CODE;
            drfm_write_u8(DRFM_REG_GAIN, lvl);
        }
        ble_send_frame(0x8B, 0, 0);
        break;
    }
    default:
        ble_send_frame(0xFF, 0, 0);   /* unknown opcode */
        break;
    }
}

void ble_init(void)
{
    rx_state = 0;
    rx_len = 0;
    rx_plen = 0;
    uart_init(USART3_BASE, BLE_BAUD);
}