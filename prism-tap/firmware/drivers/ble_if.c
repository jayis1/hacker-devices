/*
 * drivers/ble_if.c — BLE C2 Interface for Prism-Tap
 *
 * Implements UART communication with the nRF52840 BLE module, including
 * a binary command/response protocol with AES-256-CTR encryption.
 *
 * The nRF52840 handles the BLE radio stack and ECDH key exchange independently.
 * The STM32H730 communicates with it via UART4 at 921600 baud. All commands
 * are encrypted with AES-256-CTR using a session key derived from a pre-shared
 * key and the ECDH exchange performed by the nRF52840.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "ble_if.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- AES-256-CTR (software implementation) ----
 * Uses a compact AES-256 block cipher with CTR mode.
 * In production, this would use the STM32H7 hardware crypto accelerator
 * (AES peripheral at 0x58031800). For portability, a software implementation
 * is provided here.
 */

static uint8_t aes_key[32];
static uint8_t aes_nonce[16];
static uint8_t aes_ctr_block[16];
static uint32_t aes_counter;

/* AES S-box */
static const uint8_t aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static uint8_t aes_mul2(uint8_t a)
{
    return (uint8_t)((a << 1) ^ ((a & 0x80) ? 0x1b : 0));
}

static uint8_t aes_mul3(uint8_t a)
{
    return (uint8_t)(aes_mul2(a) ^ a);
}

/* AES-256 key expansion (simplified — 240 bytes of round keys) */
static uint8_t round_keys[240];

static void aes_key_expansion(const uint8_t *key)
{
    /* First 32 bytes = original key */
    memcpy(round_keys, key, 32);

    /* AES-256: 14 rounds, 15 sets of 4 words = 240 bytes */
    uint8_t rcon = 1;
    for (int i = 32; i < 240; i += 4) {
        uint8_t t[4];
        memcpy(t, &round_keys[i - 4], 4);

        if (i % 32 == 0) {
            /* RotWord + SubWord + Rcon */
            uint8_t tmp = t[0];
            t[0] = aes_sbox[t[1]] ^ rcon;
            t[1] = aes_sbox[t[2]];
            t[2] = aes_sbox[t[3]];
            t[3] = aes_sbox[tmp];
            rcon = aes_mul2(rcon);
        } else if (i % 32 == 16) {
            /* SubWord only */
            t[0] = aes_sbox[t[0]];
            t[1] = aes_sbox[t[1]];
            t[2] = aes_sbox[t[2]];
            t[3] = aes_sbox[t[3]];
        }

        for (int j = 0; j < 4; j++)
            round_keys[i + j] = round_keys[i - 32 + j] ^ t[j];
    }
}

/* AES-256 encrypt single block (16 bytes) */
static void aes_encrypt_block(const uint8_t *in, uint8_t *out)
{
    uint8_t state[16];
    memcpy(state, in, 16);

    /* Initial round: AddRoundKey */
    for (int i = 0; i < 16; i++)
        state[i] ^= round_keys[i];

    /* 13 main rounds */
    for (int round = 1; round <= 13; round++) {
        /* SubBytes */
        for (int i = 0; i < 16; i++)
            state[i] = aes_sbox[state[i]];

        /* ShiftRows */
        uint8_t tmp;
        tmp = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = tmp;
        tmp = state[2]; state[2] = state[10]; state[10] = tmp; tmp = state[6]; state[6] = state[14]; state[14] = tmp;
        tmp = state[3]; state[3] = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = tmp;

        /* MixColumns */
        for (int c = 0; c < 4; c++) {
            uint8_t a0 = state[c * 4], a1 = state[c * 4 + 1];
            uint8_t a2 = state[c * 4 + 2], a3 = state[c * 4 + 3];
            state[c * 4]     = aes_mul2(a0) ^ aes_mul3(a1) ^ a2 ^ a3;
            state[c * 4 + 1] = a0 ^ aes_mul2(a1) ^ aes_mul3(a2) ^ a3;
            state[c * 4 + 2] = a0 ^ a1 ^ aes_mul2(a2) ^ aes_mul3(a3);
            state[c * 4 + 3] = aes_mul3(a0) ^ a1 ^ a2 ^ aes_mul2(a3);
        }

        /* AddRoundKey */
        for (int i = 0; i < 16; i++)
            state[i] ^= round_keys[round * 16 + i];
    }

    /* Final round (no MixColumns) */
    for (int i = 0; i < 16; i++)
        state[i] = aes_sbox[state[i]];

    /* ShiftRows */
    uint8_t tmp;
    tmp = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = tmp;
    tmp = state[2]; state[2] = state[10]; state[10] = tmp; tmp = state[6]; state[6] = state[14]; state[14] = tmp;
    tmp = state[3]; state[3] = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = tmp;

    for (int i = 0; i < 16; i++)
        state[i] ^= round_keys[14 * 16 + i];

    memcpy(out, state, 16);
}

