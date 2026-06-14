/*
 * ble_c2_driver.c — ShadowTap BLE Command & Control driver implementation
 */

#include "drivers/ble_c2_driver.h"
#include <string.h>

/* Static driver state */
static ble_c2_state_t c2_state;

/* Forward declarations for internal functions */
static void slip_encode_and_send(const uint8_t *data, uint16_t len);
static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len);
static void handle_command(const ble_c2_packet_t *pkt);
static void uart_put_byte(uint8_t byte);

/* ========== Public Functions ========== */

void ble_c2_init(void) {
    memset(&c2_state, 0, sizeof(c2_state));

    /* Configure LPUART1: 115200 bps, 8N1 */
    /* Baud = IPG_CLK / (OSR * SBR), OSR=16, SBR = 150000000/(16*115200) ≈ 81 */
    LPUART1->BAUD = (15U << LPUART_BAUD_OSR_SHIFT) | 81U;
    LPUART1->CTRL = LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK;

    /* Enable FIFOs */
    LPUART1->FIFO = LPUART_FIFO_TXFE_MASK | LPUART_FIFO_RXFE_MASK |
                    LPUART_FIFO_TXFLUSH_MASK | LPUART_FIFO_RXFLUSH_MASK;

    /* Flush */
    LPUART1->STAT = LPUART_STAT_OR_MASK; /* Clear overrun */

    c2_state.rx_escaping = 0;
    c2_state.tx_seq = 0;
}

void ble_c2_rx_byte(uint8_t byte) {
    /* SLIP decoder state machine */
    if (c2_state.rx_len >= sizeof(c2_state.rx_buf)) {
        /* Overflow — reset */
        c2_state.rx_len = 0;
        c2_state.rx_escaping = 0;
        return;
    }

    switch (byte) {
    case BLE_C2_SLIP_END:
        if (c2_state.rx_len > 0) {
            /* Complete packet received — process it */
            /* Create temporary packet structure for handling */
            if (c2_state.rx_len >= 4) { /* Minimum: opcode + seq + len */
                ble_c2_packet_t pkt;
                uint16_t copy_len = c2_state.rx_len;
                if (copy_len > sizeof(pkt)) copy_len = sizeof(pkt);
                memcpy(&pkt, c2_state.rx_buf, copy_len);

                /* Verify CRC16 */
                uint16_t payload_len = pkt.payload_len;
                if (payload_len > sizeof(pkt.payload)) payload_len = sizeof(pkt.payload);
                uint16_t expected_crc = crc16_ccitt(c2_state.rx_buf, 4 + payload_len);
                if (expected_crc == pkt.crc16) {
                    handle_command(&pkt);
                } else {
                    /* CRC error — send error response */
                    uint8_t err = 1; /* CRC failure */
                    ble_c2_send_response(BLE_C2_RSP_ERROR, &err, 1);
                }
            }
            c2_state.rx_len = 0;
        }
        c2_state.rx_escaping = 0;
        break;

    case BLE_C2_SLIP_ESC:
        c2_state.rx_escaping = 1;
        break;

    default:
        if (c2_state.rx_escaping) {
            c2_state.rx_escaping = 0;
            if (byte == BLE_C2_SLIP_ESC_END) {
                c2_state.rx_buf[c2_state.rx_len++] = BLE_C2_SLIP_END;
            } else if (byte == BLE_C2_SLIP_ESC_ESC) {
                c2_state.rx_buf[c2_state.rx_len++] = BLE_C2_SLIP_ESC;
            }
        } else {
            c2_state.rx_buf[c2_state.rx_len++] = byte;
        }
        break;
    }
}

void ble_c2_process(void) {
    /* Main-loop processing is handled in ble_c2_rx_byte ISR callback.
     * This function is for polling UART status and sending pending
     * notifications.
     */
    /* Check if UART TX is ready for more data */
    if (c2_state.tx_len > 0 && (LPUART1->STAT & LPUART_STAT_TDRE_MASK)) {
        /* Continue sending pending TX data if any */
    }
}

void ble_c2_send_response(uint8_t opcode, const uint8_t *payload, uint16_t len) {
    ble_c2_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.opcode = opcode;
    pkt.seq = c2_state.tx_seq++;
    pkt.payload_len = len;
    if (len > 0 && payload) {
        memcpy(pkt.payload, payload, len > sizeof(pkt.payload) ? sizeof(pkt.payload) : len);
    }

    /* Calculate CRC16 over opcode + seq + payload_len + payload */
    uint16_t crc_len = 4 + len;
    pkt.crc16 = crc16_ccitt((const uint8_t *)&pkt, crc_len);

    /* SLIP-encode and send */
    uint16_t total_len = 4 + len + 2; /* header + payload + crc */
    slip_encode_and_send((const uint8_t *)&pkt, total_len);
}

