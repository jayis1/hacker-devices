/*
 * aperture-phantom / firmware / drivers / onewire.c
 *
 * Bit-banged 1-wire master for the DS28E07 secure element used to
 * authenticate the interposer to the companion app. The app issues a
 * challenge (32 bytes), the DS28E07 computes a SHA-256 MAC keyed with a
 * device secret burned at provisioning, and the app verifies it against
 * the expected response. The DS28E07's 64-bit ROM serial is also read
 * out for the device identity shown in the UI.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"

#define ONEWIRE_PIN  PIN_DS28E07_DQ
#define ONEWIRE_PORT GPIOC_BASE

extern volatile uint32_t g_ticks_ms;

static void ow_delay_us(uint32_t us) {
    /* rough: spin loop, tuned for 240 MHz core. ~8 cycles/us. */
    uint32_t n = us * 40u;
    while (n--) { __asm__("nop"); }
}

static void ow_drive_low(void) {
    GPIO_MODER(ONEWIRE_PORT) &= ~(3u << (2*ONEWIRE_PIN));
    GPIO_MODER(ONEWIRE_PORT) |=  (1u << (2*ONEWIRE_PIN)); /* output */
    GPIO_CLR(ONEWIRE_PORT, ONEWIRE_PIN);
}
static void ow_release(void) {
    GPIO_MODER(ONEWIRE_PORT) &= ~(3u << (2*ONEWIRE_PIN)); /* input */
    GPIO_PUPDR(ONEWIRE_PORT) |=  (1u << (2*ONEWIRE_PIN)); /* pull-up */
}
static uint8_t ow_read(void) {
    return (GPIO_IDR(ONEWIRE_PORT) >> ONEWIRE_PIN) & 1u;
}

static int ow_reset(void) {
    ow_drive_low();
    ow_delay_us(480);
    ow_release();
    ow_delay_us(70);
    uint8_t p = ow_read();
    ow_delay_us(410);
    return p ? -1 : 0; /* presence pulse pulls low */
}

static void ow_write_bit(uint8_t b) {
    if (b) {
        ow_drive_low(); ow_delay_us(6); ow_release(); ow_delay_us(64);
    } else {
        ow_drive_low(); ow_delay_us(60); ow_release(); ow_delay_us(10);
    }
}
static uint8_t ow_read_bit(void) {
    ow_drive_low(); ow_delay_us(6); ow_release(); ow_delay_us(9);
    uint8_t b = ow_read();
    ow_delay_us(55);
    return b;
}

static void ow_write_byte(uint8_t b) {
    for (int i = 0; i < 8; i++) { ow_write_bit(b & 1); b >>= 1; }
}
static uint8_t ow_read_byte(void) {
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) b |= (ow_read_bit() << i);
    return b;
}

int onewire_init(void) {
    ow_release();
    ow_delay_us(1000);
    if (ow_reset() != 0) return -1;
    return 0;
}

int onewire_read_serial(uint8_t *serial8) {
    if (ow_reset() != 0) return -1;
    ow_write_byte(0x33); /* READ ROM */
    for (int i = 0; i < 8; i++) serial8[i] = ow_read_byte();
    return 0;
}

int onewire_challenge(uint8_t *challenge, uint8_t *response) {
    /* DS28E07 Compute MAC: ROM id + challenge + secret → 32-byte MAC.
     * Command sequence: SKIP ROM (0xCC), COMPUTE MAC (0xA5), challenge
     * (32 bytes), read 32-byte MAC. AN number + padding handled by chip. */
    if (ow_reset() != 0) return -1;
    ow_write_byte(0xCC);  /* skip ROM */
    ow_write_byte(0xA5);  /* compute MAC */
    for (int i = 0; i < 32; i++) ow_write_byte(challenge[i]);
    ow_delay_us(2000);   /* SHA computation time */
    /* Read 32 bytes MAC + 2 status bytes. */
    for (int i = 0; i < 32; i++) response[i] = ow_read_byte();
    /* discard status */
    (void)ow_read_byte(); (void)ow_read_byte();
    return 0;
}