/*
 * ACOUSTIC-PHANTOM — BLE bridge implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * UART communication with the CYW20819 BLE module. Implements a simple
 * frame protocol for GATT commands and notifications. The CYW20819
 * handles the BLE link layer, encryption (LE Secure Connections), and
 * GATT server; this driver sends/receives application-level packets
 * over UART.
 */

#include <string.h>
#include "ble_bridge.h"
#include "registers.h"
#include "board.h"

/* ---- UART ring buffer -------------------------------------------------- */
#define UART_BUF_SIZE  512
static uint8_t  s_rx_buf[UART_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

static uint8_t  s_tx_buf[UART_BUF_SIZE];
static volatile uint16_t s_tx_head = 0;
static volatile uint16_t s_tx_tail = 0;

/* ---- Callbacks --------------------------------------------------------- */
static ble_connect_cb_t    s_on_connect = NULL;
static ble_disconnect_cb_t s_on_disconnect = NULL;
static ble_command_cb_t    s_on_command = NULL;

/* ---- Connection state -------------------------------------------------- */
static uint8_t s_connected = 0;

/* ---- Packet protocol ---------------------------------------------------
 *
 * Frame format:
 *   [SOF: 0xA5] [LEN: 2B LE] [OPCODE: 1B] [PAYLOAD: LEN-1 bytes] [CRC: 1B]
 *
 * CRC = XOR of all bytes from OPCODE to end of PAYLOAD.
 * ------------------------------------------------------------------------ */
#define SOF_BYTE    0xA5
#define MAX_PAYLOAD  240

static uint8_t s_packet_buf[256];
static uint8_t s_packet_idx = 0;
static uint8_t s_packet_len = 0;
static uint8_t s_packet_state = 0;  /* 0=SOF, 1=LEN_LO, 2=LEN_HI, 3=DATA, 4=CRC */

/* ---- UART ISR ---------------------------------------------------------- */
void USART1_IRQHandler(void)
{
    if (USART1->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART1->RDR;
        uint16_t next = (s_rx_head + 1) % UART_BUF_SIZE;
        if (next != s_rx_tail) {
            s_rx_buf[s_rx_head] = byte;
            s_rx_head = next;
        }
    }
    if ((USART1->ISR & USART_ISR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)) {
        if (s_tx_head != s_tx_tail) {
            USART1->TDR = s_tx_buf[s_tx_tail];
            s_tx_tail = (s_tx_tail + 1) % UART_BUF_SIZE;
        } else {
            /* No more data — disable TX interrupt */
            USART1->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

/* ---- UART send --------------------------------------------------------- */
static void uart_send_byte(uint8_t byte)
{
    uint16_t next = (s_tx_head + 1) % UART_BUF_SIZE;
    while (next == s_tx_tail) { }  /* wait for space */
    s_tx_buf[s_tx_head] = byte;
    s_tx_head = next;
    USART1->CR1 |= USART_CR1_TXEIE;  /* enable TX interrupt */
}

static void uart_send(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uart_send_byte(data[i]);
    }
}

/* ---- Send a BLE notification packet ------------------------------------ */
static void send_packet(uint8_t opcode, const uint8_t *payload, uint16_t len)
{
    if (len > MAX_PAYLOAD) len = MAX_PAYLOAD;

    uint8_t header[4];
    header[0] = SOF_BYTE;
    header[1] = (uint8_t)(len + 1);     /* LEN includes opcode + payload */
    header[2] = (uint8_t)((len + 1) >> 8);

    /* CRC = XOR of opcode and all payload bytes */
    uint8_t crc = opcode;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= payload[i];
    }

    uart_send(header, 3);

    uint8_t op = opcode;
    uart_send(&op, 1);

    if (len > 0) {
        uart_send(payload, len);
    }

    uart_send(&crc, 1);
}

/* ---- Parse received packets -------------------------------------------- */
static void process_byte(uint8_t byte)
{
    switch (s_packet_state) {
        case 0:  /* SOF */
            if (byte == SOF_BYTE) {
                s_packet_state = 1;
                s_packet_idx = 0;
            }
            break;
        case 1:  /* LEN low */
            s_packet_len = byte;
            s_packet_state = 2;
            break;
        case 2:  /* LEN high */
            s_packet_len |= (byte << 8);
            if (s_packet_len > MAX_PAYLOAD + 1) {
                s_packet_state = 0;  /* too large — reset */
            } else {
                s_packet_state = 3;
                s_packet_idx = 0;
            }
            break;
        case 3:  /* DATA (opcode + payload) */
            s_packet_buf[s_packet_idx++] = byte;
            if (s_packet_idx >= s_packet_len) {
                s_packet_state = 4;
            }
            break;
        case 4:  /* CRC */
            {
                uint8_t expected_crc = 0;
                for (int i = 0; i < s_packet_len; i++) {
                    expected_crc ^= s_packet_buf[i];
                }
                if (byte == expected_crc) {
                    /* Valid packet — dispatch */
                    uint8_t opcode = s_packet_buf[0];
                    uint8_t *payload = &s_packet_buf[1];
                    uint16_t payload_len = s_packet_len - 1;

                    /* Handle link-layer commands internally */
                    if (opcode == 0xF0) {
                        /* Connection event from BLE module */
                        s_connected = 1;
                        if (s_on_connect) s_on_connect();
                    } else if (opcode == 0xF1) {
                        /* Disconnection event */
                        s_connected = 0;
                        if (s_on_disconnect) s_on_disconnect();
                    } else if (s_on_command) {
                        s_on_command(opcode, payload, payload_len);
                    }
                }
                s_packet_state = 0;
            }
            break;
    }
}

/* ---- Public API -------------------------------------------------------- */
void ble_bridge_init(const char *passkey)
{
    /* Configure USART1: 115200 8N1, enable RX interrupt */
    USART1->CR1 = 0;
    USART1->BRR = (APB2_HZ / BLE_UART_BAUD);
    USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    /* Enable USART1 interrupt in NVIC */
    NVIC_ISER0 |= (1U << USART1_IRQn);

    /* Send init command to BLE module: set advertising name + passkey */
    uint8_t init_cmd[16];
    init_cmd[0] = 0x00;  /* CMD_INIT */
    memcpy(&init_cmd[1], "ACOUSTIC-PHANTOM", 16);
    send_packet(0xF2, init_cmd, 17);

    /* Set passkey for LE Secure Connections pairing */
    uint8_t pk_cmd[7];
    memcpy(pk_cmd, passkey, 6);
    pk_cmd[6] = 0;  /* numeric comparison mode */
    send_packet(0xF3, pk_cmd, 7);

    (void)passkey;
}

void ble_bridge_set_callbacks(ble_connect_cb_t on_connect,
                              ble_disconnect_cb_t on_disconnect,
                              ble_command_cb_t on_command)
{
    s_on_connect = on_connect;
    s_on_disconnect = on_disconnect;
    s_on_command = on_command;
}

void ble_bridge_poll(void)
{
    /* Process received bytes */
    while (s_rx_tail != s_rx_head) {
        uint8_t byte = s_rx_buf[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) % UART_BUF_SIZE;
        process_byte(byte);
    }
}

void ble_bridge_send_event(const classification_t *result)
{
    if (!s_connected) return;

    /* Pack event: [class_id(1)] [conf(4)] [timestamp(4)] [inter_event(4)]
     *             [top5_id(5)] [top5_conf(20)]  = 38 bytes */
    uint8_t buf[38];
    buf[0] = result->class_id;

    /* Pack floats as 4 bytes each (little-endian) */
    memcpy(&buf[1], &result->confidence, 4);
    memcpy(&buf[5], &result->timestamp_ms, 4);
    memcpy(&buf[9], &result->inter_event_ms, 4);

    memcpy(&buf[13], result->top5_id, 5);
    memcpy(&buf[18], result->top5_conf, 20);

    send_packet(BLE_NOTIF_EVENT, buf, 38);
}

void ble_bridge_send_audio(const int16_t *samples, uint16_t count)
{
    if (!s_connected) return;

    /* Send audio in chunks of 120 samples (240 bytes) */
    uint16_t offset = 0;
    while (offset < count) {
        uint16_t chunk = count - offset;
        if (chunk > 120) chunk = 120;

        uint8_t buf[244];
        buf[0] = (uint8_t)(offset & 0xFF);
        buf[1] = (uint8_t)(offset >> 8);
        buf[2] = (uint8_t)(chunk & 0xFF);
        buf[3] = (uint8_t)(chunk >> 8);
        memcpy(&buf[4], &samples[offset], chunk * sizeof(int16_t));

        send_packet(BLE_NOTIF_AUDIO, buf, 4 + chunk * 2);
        offset += chunk;
    }
}

void ble_bridge_send_status(uint8_t state, uint8_t profile,
                            uint32_t event_count, uint8_t battery)
{
    uint8_t buf[7];
    buf[0] = state;
    buf[1] = profile;
    memcpy(&buf[2], &event_count, 4);
    buf[6] = battery;
    send_packet(BLE_NOTIF_STATUS, buf, 7);
}

void ble_bridge_send_file_list(void)
{
    /* The actual file listing comes from the storage driver; this is
     * a placeholder that sends a request to the storage subsystem.
     * In a full implementation, we'd query storage_list_files() and
     * pack each entry. */
    uint8_t buf[2] = {0, 0};  /* file count = 0 (filled by storage) */
    send_packet(BLE_NOTIF_FILE_LIST, buf, 2);
}

void ble_bridge_send_file_chunk(uint32_t file_id, uint32_t offset,
                                const uint8_t *data, uint16_t len)
{
    if (!s_connected) return;

    uint8_t buf[252];
    memcpy(&buf[0], &file_id, 4);
    memcpy(&buf[4], &offset, 4);
    memcpy(&buf[8], data, len);
    send_packet(BLE_NOTIF_FILE_CHUNK, buf, 8 + len);
}

void ble_bridge_send_calibration_progress(uint8_t current, uint8_t total)
{
    uint8_t buf[2] = {current, total};
    send_packet(BLE_NOTIF_CALIB_PROGRESS, buf, 2);
}