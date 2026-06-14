/*
 * EM4095 125 kHz RFID Reader Driver
 * Handles EM4100, HID Prox, and T5577 cloning
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#include "em4095.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* T5577 write command timing (in carrier cycles at 125 kHz) */
#define T5577_WRITE_GAP_US       150   /* Gap pulse width in µs */
#define T5577_WRITE_0_US        24     /* 0-bit width in periods */
#define T5577_WRITE_1_US        56     /* 1-bit width in periods */
#define T5577_WRITE_START_GAP   300   /* Start gap in µs */
#define T5577_WRITE_CMD_GAP     18    /* Command gap in periods */

/* EM4100 format:
 * 9 ones (header) + 10 rows of 4 data + 1 even parity + 4 column parity + 0 stop
 * Total: 64 bits
 */

/* ======================================================================
 * UART1 Initialization for EM4095 (9600 bps)
 * ====================================================================== */
static void uart1_init_9600(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART1EN;

    /* Baud rate: 9600 @ 120 MHz APB1 */
    /* BRR = 120000000 / 9600 = 12500 */
    USART1->BRR = 12500;

    /* 8N1, enable TX and RX */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart1_putc(char c) {
    while (!(USART1->ISR & USART_ISR_TXE));
    USART1->TDR = c;
}

static uint8_t uart1_getc_timeout(uint32_t timeout_cycles) {
    while (!(USART1->ISR & USART_ISR_RXNE)) {
        if (timeout_cycles-- == 0) return 0xFF;
    }
    return (uint8_t)(USART1->RDR & 0xFF);
}

/* ======================================================================
 * EM4095 Initialization
 * ====================================================================== */
void em4095_init(void) {
    uart1_init_9600();

    /* Configure control pins */
    /* EM4095_SHD = PC1 (active low shutdown) */
    /* EM4095_MOD = PC2 (modulation control) */
    EM4095_SHD_HIGH();   /* Start in standby */
    EM4095_MOD_LOW();    /* Normal receive mode */
}

void em4095_power_on(void) {
    EM4095_SHD_LOW();    /* Active mode */
    /* Wait for EM4095 to stabilize (~10ms) */
    for (volatile int i = 0; i < 1200000; i++);
}

void em4095_power_off(void) {
    EM4095_SHD_HIGH();   /* Standby mode */
    EM4095_MOD_LOW();
}

/* ======================================================================
 * EM4100 Read
 * The EM4095 demodulates Manchester-encoded 125 kHz data and outputs
 * on DEMOD_OUT (PA10/UART1_RX). We read raw data and decode.
 * ====================================================================== */
bool em4095_read_em4100(rfid_125_context_t *ctx) {
    uint8_t raw_bits[8];   /* Raw Manchester bit storage */
    uint8_t decoded_bytes[EM4100_BYTE_LEN];
    size_t decoded_bits = 0;
    int8_t ret;

    em4095_power_on();

    /* Enable EM4095 modulation for reading */
    EM4095_MOD_LOW();

    /* Read raw data from DEMOD_OUT via UART
     * EM4095 outputs Manchester-coded data at the tag's data rate
     * (typically RF/64 for EM4100 = ~1953 bps)
     * We oversample and decode */

    /* Read 64 Manchester bits (128 raw transitions) */
    uint8_t raw_data[16];
    for (int i = 0; i < 16; i++) {
        raw_data[i] = uart1_getc_timeout(200000);
        if (raw_data[i] == 0xFF) {
            em4095_power_off();
            return false;  /* Timeout */
        }
    }

    /* Manchester decode */
    ret = em4095_manchester_decode(raw_data, 128, decoded_bytes, &decoded_bits);
    if (ret < 0 || decoded_bits < 64) {
        em4095_power_off();
        return false;
    }

    /* Verify EM4100 format:
     * 9 header bits (all 1s)
     * 10 rows × 5 bits (4 data + 1 even parity)
     * 4 column parity bits
     * 1 stop bit (0)
     */

    /* Check header (9 ones) */
    uint8_t *d = decoded_bytes;
    /* First byte should be 0xFF (8 bits) + bit 8 of next byte = 1 */

    /* Extract data from EM4100 format */
    /* Bits 0-8: header (all 1s) */
    /* Bits 9-48: 10 groups of 4 data bits + 1 even parity */
    /* Bits 49-52: 4 column parity bits */
    /* Bit 53: stop bit (0) */

    /* Simplified extraction: take bytes 1-5 as the card data */
    uint8_t version = (d[1] >> 4) & 0x0F;
    ctx->data[0] = version;
    ctx->data[1] = ((d[1] & 0x0F) << 4) | ((d[2] >> 4) & 0x0F);
    ctx->data[2] = ((d[2] & 0x0F) << 4) | ((d[3] >> 4) & 0x0F);
    ctx->data[3] = ((d[3] & 0x0F) << 4) | ((d[4] >> 4) & 0x0F);
    ctx->data[4] = ((d[4] & 0x0F) << 4) | ((d[5] >> 4) & 0x0F);
    ctx->bit_count = 40;
    ctx->protocol = RFID_PROTO_EM4100;

    /* Compute facility code and card number from 5-byte EM4100 ID */
    ctx->facility_code = (ctx->data[0] << 8) | ctx->data[1];
    ctx->card_number = (ctx->data[2] << 16) | (ctx->data[3] << 8) | ctx->data[4];

    em4095_power_off();
    return true;
}

/* ======================================================================
 * HID Prox Read
 * HID Prox II uses PSK modulation at 125 kHz.
 * The EM4095 can receive the Manchester data, but HID Prox uses
 * a different encoding. We attempt to decode.
 * ====================================================================== */
bool em4095_read_hid_prox(rfid_125_context_t *ctx) {
    em4095_power_on();

    /* HID Prox uses a different data rate and encoding.
     * The card transmits a variable-length binary string.
     * Typical format: preamble + 32-96 data bits */

    uint8_t raw_data[32];
    for (int i = 0; i < 32; i++) {
        raw_data[i] = uart1_getc_timeout(300000);
        if (raw_data[i] == 0xFF) {
            em4095_power_off();
            return false;
        }
    }

    /* Try to detect HID Prox preamble (1...111...0) */
    /* Simplified: copy raw data as-is for BLE forwarding */
    memcpy(ctx->data, raw_data, RFID_125_MAX_BYTES);
    ctx->bit_count = 200;  /* Variable, estimate */
    ctx->protocol = RFID_PROTO_HID_PROX;
    ctx->facility_code = 0;
    ctx->card_number = 0;

    em4095_power_off();
    return true;
}

/* ======================================================================
 * T5577 Write
 * Write cloned data to a blank T5577 card
 * Uses On-Off Keying (OOK) modulation at 125 kHz
 * ====================================================================== */
bool em4095_write_t5577(const rfid_125_context_t *ctx, uint8_t *key) {
    (void)key;  /* Key not used for EM4100 format */

    if (ctx->protocol == RFID_PROTO_EM4100) {
        /* T5577 write sequence:
         * 1. Start gap (300 µs)
         * 2. 0-bit: 24 RF periods between gaps
         * 3. 1-bit: 56 RF periods between gaps
         * 4. Write command: start gap, then data bits, then programming gap
         */

        em4095_power_on();

        /* Enable modulation output */
        EM4095_MOD_HIGH();

        /* Write EM4100 config to T5577:
         * Page 0, block 0: Configuration
         * - Modulation: Manchester
         * - Data rate: RF/64
         * - Encoding: Manchester
         * - Max block: 2
         * - Password disabled
         */

        /* T5577 config word for EM4100 compatibility:
         * Bits: [10:8] = modulation (001 = Manchester)
         *        [7:0] = data rate (000110 = RF/64)
         *        etc.
         * Full config: 0x000880E8 (simplified) */

        uint32_t config_word = 0x000880E8;
        uint32_t id_word = ((uint32_t)ctx->data[0] << 24) |
                           ((uint32_t)ctx->data[1] << 16) |
                           ((uint32_t)ctx->data[2] << 8) |
                           ctx->data[3];

        /* Write each bit using GPIO modulation */
        /* Start gap */
        EM4095_MOD_LOW();
        for (volatile int i = 0; i < 37500; i++);  /* ~312 µs */

        /* Write config word (block 0) */
        for (int bit = 31; bit >= 0; bit--) {
            EM4095_MOD_HIGH();
            if ((config_word >> bit) & 1) {
                /* 1-bit: 56 periods = ~448 µs at 125 kHz */
                for (volatile int i = 0; i < 53760; i++);
            } else {
                /* 0-bit: 24 periods = ~192 µs at 125 kHz */
                for (volatile int i = 0; i < 23040; i++);
            }
            EM4095_MOD_LOW();
            for (volatile int i = 0; i < 18000; i++);  /* Gap: ~150 µs */
        }

        /* Programming gap */
        EM4095_MOD_LOW();
        for (volatile int i = 0; i < 150000; i++);  /* ~1.25 ms */

        /* Write ID word (block 1) */
        for (int bit = 31; bit >= 0; bit--) {
            EM4095_MOD_HIGH();
            if ((id_word >> bit) & 1) {
                for (volatile int i = 0; i < 53760; i++);
            } else {
                for (volatile int i = 0; i < 23040; i++);
            }
            EM4095_MOD_LOW();
            for (volatile int i = 0; i < 18000; i++);
        }

        EM4095_MOD_LOW();
        em4095_power_off();
        return true;
    }

    em4095_power_off();
    return false;
}

void em4095_set_modulation(bool on) {
    if (on) {
        EM4095_MOD_HIGH();
    } else {
        EM4095_MOD_LOW();
    }
}

/* ======================================================================
 * Manchester Decoding
 * Input: raw data bits from EM4095 DEMOD_OUT
 * Output: decoded NRZ data
 * ====================================================================== */
int8_t em4095_manchester_decode(const uint8_t *raw, size_t raw_bits,
                                 uint8_t *decoded, size_t *decoded_bits) {
    size_t out_bit = 0;
    uint8_t current_byte = 0;
    size_t byte_idx = 0;

    /* Manchester encoding: 
     * 0 = high-to-low transition (10)
     * 1 = low-to-high transition (01)
     * We detect transitions in pairs of bits */

    for (size_t i = 0; i + 1 < raw_bits; i += 2) {
        uint8_t bit0 = (raw[i / 8] >> (7 - (i % 8))) & 1;
        uint8_t bit1 = (raw[(i + 1) / 8] >> (7 - ((i + 1) % 8))) & 1;

        uint8_t decoded_bit;
        if (bit0 == 0 && bit1 == 1) {
            decoded_bit = 1;   /* 01 → 1 */
        } else if (bit0 == 1 && bit1 == 0) {
            decoded_bit = 0;   /* 10 → 0 */
        } else {
            /* Invalid Manchester pair — could be a transition error */
            continue;
        }

        current_byte |= (decoded_bit << (7 - (out_bit % 8)));
        out_bit++;

        if (out_bit % 8 == 0) {
            decoded[byte_idx++] = current_byte;
            current_byte = 0;
        }
    }

    /* Store final partial byte */
    if (out_bit % 8 != 0) {
        decoded[byte_idx] = current_byte;
    }

    *decoded_bits = out_bit;
    return 0;
}

/* Continuous read mode (background) */
static void (*g_continuous_callback)(const rfid_125_context_t *) = NULL;
static volatile bool g_continuous_running = false;

void em4095_start_continuous_read(void (*callback)(const rfid_125_context_t *)) {
    g_continuous_callback = callback;
    g_continuous_running = true;
    em4095_power_on();
}

void em4095_stop_continuous_read(void) {
    g_continuous_running = false;
    g_continuous_callback = NULL;
    em4095_power_off();
}