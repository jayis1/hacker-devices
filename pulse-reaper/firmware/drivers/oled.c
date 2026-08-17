/*
 * oled.c — SSD1306 128x64 OLED driver + UI primitives
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal I2C1 driver for the SSD1306 OLED. The display is 128x64 with
 * 8 pages of 128 bytes each. We use the built-in 5x7 font in "page
 * addressing mode" — each page is 8 rows, so 8 pages cover the full
 * 64-row height.
 */

#include "oled.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ----------------------------------------------------------------------- */
/*  I2C1 low-level                                                          */
/* ----------------------------------------------------------------------- */

static void i2c1_hw_init(void) {
    /* Enable I2C1 clock (APB1L bit 21) */
    volatile uint32_t *apb1lenr = (volatile uint32_t *)(RCC_BASE + 0x0C0u);
    *apb1lenr |= (1u << 21);

    /* TIMINGR for 400 kHz from 120 MHz PCLK: a standard value is 0x307075B1
     * for I2C fast mode. Use a conservative 0x10909CEC for 100 kHz. */
    I2C1->TIMINGR = 0x10909CECu;
    I2C1->CR1 = I2C_CR1_PE;
}

static int i2c1_write(uint8_t addr7, const uint8_t *data, uint16_t len) {
    /* Start + address (W) */
    I2C1->CR2 = ((uint32_t)(addr7 << 1) & 0xFFu)        /* address (W=0) */
             | ((uint32_t)len << I2C_CR2_NBYTES_SHIFT)
             | I2C_CR2_AUTOEND
             | I2C_CR2_START;
    for (uint16_t i = 0u; i < len; i++) {
        while ((I2C1->ISR & I2C_ISR_TXIS) == 0u) {
            if (I2C1->ISR & I2C_ISR_NACKF) return -1;
        }
        I2C1->TXDR = (uint32_t)data[i];
    }
    while ((I2C1->ISR & I2C_ISR_STOPF) == 0u) { /* spin */ }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  SSD1306 commands                                                        */
/* ----------------------------------------------------------------------- */

#define SSD_SET_CONTRAST       0x81u
#define SSD_DISPLAY_ON         0xAFu
#define SSD_DISPLAY_OFF        0xAEu
#define SSD_NORMAL_DISPLAY     0xA6u
#define SSD_SET_PAGE_ADDR      0xB0u
#define SSD_SET_LOW_COL        0x00u
#define SSD_SET_HIGH_COL       0x10u

static void ssd_cmd(uint8_t cmd) {
    uint8_t buf[2] = { 0x00u, cmd };   /* control byte 0x00 = command */
    i2c1_write(OLED_ADDR >> 1, buf, 2);
}

static void ssd_data(const uint8_t *data, uint16_t len) {
    /* Control byte 0x40 = data stream. We send the control byte first,
     * then the data. The I2C write above sends one transaction; for a
     * single control byte + data we need a single transaction with the
     * control byte as the first byte. */
    uint8_t buf[130];
    buf[0] = 0x40u;
    if (len > 129u) len = 129u;
    memcpy(buf + 1, data, len);
    i2c1_write(OLED_ADDR >> 1, buf, (uint16_t)(1u + len));
}

/* ----------------------------------------------------------------------- */
/*  Init                                                                    */
/* ----------------------------------------------------------------------- */

void oled_init(void) {
    i2c1_hw_init();
    board_delay_ms(50);   /* power-up delay */

    ssd_cmd(SSD_DISPLAY_OFF);
    ssd_cmd(0xD5u); ssd_cmd(0x80u);   /* display clock divide */
    ssd_cmd(0xA8u); ssd_cmd(0x3Fu);   /* multiplex 1/64 */
    ssd_cmd(0xD3u); ssd_cmd(0x00u);   /* display offset */
    ssd_cmd(0x40u);                   /* start line 0 */
    ssd_cmd(0x8Du); ssd_cmd(0x14u);   /* charge pump on */
    ssd_cmd(0x20u); ssd_cmd(0x00u);   /* page addressing mode */
    ssd_cmd(0xA1u);                   /* segment remap */
    ssd_cmd(0xC8u);                   /* COM scan direction */
    ssd_cmd(0xDAu); ssd_cmd(0x12u);   /* COM pins */
    ssd_cmd(0x81u); ssd_cmd(0xCFu);   /* contrast */
    ssd_cmd(0xD9u); ssd_cmd(0xF1u);   /* pre-charge period */
    ssd_cmd(0xDBu); ssd_cmd(0x40u);   /* VCOMH deselect */
    ssd_cmd(SSD_NORMAL_DISPLAY);
    ssd_cmd(SSD_DISPLAY_ON);
}

/* ----------------------------------------------------------------------- */
/*  5x7 font (printable ASCII subset)                                       */
/* ----------------------------------------------------------------------- */

#include "oled_font5x7.h"

static void oled_draw_char(uint8_t col, uint8_t page, char c) {
    if (c < 32 || c > 127) c = '?';
    uint8_t glyph[5];
    const uint8_t *g = &font5x7[(c - 32) * 5];
    for (int i = 0; i < 5; i++) glyph[i] = g[i];

    ssd_cmd((uint8_t)(SSD_SET_PAGE_ADDR + (page & 0x07u)));
    ssd_cmd((uint8_t)(SSD_SET_LOW_COL  | (col & 0x0Fu)));
    ssd_cmd((uint8_t)(SSD_SET_HIGH_COL | ((col >> 4) & 0x0Fu)));
    ssd_data(glyph, 5);
}

static void oled_draw_string(uint8_t col, uint8_t page, const char *s) {
    while (*s && col < 128) {
        oled_draw_char(col, page, *s);
        col += 6;
        s++;
    }
}

/* ----------------------------------------------------------------------- */
/*  Public UI                                                               */
/* ----------------------------------------------------------------------- */

void oled_clear(void) {
    for (uint8_t p = 0; p < 8; p++) {
        ssd_cmd((uint8_t)(SSD_SET_PAGE_ADDR + p));
        ssd_cmd(0x00u);
        ssd_cmd(0x10u);
        uint8_t zeros[128];
        memset(zeros, 0, sizeof(zeros));
        ssd_data(zeros, 128);
    }
}

void oled_show_status(const char *line1, const char *line2) {
    oled_clear();
    if (line1) oled_draw_string(0, 0, line1);
    if (line2) oled_draw_string(0, 2, line2);
}

void oled_show_capture_stats(uint32_t frames, uint32_t bytes) {
    char line2[22];
    int p = 0;
    const char *f = "F=";
    while (*f) line2[p++] = *f++;
    char tmp[12]; int ti = 0;
    if (frames == 0u) tmp[ti++] = '0';
    while (frames) { tmp[ti++] = (char)('0' + (frames % 10u)); frames /= 10u; }
    while (ti) line2[p++] = tmp[--ti];
    line2[p] = '\0';
    oled_show_status("SNIFF", line2);
    (void)bytes;  /* could also show bytes if needed */
}