void ble_c2_notify(uint8_t event_type, const uint8_t *data, uint16_t len) {
    /* Build notification packet: [event_type, 0x00, len_hi, len_lo, data...] */
    uint8_t buf[BLE_C2_MAX_CMD_LEN];
    buf[0] = event_type;
    buf[1] = 0x00; /* Notification marker */
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;
    uint16_t total = 4 + len;
    if (total > sizeof(buf)) total = sizeof(buf);
    if (len > 0 && data) {
        memcpy(buf + 4, data, len);
    }
    slip_encode_and_send(buf, total);
}

void ble_c2_set_crypto(const uint8_t key[32], const uint8_t nonce[12]) {
    memcpy(c2_state.aes_key, key, 32);
    memcpy(c2_state.aes_nonce, nonce, 12);
    c2_state.encrypted = 1;
}

uint8_t ble_c2_is_connected(void) {
    return c2_state.connected;
}

/* ========== Internal Functions ========== */

static void uart_put_byte(uint8_t byte) {
    /* Wait for TX data register empty */
    uint32_t timeout = 100000U;
    while (!(LPUART1->STAT & LPUART_STAT_TDRE_MASK)) {
        if (--timeout == 0) return;
    }
    LPUART1->DATA = byte;
}

static void slip_encode_and_send(const uint8_t *data, uint16_t len) {
    uart_put_byte(BLE_C2_SLIP_END); /* Start delimiter */

    for (uint16_t i = 0; i < len; i++) {
        switch (data[i]) {
        case BLE_C2_SLIP_END:
            uart_put_byte(BLE_C2_SLIP_ESC);
            uart_put_byte(BLE_C2_SLIP_ESC_END);
            break;
        case BLE_C2_SLIP_ESC:
            uart_put_byte(BLE_C2_SLIP_ESC);
            uart_put_byte(BLE_C2_SLIP_ESC_ESC);
            break;
        default:
            uart_put_byte(data[i]);
            break;
        }
    }

    uart_put_byte(BLE_C2_SLIP_END); /* End delimiter */
}

static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000U) {
                crc = (crc << 1) ^ 0x1021U;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ========== Command Handlers ========== */

/* External references to main.c state */
extern uint8_t mitm_rule_count;

static void handle_command(const ble_c2_packet_t *pkt) {
    uint16_t plen = pkt->payload_len;

    switch (pkt->opcode) {
    case BLE_C2_CMD_PING:
        ble_c2_send_response(BLE_C2_RSP_PONG, NULL, 0);
        break;

    case BLE_C2_CMD_GET_STATUS: {
        /* Return system status: link states, mode, capture, rule count */
        uint8_t status[8] = {0};
        status[0] = 1; /* Always "running" */
        status[1] = mitm_rule_count;
        status[2] = 0; /* capture state (placeholder) */
        status[3] = 0; /* mode (placeholder) */
        ble_c2_send_response(BLE_C2_RSP_STATUS, status, 8);
        break;
    }

    case BLE_C2_CMD_ADD_RULE: {
        /* Payload: [type, match_offset(2), match_mask(4), match_value(4), replace_data...] */
        if (plen >= 11 && mitm_rule_count < MITM_RULE_MAX) {
            mitm_rule_count++;
            uint8_t resp = mitm_rule_count; /* Rule ID */
            ble_c2_send_response(BLE_C2_RSP_RULE_ADDED, &resp, 1);
        } else {
            uint8_t err = 2; /* Rule table full or bad payload */
            ble_c2_send_response(BLE_C2_RSP_ERROR, &err, 1);
        }
        break;
    }

    case BLE_C2_CMD_DEL_RULE: {
        if (plen >= 1) {
            uint8_t rule_id = pkt->payload[0];
            /* Mark rule as disabled (simplified) */
            uint8_t resp = rule_id;
            ble_c2_send_response(BLE_C2_RSP_RULE_DEL, &resp, 1);
        }
        break;
    }

    case BLE_C2_CMD_START_CAP:
        ble_c2_send_response(BLE_C2_RSP_CAP_START, NULL, 0);
        break;

    case BLE_C2_CMD_STOP_CAP:
        ble_c2_send_response(BLE_C2_RSP_CAP_STOP, NULL, 0);
        break;

    case BLE_C2_CMD_SET_MODE: {
        /* Payload: [mode: 0=tap, 1=mitm] */
        uint8_t mode_ack = (plen >= 1) ? pkt->payload[0] : 0;
        ble_c2_send_response(BLE_C2_RSP_STATUS, &mode_ack, 1);
        break;
    }

    default: {
        uint8_t err = 3; /* Unknown command */
        ble_c2_send_response(BLE_C2_RSP_ERROR, &err, 1);
        break;
    }
    }
}