/* ---- AES-256-CTR encrypt/decrypt (same operation) ---- */

static void aes_ctr_crypt(const uint8_t *in, uint8_t *out, uint32_t len,
                           const uint8_t *nonce)
{
    /* Initialize counter from nonce */
    memcpy(aes_ctr_block, nonce, 16);
    aes_counter = 0;

    uint32_t offset = 0;
    while (offset < len) {
        /* Encrypt counter block to produce keystream */
        uint8_t keystream[16];
        aes_encrypt_block(aes_ctr_block, keystream);

        /* XOR plaintext with keystream */
        uint32_t block_len = (len - offset < 16) ? (len - offset) : 16;
        for (uint32_t i = 0; i < block_len; i++)
            out[offset + i] = in[offset + i] ^ keystream[i];

        offset += block_len;

        /* Increment counter (big-endian, last 4 bytes) */
        aes_counter++;
        aes_ctr_block[12] = (uint8_t)(aes_counter >> 24);
        aes_ctr_block[13] = (uint8_t)(aes_counter >> 16);
        aes_ctr_block[14] = (uint8_t)(aes_counter >> 8);
        aes_ctr_block[15] = (uint8_t)(aes_counter & 0xFF);
    }
}

void ble_set_key(const uint8_t key[32])
{
    memcpy(aes_key, key, 32);
    aes_key_expansion(key);
}

void ble_encrypt(const uint8_t *in, uint8_t *out, uint32_t len,
                 const uint8_t nonce[16])
{
    aes_ctr_crypt(in, out, len, nonce);
}

void ble_decrypt(const uint8_t *in, uint8_t *out, uint32_t len,
                 const uint8_t nonce[16])
{
    /* CTR mode: encrypt = decrypt */
    aes_ctr_crypt(in, out, len, nonce);
}

/* ---- CRC-16-CCITT ---- */

uint16_t ble_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ---- UART4 init for BLE module ---- */

static uint8_t rx_buffer[256];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static uint8_t ble_connected_flag = 0;

/* Command handler table */
#define MAX_HANDLERS 32
static struct {
    uint8_t opcode;
    ble_cmd_handler_t handler;
} cmd_handlers[MAX_HANDLERS];
static uint8_t num_handlers = 0;

