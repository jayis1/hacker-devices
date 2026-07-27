/*
 * ble_link.c — UART link to ESP32-C3 BLE module for app communication
 *
 * Implements a simple length-prefixed binary protocol over USART1.
 * The ESP32-C3 bridges this to BLE GATT characteristics.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <string.h>
#include "ble_link.h"
#include "registers.h"
#include "board.h"
#include "skew_gen.h"

void ble_link_init(ble_link_t *st)
{
    memset(st, 0, sizeof(*st));
    st->connected = 0;
}

void ble_link_uart_rx_byte(ble_link_t *st, uint8_t b)
{
    /* Protocol: [SOF=0xAA][LEN_HI][LEN_LO][CMD][payload...][CRC8]
     * LEN = payload + CMD + CRC8 (i.e., total minus SOF and LEN)
     */
    if (st->rx_idx == 0 && b != 0xAA)
        return;  /* wait for start-of-frame */
    if (st->rx_idx < BLE_UART_BUF_SIZE)
        st->rx_buf[st->rx_idx++] = b;

    /* After we have SOF + LEN_HI + LEN_LO, compute expected total */
    if (st->rx_idx == 3) {
        st->rx_expected = ((uint16_t)st->rx_buf[1] << 8) | st->rx_buf[2];
        st->rx_expected += 3;  /* + SOF + 2 len bytes */
    }
    if (st->rx_idx >= 3 && st->rx_idx >= st->rx_expected && st->rx_expected > 3) {
        /* Full frame received — dispatch is handled in ble_link_dispatch */
        st->connected = 1;
    }
}

static uint8_t compute_crc(const uint8_t *frame, uint16_t len)
{
    /* Same CRC-8/MAXIM as covert_codec */
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= frame[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else crc <<= 1;
        }
    }
    return crc;
}

void ble_link_send_event(ble_link_t *st, uint8_t evt_code,
                         const uint8_t *payload, uint16_t len)
{
    if (st->tx_len + 4 + len > BLE_UART_BUF_SIZE)
        return;  /* no room */
    uint16_t total = len + 2;  /* evt_code + CRC8 + payload = payload + 1 + 1 */
    st->tx_buf[st->tx_len++] = 0xAA;   /* SOF */
    st->tx_buf[st->tx_len++] = (uint8_t)(total >> 8);
    st->tx_buf[st->tx_len++] = (uint8_t)(total & 0xFF);
    st->tx_buf[st->tx_len++] = evt_code;
    if (len > 0)
        memcpy(&st->tx_buf[st->tx_len], payload, len);
    st->tx_len += len;
    /* CRC over evt_code + payload */
    uint8_t crc = compute_crc(&st->tx_buf[st->tx_len - len - 1], len + 1);
    st->tx_buf[st->tx_len++] = crc;
}

int ble_link_tx_drain(ble_link_t *st, uint8_t *out, uint16_t max)
{
    if (st->tx_len == 0)
        return 0;
    uint16_t n = st->tx_len;
    if (n > max) n = max;
    memcpy(out, st->tx_buf, n);
    if (st->tx_len > n)
        memmove(st->tx_buf, &st->tx_buf[n], st->tx_len - n);
    st->tx_len -= n;
    return n;
}

static void send_status(ble_link_t *ble, ptp_engine_state_t *ptp,
                         ntp_engine_state_t *ntp, uint8_t mode)
{
    uint8_t status[32];
    status[0] = mode;                          /* operating mode           */
    status[1] = (uint8_t)ptp->mode;
    status[2] = ptp->skew.active ? 1 : 0;     /* skew active               */
    status[3] = (uint8_t)ptp->skew.profile;   /* skew profile              */
    /* current skew ns (int64 → 8 bytes) */
    int64_t skew = ptp->current_skew_ns;
    memcpy(&status[4], &skew, 8);
    /* observed GM priority1 */
    status[12] = ptp->observed_gm.grandmaster_priority1;
    status[13] = ptp->observed_gm.grandmaster_clock_quality.clock_class;
    /* spoofed GM priority1 */
    status[14] = ptp->spoofed_gm.grandmaster_priority1;
    status[15] = ptp->spoofed_gm.grandmaster_clock_quality.clock_class;
    /* frame counters */
    uint32_t capt = ptp->frames_captured;
    uint32_t mod = ptp->frames_modified;
    memcpy(&status[16], &capt, 4);
    memcpy(&status[20], &mod, 4);
    status[24] = (uint8_t)(ntp->requests_received & 0xFF);
    status[25] = (uint8_t)(ntp->responses_sent & 0xFF);
    ble_link_send_event(ble, EVT_STATUS, status, 26);
}

