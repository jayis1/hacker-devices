/*
 * ble_c2.c — BLE C2 protocol (framed, AES-256-CTR encrypted)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The nRF52840 module runs the Nordic SoftDevice BLE stack and presents
 * a custom GATT service with a write characteristic (commands) and a
 * notify characteristic (captures + status). The MCU talks to the
 * nRF52840 over USART3 using a simple framed protocol:
 *
 *   [0xA5][len_hi][len_lo][payload (len bytes)][crc8]
 *
 * The payload is encrypted with AES-256-CTR using a session key derived
 * from an ECDH P-256 handshake done at pairing time. This file handles
 * framing + the MCU side of the UART link.
 */

#include "ble_c2.h"
#include "board.h"
#include "registers.h"
#include "crypto.h"
#include <string.h>

/* ----------------------------------------------------------------------- */
/*  UART3 low-level                                                         */
/* ----------------------------------------------------------------------- */

static void uart3_putc(uint8_t b) {
    while ((USART3->ISR & USART_ISR_TXE) == 0u) { /* spin */ }
    USART3->TDR = (uint32_t)b;
}

static int uart3_getc(uint8_t *b) {
    if ((USART3->ISR & USART_ISR_RXNE) == 0u) return -1;
    *b = (uint8_t)USART3->RDR;
    return 0;
}

static void uart3_hw_init(void) {
    /* Enable USART3 clock (APB1L). RCC APB1LENR bit 18. */
    volatile uint32_t *apb1lenr = (volatile uint32_t *)(RCC_BASE + 0x0C0u);
    *apb1lenr |= (1u << 18);

    /* BRR = PCLK1 / baud = 120e6 / 1e6 = 120 for 1 Mbaud */
    USART3->BRR = 120u;
    USART3->CR2 = 0u;   /* 1 stop bit */
    USART3->CR3 = USART_CR3_RTSE | USART_CR3_CTSE;  /* hardware flow control */
    USART3->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;
}

/* ----------------------------------------------------------------------- */
/*  State                                                                   */
/* ----------------------------------------------------------------------- */

static int      g_ble_connected = 0;
static uint8_t  g_rx_ring[256];
static uint16_t g_rx_head = 0u;
static uint16_t g_rx_tail = 0u;

/* ----------------------------------------------------------------------- */
/*  Init + advertising                                                       */
/* ----------------------------------------------------------------------- */

void ble_c2_init(void) {
    uart3_hw_init();

    /* De-assert nRF52840 reset */
    GPIOC->BSRR = (1u << 14u);

    /* Wait for the nRF module to boot — it will assert RTS when ready. */
    board_delay_ms(50);

    /* Send a soft reset command to the nRF52840 over the framed protocol.
     * The nRF firmware (also by jayis1) implements the GATT service and
     * the UART framing. */
    uint8_t cmd[2] = { 0x00u, 0x00u };  /* SOFT_RESET opcode 0x00 */
    ble_c2_send_raw(cmd, 2u);

    /* Derive a session key via ECDH (placeholder; real handshake is in
     * crypto.c). */
    crypto_derive_session_key();
}

void ble_c2_start_advertising(const char *name) {
    /* Build an "START_ADVERTISING" command with the device name. */
    uint8_t buf[40];
    uint16_t n = 0u;
    buf[n++] = 0x01u;  /* opcode: START_ADVERTISING */
    while (*name && n < 38u) buf[n++] = (uint8_t)*name++;
    ble_c2_send_raw(buf, n);
}

int ble_c2_is_connected(void) {
    return g_ble_connected;
}

/* ----------------------------------------------------------------------- */
/*  CRC-8 (poly 0x07, init 0xFF)                                             */
/* ----------------------------------------------------------------------- */

static uint8_t crc8(const uint8_t *data, uint16_t len) {
    uint8_t c = 0xFFu;
    for (uint16_t i = 0u; i < len; i++) {
        c ^= data[i];
        for (int b = 0; b < 8; b++) {
            c = (c & 0x80u) ? (uint8_t)((c << 1) ^ 0x07u) : (uint8_t)(c << 1);
        }
    }
    return c;
}

/* ----------------------------------------------------------------------- */
/*  Framed send                                                             */
/* ----------------------------------------------------------------------- */

static void send_frame(const uint8_t *payload, uint16_t len) {
    /* Encrypt the payload in-place with AES-256-CTR. */
    static uint8_t tx_nonce[12];
    crypto_ctr_increment(tx_nonce, 12);
    static uint8_t enc[256];
    uint16_t n = len > 256u ? 256u : len;
    crypto_aes_ctr_encrypt(payload, enc, n, tx_nonce);

    /* Frame: 0xA5 | len_hi | len_lo | enc[] | crc8 */
    uint8_t hdr[3] = { 0xA5u, (uint8_t)(n >> 8), (uint8_t)(n & 0xFFu) };
    uint8_t crc = crc8(enc, n);

    uart3_putc(hdr[0]);
    uart3_putc(hdr[1]);
    uart3_putc(hdr[2]);
    for (uint16_t i = 0u; i < n; i++) uart3_putc(enc[i]);
    uart3_putc(crc);
}