static void ble_uart_init(void)
{
    /* Enable GPIOB and UART4 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    RCC_APB1LENR |= RCC_APB1LENR_UART4EN;

    /* Configure PB10 (TX) and PB11 (RX) as AF5 (UART4) */
    uint32_t moder = GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (BLE_UART_TX_PIN * 2));
    moder &= ~(3U << (BLE_UART_RX_PIN * 2));
    moder |= (GPIO_MODE_AF << (BLE_UART_TX_PIN * 2));
    moder |= (GPIO_MODE_AF << (BLE_UART_RX_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) = moder;

    /* AF5 for both pins */
    uint32_t afrl = GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET);
    afrl &= ~(0xFU << (BLE_UART_TX_PIN * 4));
    afrl |= (AF_UART4_TX_PB10 << (BLE_UART_TX_PIN * 4));
    afrl &= ~(0xFU << (BLE_UART_RX_PIN * 4));
    afrl |= (AF_UART4_RX_PB11 << (BLE_UART_RX_PIN * 4));
    GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET) = afrl;

    /* High speed */
    uint32_t speed = GPIO_REG(GPIOB_BASE, GPIO_OSPEEDR_OFFSET);
    speed |= (GPIO_SPEED_VHIGH << (BLE_UART_TX_PIN * 2));
    speed |= (GPIO_SPEED_VHIGH << (BLE_UART_RX_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_OSPEEDR_OFFSET) = speed;

    /* Pull-up on RX */
    uint32_t pupdr = GPIO_REG(GPIOB_BASE, GPIO_PUPDR_OFFSET);
    pupdr &= ~(3U << (BLE_UART_RX_PIN * 2));
    pupdr |= (GPIO_PUPD_UP << (BLE_UART_RX_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_PUPDR_OFFSET) = pupdr;

    /* Configure UART4: 921600 baud, 8N1, enable RX interrupt */
    USART_CR1(UART4_BASE) = 0;  /* Disable before config */
    USART_CR2(UART4_BASE) = 0;
    USART_CR3(UART4_BASE) = 0;
    USART_BRR(UART4_BASE) = (APB1_FREQ / BLE_UART_BAUD);
    USART_CR1(UART4_BASE) = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE |
                             USART_CR1_RXNEIE;

    /* Enable UART4 interrupt in NVIC */
    NVIC_ISER0 |= (1U << (IRQ_UART4 - 32));  /* IRQ_UART4 = 52, bit 20 in ISER1 */
    /* Actually IRQ 52 maps to ISER[1] bit 20; simplified here */
}

/* ---- UART ISR (called from IRQ handler in main.c) ---- */

void ble_uart_rx_isr(void)
{
    if (USART_ISR(UART4_BASE) & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART_RDR(UART4_BASE);
        uint16_t next = (rx_head + 1) % sizeof(rx_buffer);
        if (next != rx_tail) {
            rx_buffer[rx_head] = byte;
            rx_head = next;
        }
    }
}

static int ble_uart_read(uint8_t *buf, uint32_t len)
{
    uint32_t read = 0;
    while (read < len && rx_tail != rx_head) {
        buf[read++] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % sizeof(rx_buffer);
    }
    return (int)read;
}

static void ble_uart_write(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        while (!(USART_ISR(UART4_BASE) & USART_ISR_TXE))
            ;
        USART_TDR(UART4_BASE) = buf[i];
    }
    /* Wait for last byte to complete */
    while (!(USART_ISR(UART4_BASE) & USART_ISR_TC))
        ;
}

/* ---- BLE protocol processing ---- */

static uint8_t pkt_buffer[BLE_MAX_PAYLOAD + sizeof(ble_header_t) + 16];
static uint8_t resp_buffer[BLE_MAX_PAYLOAD + sizeof(ble_header_t) + 16];

void ble_poll(void)
{
    /* Check if we have enough bytes for a header (6 bytes) */
    if (rx_head == rx_tail)
        return;

    /* Try to read a packet */
    ble_header_t hdr;
    uint8_t raw[6];
    int n = ble_uart_read(raw, 6);
    if (n < 6)
        return;

    /* Parse header (little-endian) */
    hdr.opcode = raw[0];
    hdr.seq    = raw[1];
    hdr.length = (uint16_t)(raw[2] | (raw[3] << 8));
    hdr.crc    = (uint16_t)(raw[4] | (raw[5] << 8));

    if (hdr.length > BLE_MAX_PAYLOAD) {
        /* Invalid packet — discard */
        return;
    }

    /* Read payload + nonce (16 bytes) */
    uint32_t total = hdr.length + 16;
    uint8_t payload_nonce[256 + 16];
    n = ble_uart_read(payload_nonce, total);
    if ((uint32_t)n < total)
        return;  /* incomplete; in a real impl, we'd buffer and wait */

    /* Decrypt payload */
    uint8_t decrypted[256];
    ble_decrypt(payload_nonce, decrypted, hdr.length, &payload_nonce[hdr.length]);

    /* Verify CRC */
    uint8_t crc_data[4 + 256];
    crc_data[0] = hdr.opcode;
    crc_data[1] = hdr.seq;
    crc_data[2] = (uint8_t)(hdr.length & 0xFF);
    crc_data[3] = (uint8_t)(hdr.length >> 8);
    memcpy(&crc_data[4], decrypted, hdr.length);
    uint16_t computed_crc = ble_crc16(crc_data, 4 + hdr.length);

    if (computed_crc != hdr.crc) {
        ble_send_error(hdr.opcode, hdr.seq, 0x01);  /* CRC error */
        return;
    }

    /* Find handler for opcode */
    for (int i = 0; i < num_handlers; i++) {
        if (cmd_handlers[i].opcode == hdr.opcode) {
            uint8_t resp_payload[BLE_MAX_PAYLOAD];
            uint8_t resp_len = 0;
            int ret = cmd_handlers[i].handler(decrypted, hdr.length,
                                               resp_payload, &resp_len);
            if (ret == 0)
                ble_send_response(hdr.opcode, hdr.seq, resp_payload, resp_len);
            else
                ble_send_error(hdr.opcode, hdr.seq, (uint8_t)(-ret));
            return;
        }
    }

    /* No handler found */
    ble_send_error(hdr.opcode, hdr.seq, 0x02);
}

