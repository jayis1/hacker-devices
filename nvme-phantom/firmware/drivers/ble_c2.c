/*
 * drivers/ble_c2.c — Encrypted BLE 5.0 command & control (Nordic UART Service)
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Talks to an nRF52840-M2 module over UART4 at 1 Mbaud.  The module runs a
 * Nordic UART Service (NUS) GATT server that the companion app connects to.
 * Frames are encrypted with AES-256-CTR using a session key derived from a
 * pre-shared BLE pairing key (ECDH on the nRF52840 side).
 *
 * Frame format (after AES-256-CTR decryption):
 *   [0xAA][0x55][uint8 cmd][uint16 len][payload...][uint8 crc8][0x0D]
 *
 * This driver handles: UART4 init, RX ring buffer, frame assembly, AES
 * keystream (simplified — a real implementation uses the STM32H5 AES
 * peripheral; here we implement a lightweight XTEA-based stream cipher for
 * portability and to avoid importing a full AES library), and command
 * dispatch to the main.c handler.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- UART4 init -------------------------------------------------------- */

static void uart4_init(void)
{
    RCC_APB1LENR |= RCC_APB1LENR_UART4;
    RCC_AHB1ENR  |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOC;
    /* PA12 = TX (AF6), PA11 = RX (AF6) */
    volatile uint32_t *gpioa_moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpioa_afrh  = (volatile uint32_t *)(GPIOA_BASE + GPIO_AFRH_OFF);
    *gpioa_moder &= ~((3U << (11*2)) | (3U << (12*2)));
    *gpioa_moder |=  ((2U << (11*2)) | (2U << (12*2)));
    *gpioa_afrh  &= ~((0xFU << ((11-8)*4)) | (0xFU << ((12-8)*4)));
    *gpioa_afrh  |=  ((6U  << ((11-8)*4)) | (6U  << ((12-8)*4)));
    /* PC10 = BLE_RESET output */
    volatile uint32_t *gpioc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER_OFF);
    *gpioc_moder |= (1U << (10*2));
    volatile uint32_t *gpioc_odr   = (volatile uint32_t *)(GPIOC_BASE + GPIO_ODR_OFF);
    *gpioc_odr   |= (1U << 10);                /* hold reset high (idle)   */

    /* BRR = APB1 / baud = 125 MHz / 1 Mbaud = 125 */
    volatile uint32_t *uart_brr = (volatile uint32_t *)(UART4_BASE + USART_BRR);
    volatile uint32_t *uart_cr1 = (volatile uint32_t *)(UART4_BASE + USART_CR1);
    *uart_brr = 125;
    *uart_cr1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void uart4_putc(uint8_t c)
{
    volatile uint32_t *isr = (volatile uint32_t *)(UART4_BASE + USART_ISR);
    volatile uint32_t *tdr = (volatile uint32_t *)(UART4_BASE + USART_TDR);
    while (!(*isr & USART_ISR_TXE)) { }
    *tdr = c;
}

static int uart4_getc(uint8_t *c, uint32_t timeout_ticks)
{
    volatile uint32_t *isr = (volatile uint32_t *)(UART4_BASE + USART_ISR);
    volatile uint32_t *rdr = (volatile uint32_t *)(UART4_BASE + USART_RDR);
    while (!(*isr & USART_ISR_RXNE)) {
        if (timeout_ticks-- == 0) return -1;
    }
    *c = (uint8_t)*rdr;
    return 0;
}

/* ---- XTEA-CTR stream cipher (lightweight AES-256-CTR stand-in) --------- */

static uint32_t s_key[4] = { 0x4A415900, 0x49530031, 0x4E564D00, 0x50484100 };
                                        /* "JAYI\0S1\0NVM\0PHA\0" derived  */

static void xtea_encrypt_block(uint32_t v[2], const uint32_t key[4])
{
    uint32_t sum = 0, delta = 0x9E3779B9;
    for (int i = 0; i < 32; i++) {
        v[0] += ((v[1] << 4 ^ v[1] >> 5) + v[1]) ^ (sum + key[sum & 3]);
        sum  += delta;
        v[1] += ((v[0] << 4 ^ v[0] >> 5) + v[0]) ^ (sum + key[(sum >> 11) & 3]);
    }
}

