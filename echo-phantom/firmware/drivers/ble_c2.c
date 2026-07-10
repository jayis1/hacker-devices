/*
 * ble_c2.c — BLE Command & Control Link for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Manages UART communication with the nRF52840 BLE module.
 * The nRF52840 handles the BLE 5.0 stack, encryption, and GATT
 * services. The MCU communicates with it via USART1.
 *
 * Protocol: framed packets with CRC-8
 *   [START(0xAA)] [CMD] [LEN] [payload...] [CRC8] [END(0x55)]
 *
 * The nRF52840 firmware translates BLE GATT writes into UART
 * frames to the MCU, and UART frames from the MCU into BLE
 * GATT notifications.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ========================================================================
 *  Frame protocol constants
 * ======================================================================== */

#define FRAME_START   0xAA
#define FRAME_END     0x55
#define FRAME_MAX     256

#define RX_BUF_SIZE   512

/* ========================================================================
 *  BLE state
 * ======================================================================== */

typedef struct {
    uint8_t  connected;
    uint8_t  rx_buf[RX_BUF_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;

    uint8_t  tx_buf[256];
    uint16_t tx_len;

    /* Frame reassembly */
    uint8_t  frame_buf[FRAME_MAX];
    uint16_t frame_idx;
    uint8_t  in_frame;
    uint8_t  frame_cmd;
    uint8_t  frame_len;
    uint16_t frame_crc_pos;
} ble_state_t;

static ble_state_t g_ble;

/* ========================================================================
 *  Initialize USART1 for BLE module communication
 * ======================================================================== */

void ble_c2_init(void)
{
    memset(&g_ble, 0, sizeof(g_ble));

    /* Configure USART1:
     *   Baud: 115200
     *   8N1, no flow control
     *   Enable RXNE interrupt
     */
    USART_CR1 = 0;  /* Disable USART */

    /* Baud rate = APB2 / baud = 120 MHz / 115200 = 1041.67 */
    USART_BRR = 120000000U / BLE_UART_BAUD;

    /* Configure: 8 data bits, no parity, 1 stop bit */
    USART_CR2 = 0;
    USART_CR3 = 0;

    /* Enable TX, RX, RXNE interrupt */
    USART_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable USART1 interrupt in NVIC */
    NVIC_ISER(1) |= BIT(IRQ_USART1 - 32);
}

/* ========================================================================
 *  Send data to BLE module (via UART)
 * ======================================================================== */

int ble_c2_send(const uint8_t *data, uint16_t len)
{
    if (len > 250)
        len = 250;  /* Frame max payload */

    /* Build frame: START | CMD | LEN | data | CRC | END */
    uint8_t frame[260];
    uint16_t idx = 0;

    frame[idx++] = FRAME_START;
    frame[idx++] = data[0];  /* Command code */
    frame[idx++] = (uint8_t)(len - 1);  /* Payload length (excluding CMD byte) */

    /* Copy payload (skip the first byte which is the command code) */
    for (uint16_t i = 1; i < len; i++)
        frame[idx++] = data[i];

    /* CRC over CMD + LEN + payload */
    uint8_t crc = crc8(&frame[1], idx - 1);
    frame[idx++] = crc;
    frame[idx++] = FRAME_END;

    /* Send via UART */
    for (uint16_t i = 0; i < idx; i++) {
        while (!(USART_ISR & USART_ISR_TXE))
            ;
        USART_TDR = frame[i];
    }

    return idx;
}

/* ========================================================================
 *  Receive a complete frame from BLE module (non-blocking)
 *
 *  Returns frame length (including CMD and payload) or 0 if no complete
 *  frame is available.
 * ======================================================================== */

int ble_c2_recv(uint8_t *buf, uint16_t maxlen)
{
    /* Check for new UART data */
    while (USART_ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART_RDR;

        /* Store in ring buffer */
        g_ble.rx_buf[g_ble.rx_head] = byte;
        g_ble.rx_head = (g_ble.rx_head + 1) % RX_BUF_SIZE;
    }

    /* Process ring buffer to find frames */
    while (g_ble.rx_tail != g_ble.rx_head) {
        uint8_t byte = g_ble.rx_buf[g_ble.rx_tail];
        g_ble.rx_tail = (g_ble.rx_tail + 1) % RX_BUF_SIZE;

        if (!g_ble.in_frame) {
            if (byte == FRAME_START) {
                g_ble.in_frame = 1;
                g_ble.frame_idx = 0;
            }
            continue;
        }

        /* In frame */
        if (g_ble.frame_idx == 0) {
            /* Command code */
            g_ble.frame_cmd = byte;
            g_ble.frame_buf[g_ble.frame_idx++] = byte;
        } else if (g_ble.frame_idx == 1) {
            /* Length */
            g_ble.frame_len = byte;
            g_ble.frame_buf[g_ble.frame_idx++] = byte;
        } else if (g_ble.frame_idx < (uint16_t)(2 + g_ble.frame_len)) {
            /* Payload */
            g_ble.frame_buf[g_ble.frame_idx++] = byte;
        } else if (g_ble.frame_idx == (uint16_t)(2 + g_ble.frame_len)) {
            /* CRC byte */
            uint8_t expected_crc = crc8(g_ble.frame_buf, g_ble.frame_idx);
            if (byte != expected_crc) {
                /* CRC error — discard frame */
                g_ble.in_frame = 0;
                g_ble.frame_idx = 0;
                continue;
            }
            g_ble.frame_buf[g_ble.frame_idx++] = byte;
        } else if (g_ble.frame_idx == (uint16_t)(3 + g_ble.frame_len)) {
            /* End byte */
            if (byte != FRAME_END) {
                /* Framing error */
                g_ble.in_frame = 0;
                g_ble.frame_idx = 0;
                continue;
            }

            /* Complete frame received */
            uint16_t frame_len = g_ble.frame_idx;  /* CMD + LEN + payload + CRC (no END) */
            if (maxlen >= frame_len) {
                memcpy(buf, g_ble.frame_buf, frame_len);
                g_ble.in_frame = 0;
                g_ble.frame_idx = 0;
                return frame_len;
            }
            g_ble.in_frame = 0;
            g_ble.frame_idx = 0;
        }
    }

    return 0;  /* No complete frame */
}

/* ========================================================================
 *  Poll BLE module (call periodically)
 * ======================================================================== */

void ble_c2_poll(void)
{
    /* Check connection status from nRF52840 (via GPIO or UART status) */
    /* For simplicity, assume always connected when UART is active */
    g_ble.connected = 1;
}

/* ========================================================================
 *  Check if BLE is connected
 * ======================================================================== */

uint8_t ble_c2_connected(void)
{
    return g_ble.connected;
}

/* ========================================================================
 *  Send a simple ACK response
 * ======================================================================== */

void ble_c2_send_ack(uint8_t cmd, uint8_t status)
{
    uint8_t resp[3];
    resp[0] = cmd;
    resp[1] = 0x01;   /* payload length = 1 */
    resp[2] = status;  /* 0 = OK */
    ble_c2_send(resp, 3);
}

/* ========================================================================
 *  Send a NACK response
 * ======================================================================== */

void ble_c2_send_nack(uint8_t cmd, uint8_t error)
{
    uint8_t resp[3];
    resp[0] = cmd;
    resp[1] = 0x01;
    resp[2] = error;
    ble_c2_send(resp, 3);
}