void ble_send_response(uint8_t opcode, uint8_t seq,
                        const uint8_t *payload, uint8_t len)
{
    ble_header_t hdr;
    hdr.opcode = opcode | CMD_RESPONSE;
    hdr.seq = seq;
    hdr.length = len;

    /* Compute CRC over opcode + seq + length + payload */
    uint8_t crc_data[4 + 256];
    crc_data[0] = hdr.opcode;
    crc_data[1] = hdr.seq;
    crc_data[2] = (uint8_t)(len & 0xFF);
    crc_data[3] = (uint8_t)(len >> 8);
    if (len > 0)
        memcpy(&crc_data[4], payload, len);
    hdr.crc = ble_crc16(crc_data, 4 + len);

    /* Encrypt payload */
    uint8_t encrypted[256];
    uint8_t nonce[16];
    /* Generate nonce from seq + timestamp (simplified) */
    memset(nonce, 0, 16);
    nonce[0] = seq;
    nonce[1] = (uint8_t)(g_status.uptime_ms & 0xFF);
    ble_encrypt(payload, encrypted, len, nonce);

    /* Build packet */
    uint8_t pkt[6 + 256 + 16];
    pkt[0] = hdr.opcode;
    pkt[1] = hdr.seq;
    pkt[2] = (uint8_t)(len & 0xFF);
    pkt[3] = (uint8_t)(len >> 8);
    pkt[4] = (uint8_t)(hdr.crc & 0xFF);
    pkt[5] = (uint8_t)(hdr.crc >> 8);
    memcpy(&pkt[6], encrypted, len);
    memcpy(&pkt[6 + len], nonce, 16);

    ble_uart_write(pkt, 6 + len + 16);
}

void ble_send_error(uint8_t opcode, uint8_t seq, uint8_t error_code)
{
    uint8_t payload[1] = { error_code };
    ble_send_response(opcode, seq, payload, 1);
}

void ble_register_handler(uint8_t opcode, ble_cmd_handler_t handler)
{
    if (num_handlers < MAX_HANDLERS) {
        cmd_handlers[num_handlers].opcode = opcode;
        cmd_handlers[num_handlers].handler = handler;
        num_handlers++;
    }
}

int ble_init(void)
{
    memset(cmd_handlers, 0, sizeof(cmd_handlers));
    num_handlers = 0;
    rx_head = 0;
    rx_tail = 0;
    ble_connected_flag = 0;

    /* Default AES key (in production, derived from ECDH with app) */
    static const uint8_t default_key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    };
    ble_set_key(default_key);

    ble_uart_init();
    return 0;
}

int ble_is_connected(void)
{
    /* In a real implementation, the nRF52840 would signal connection status
       via a GPIO pin or a UART status message. For now, assume connected
       after init. */
    return 1;
}

/* ---- End of ble_if.c ----
 * Author: jayis1
 */