static void ble_keystream(uint8_t *out, uint32_t nonce, uint16_t len)
{
    /* Generate `len` bytes of keystream by encrypting (nonce||counter) blocks. */
    uint16_t i = 0;
    uint32_t counter = 0;
    while (i < len) {
        uint32_t v[2] = { nonce, counter };
        xtea_encrypt_block(v, s_key);
        for (int b = 0; b < 8 && i < len; b++) {
            out[i++] = (uint8_t)(v[b / 4] >> ((b % 4) * 8));
        }
        counter++;
    }
}

void ble_set_key(const uint8_t key[16])
{
    for (int i = 0; i < 4; i++) {
        s_key[i] = (uint32_t)key[i*4]       | ((uint32_t)key[i*4+1] << 8) |
                   ((uint32_t)key[i*4+2] << 16) | ((uint32_t)key[i*4+3] << 24);
    }
}

/* ---- CRC-8 (same poly as tlp_engine) ----------------------------------- */

static uint8_t crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
    return crc;
}

/* ---- RX ring buffer + frame assembly ----------------------------------- */

#define BLE_RX_BUF  280
static uint8_t  s_rxbuf[BLE_RX_BUF];
static uint16_t s_rxlen = 0;
static uint32_t s_nonce = 0;

void ble_init(void)
{
    uart4_init();
    /* Reset the nRF52840 module */
    volatile uint32_t *gpioc_odr = (volatile uint32_t *)(GPIOC_BASE + GPIO_ODR_OFF);
    *gpioc_odr &= ~(1U << 10);                 /* RESET low   */
    board_delay_ms(10);
    *gpioc_odr |=  (1U << 10);                 /* RESET high  */
    board_delay_ms(50);                        /* module boot */
    s_rxlen = 0;
    s_nonce = 0;
}

/* Send a framed, encrypted command to the companion app (or a response). */
int ble_send(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    if (len > 240) return -1;
    uint8_t frame[4 + 240 + 1 + 2];
    uint8_t enc[240];
    frame[0] = 0xAA;
    frame[1] = 0x55;
    frame[2] = cmd;
    frame[3] = (uint8_t)len;
    frame[4] = (uint8_t)(len >> 8);
    /* encrypt payload */
    uint8_t ks[240];
    ble_keystream(ks, s_nonce, len);
    for (uint16_t i = 0; i < len; i++) enc[i] = payload[i] ^ ks[i];
    memcpy(frame + 5, enc, len);
    /* CRC over cmd+len+enc */
    uint8_t crc = crc8(frame + 2, 3 + len);
    frame[5 + len] = crc;
    frame[5 + len + 1] = 0x0D;
    for (uint16_t i = 0; i < 5 + len + 2; i++) uart4_putc(frame[i]);
    s_nonce++;
    return 0;
}

/* Poll for an incoming frame.  Returns 0 if a complete frame is ready,
 * -1 if no data, -2 on CRC error.  cmd/len/payload are filled on success. */
int ble_poll(uint8_t *cmd, uint16_t *len, uint8_t *payload, uint16_t max_len)
{
    uint8_t c;
    while (uart4_getc(&c, 1000) == 0) {
        if (s_rxlen == 0 && c != 0xAA) continue;        /* resync          */
        if (s_rxlen == 1 && c != 0x55) { s_rxlen = 0; continue; }
        if (s_rxlen < BLE_RX_BUF) s_rxbuf[s_rxlen++] = c;
        if (s_rxlen >= 5) {
            uint16_t plen = (uint16_t)s_rxbuf[3] | ((uint16_t)s_rxbuf[4] << 8);
            if (plen > 240) { s_rxlen = 0; continue; }
            if (s_rxlen == 5 + plen + 2) {               /* frame complete */
                if (s_rxbuf[5 + plen + 1] != 0x0D) { s_rxlen = 0; continue; }
                uint8_t crc = crc8(s_rxbuf + 2, 3 + plen);
                if (crc != s_rxbuf[5 + plen]) { s_rxlen = 0; return -2; }
                *cmd = s_rxbuf[2];
                *len = plen;
                /* decrypt payload */
                uint8_t ks[240];
                ble_keystream(ks, s_nonce, plen);
                for (uint16_t i = 0; i < plen && i < max_len; i++)
                    payload[i] = s_rxbuf[5 + i] ^ ks[i];
                s_nonce++;
                s_rxlen = 0;
                return 0;
            }
        }
    }
    return -1;
}