void ble_link_dispatch(ble_link_t *ble,
                       ptp_engine_state_t *ptp,
                       ntp_engine_state_t *ntp,
                       covert_state_t *covert,
                       uint8_t *mode_out)
{
    if (ble->rx_idx < 4 || ble->rx_expected == 0 || ble->rx_idx < ble->rx_expected)
        return;  /* no complete frame */

    /* Verify CRC */
    uint16_t payload_len = ble->rx_expected - 4;  /* minus SOF+LEN(2)+CRC */
    uint8_t crc = compute_crc(&ble->rx_buf[3], payload_len + 1);
    if (crc != ble->rx_buf[ble->rx_expected - 1]) {
        /* CRC fail — discard */
        ble->rx_idx = 0;
        ble->rx_expected = 0;
        ble_link_send_event(ble, EVT_ERROR, (uint8_t *)"CRC", 3);
        return;
    }

    uint8_t cmd = ble->rx_buf[3];
    uint8_t *payload = &ble->rx_buf[4];
    uint16_t plen = payload_len;

    switch (cmd) {
    case CMD_PING:
        ble_link_send_event(ble, EVT_PING_RESP, (uint8_t *)"PONG", 4);
        break;

    case CMD_GET_STATUS:
        send_status(ble, ptp, ntp, *mode_out);
        break;

    case CMD_SET_MODE: {
        if (plen >= 1) {
            uint8_t m = payload[0];
            if (m < MODE_COUNT) {
                ptp->mode = (op_mode_t)m;
                *mode_out = m;
            }
        }
        send_status(ble, ptp, ntp, *mode_out);
        break;
    }

    case CMD_SET_SKEW_PROFILE: {
        /* payload: [profile:1][offset_ns:8][rate_nsps:8]
         *          [jitter_amp_ns:4][sawtooth_ms:4][active:1] = 26 bytes */
        if (plen >= 26) {
            skew_config_t cfg;
            cfg.profile = (skew_profile_t)payload[0];
            memcpy(&cfg.offset_ns, &payload[1], 8);
            memcpy(&cfg.rate_nsps, &payload[9], 8);
            memcpy(&cfg.jitter_amp_ns, &payload[17], 4);
            memcpy(&cfg.sawtooth_period_ms, &payload[21], 4);
            cfg.active = payload[25];
            ptp_engine_set_skew(ptp, &cfg);
            ntp_engine_set_skew(ntp, &cfg);
        }
        send_status(ble, ptp, ntp, *mode_out);
        break;
    }

    case CMD_SET_SKEW_PRESET: {
        if (plen >= 1) {
            skew_config_t cfg;
            skew_gen_apply_preset(&cfg, (skew_preset_t)payload[0]);
            ptp_engine_set_skew(ptp, &cfg);
            ntp_engine_set_skew(ntp, &cfg);
        }
        break;
    }

    case CMD_BMCA_SPOOF: {
        /* payload: [priority1:1][clockClass:1][accuracy:1]
         *          [variance:2][priority2:1][domain:1] = 7 bytes */
        if (plen >= 7) {
            ptp->spoofed_gm.grandmaster_priority1 = payload[0];
            ptp->spoofed_gm.grandmaster_clock_quality.clock_class = payload[1];
            ptp->spoofed_gm.grandmaster_clock_quality.clock_accuracy = payload[2];
            ptp->spoofed_gm.grandmaster_clock_quality.clock_variance =
                (int16_t)((payload[3] << 8) | payload[4]);
            ptp->spoofed_gm.grandmaster_priority2 = payload[5];
            ptp->spoofed_gm.domain_number = payload[6];
        }
        break;
    }

    case CMD_BMCA_AUTO_WIN: {
        /* Auto-adjust to beat observed GM */
        if (ptp->observed_gm.grandmaster_priority1 > 0) {
            ptp->spoofed_gm.grandmaster_priority1 =
                ptp->observed_gm.grandmaster_priority1 - 1;
        } else {
            ptp->spoofed_gm.grandmaster_priority1 = 0;
            ptp->spoofed_gm.grandmaster_clock_quality.clock_class = 6;
        }
        break;
    }

    case CMD_COVERT_SEND: {
        if (plen > 0)
            covert_tx_queue(covert, payload, plen);
        break;
    }

    case CMD_COVERT_RECV_START:
    case CMD_COVERT_RECV_STOP:
        /* Handled in main loop by enabling/disabling capture flag */
        break;

    case CMD_GNSS_DISCIPLINE: {
        if (plen >= 1) {
            /* Toggle GNSS disciplining — actual hardware interaction
             * is handled in tcxo_drvr via pps handler */
        }
        break;
    }

    case CMD_TAMPER_THRESHOLD: {
        if (plen >= 2) {
            /* Forwarded to imux_tamper module */
            (void)payload;
        }
        break;
    }

    default:
        ble_link_send_event(ble, EVT_ERROR, (uint8_t *)"CMD", 3);
        break;
    }

    /* Reset RX buffer for next frame */
    ble->rx_idx = 0;
    ble->rx_expected = 0;
}