void ble_c2_send_raw(const uint8_t *data, uint16_t len) {
    send_frame(data, len);
}

void ble_c2_send_status(const char *msg) {
    uint16_t n = 0u;
    while (msg[n] && n < 200u) n++;
    uint8_t buf[201];
    buf[0] = BLE_NOTIF_STATUS;
    for (uint16_t i = 0u; i < n; i++) buf[1u + i] = (uint8_t)msg[i];
    send_frame(buf, (uint16_t)(1u + n));
}

void ble_c2_send_frame(const pr_parsed_t *parsed) {
    uint8_t buf[270];
    uint16_t n = 0u;
    buf[n++] = BLE_NOTIF_FRAME;
    buf[n++] = parsed->protocol_id;
    buf[n++] = (uint8_t)(parsed->dst_addr & 0xFFu);
    buf[n++] = (uint8_t)((parsed->dst_addr >> 8) & 0xFFu);
    buf[n++] = parsed->function;
    buf[n++] = (uint8_t)(parsed->length & 0xFFu);
    uint16_t pl = parsed->length;
    if (pl > sizeof(parsed->payload)) pl = sizeof(parsed->payload);
    for (uint16_t i = 0u; i < pl; i++) buf[n++] = parsed->payload[i];
    send_frame(buf, n);
}

/* ----------------------------------------------------------------------- */
/*  Receive                                                                 */
/* ----------------------------------------------------------------------- */

uint16_t ble_c2_poll_rx(uint8_t *buf, uint16_t max, uint16_t *out_len) {
    /* Suck any available bytes from UART into the ring buffer. */
    uint8_t b;
    while (uart3_getc(&b) == 0) {
        g_rx_ring[g_rx_head++] = b;
        g_rx_head &= 0xFFu;
    }

    /* Look for a complete frame: 0xA5 | len_hi | len_lo | enc[] | crc8 */
    if (g_rx_head == g_rx_tail) { *out_len = 0u; return 0u; }
    /* Need at least 4 bytes to know the length. */
    uint16_t avail = (uint16_t)((g_rx_head - g_rx_tail) & 0xFFu);
    if (avail < 4u) { *out_len = 0u; return 0u; }

    uint16_t idx = g_rx_tail;
    if (g_rx_ring[idx] != 0xA5u) {
        /* Resync: drop one byte and retry next poll. */
        g_rx_tail = (uint16_t)((g_rx_tail + 1u) & 0xFFu);
        *out_len = 0u;
        return 0u;
    }
    uint16_t flen = (uint16_t)((uint16_t)g_rx_ring[(idx + 1u) & 0xFFu] << 8)
                 | (uint16_t)g_rx_ring[(idx + 2u) & 0xFFu];
    if (flen > 250u) {
        /* Bogus length — resync. */
        g_rx_tail = (uint16_t)((g_rx_tail + 1u) & 0xFFu);
        *out_len = 0u;
        return 0u;
    }
    uint16_t need = (uint16_t)(4u + flen + 1u);  /* hdr(3) + flen + crc(1) — wait, hdr is 3 not 4 */
    /* hdr is 0xA5 + len_hi + len_lo = 3 bytes, then flen bytes, then crc1 = 3 + flen + 1 */
    need = (uint16_t)(3u + flen + 1u);
    if (avail < need) { *out_len = 0u; return 0u; }

    /* Copy out the encrypted payload. */
    static uint8_t enc[256];
    for (uint16_t i = 0u; i < flen; i++) {
        enc[i] = g_rx_ring[(idx + 3u + i) & 0xFFu];
    }
    uint8_t crc_recv = g_rx_ring[(idx + 3u + flen) & 0xFFu];
    uint8_t crc_calc = crc8(enc, flen);
    /* Advance the tail past this frame. */
    g_rx_tail = (uint16_t)((idx + 3u + flen + 1u) & 0xFFu);

    if (crc_recv != crc_calc) { *out_len = 0u; return 0u; }

    /* Decrypt into the caller's buffer. */
    static uint8_t rx_nonce[12];
    crypto_ctr_increment(rx_nonce, 12);
    uint16_t n = flen > max ? max : flen;
    crypto_aes_ctr_decrypt(enc, buf, n, rx_nonce);
    *out_len = n;

    /* Peek the decrypted opcode: 0xFE = "connection state changed" from nRF */
    if (n >= 2u && buf[0] == 0xFEu) {
        g_ble_connected = (buf[1] != 0u);
        return 0u;
    }
    return n;
}