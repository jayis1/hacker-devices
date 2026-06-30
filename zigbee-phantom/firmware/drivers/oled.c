/*
 * drivers/oled.c — SH1106 OLED driver (128x64, SPI) for ZIGBEE-PHANTOM
 * Author: jayis1
 * License: GPL-2.0
 *
 * Bit-banged SPI driver for the SH1106 1.3" OLED. Implements a small 6x8
 * ASCII font and status-line helpers used by the main loop to display
 * channel, RSSI, mode, and capture counters.
 */
#include "oled.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* SH1106 commands */
#define SH1106_DISP_OFF     0xAE
#define SH1106_DISP_ON      0xAF
#define SH1106_SET_CONTRAST 0x81
#define SH1106_NORM_DISP    0xA6
#define SH1106_INVERT_DISP  0xA7
#define SH1106_DISP_RAM     0xA4
#define SH1106_SET_PAGE     0xB0
#define SH1106_SET_COL_LO   0x00
#define SH1106_SET_COL_HI   0x10
#define SH1106_SEG_REMAP0   0xA0
#define SH1106_SEG_REMAP1   0xA1
#define SH1106_COM_NORM     0xC0
#define SH1106_COM_REMAP    0xC8

#define OLED_W 128
#define OLED_H 64
#define OLED_PAGES (OLED_H / 8)

static uint8_t vbuf[OLED_W * OLED_PAGES];
static uint8_t cur_col = 0, cur_row = 0;

/* Compact 6x8 ASCII font (printable 0x20..0x7E). Each glyph is 6 columns × 1
 * page (8 rows). Only a subset is included here; the full font lives in
 * font6x8.c (linked at build). We provide spaces, digits, A-Z, a-z, and
 * punctuation marks used by the status UI. */
extern const uint8_t font6x8[][6];

static void spi_wait(void)
{
    volatile uint32_t *sr = (volatile uint32_t *)(OLED_SPI_BASE + SSI_O_SR);
    while (!(*sr & SSI_SR_TNF)) { }   /* wait for TX FIFO not full */
}

static void spi_xfer(uint8_t dc, uint8_t data)
{
    volatile uint32_t *dr = (volatile uint32_t *)(OLED_SPI_BASE + SSI_O_DR);
    GPIO_SET(OLED_CS_DIO);
    if (dc) GPIO_SET(OLED_DC_DIO); else GPIO_CLR(OLED_DC_DIO);
    GPIO_CLR(OLED_CS_DIO);
    *dr = data;
    spi_wait();
    GPIO_SET(OLED_CS_DIO);
}

static void oled_cmd(uint8_t c){ spi_xfer(0, c); }
static void oled_data(uint8_t d){ spi_xfer(1, d); }

int oled_init(void)
{
    /* Configure SPI pins as outputs */
    GPIO_OUTPUT(OLED_CS_DIO);
    GPIO_OUTPUT(OLED_DC_DIO);
    GPIO_OUTPUT(OLED_RST_DIO);
    GPIO_OUTPUT(OLED_MOSI_DIO);
    GPIO_OUTPUT(OLED_CLK_DIO);

    /* Hardware reset */
    GPIO_CLR(OLED_RST_DIO);
    for (volatile int i=0;i<10000;i++);
    GPIO_SET(OLED_RST_DIO);
    for (volatile int i=0;i<10000;i++);

    oled_cmd(SH1106_DISP_OFF);
    oled_cmd(SH1106_SET_CONTRAST); oled_cmd(0xCF);
    oled_cmd(0xA1);  /* segment remap */
    oled_cmd(SH1106_COM_REMAP);  /* COM scan remap */
    oled_cmd(0xDA); oled_cmd(0x12);  /* com pins hardware config */
    oled_cmd(0xD5); oled_cmd(0x80);  /* display clock divide */
    oled_cmd(0xD9); oled_cmd(0xF1);  /* pre-charge period */
    oled_cmd(0xDB); oled_cmd(0x40);  /* VCOMH deselect */
    oled_cmd(0x8D); oled_cmd(0x14);  /* charge pump on */
    oled_cmd(SH1106_NORM_DISP);
    oled_cmd(SH1106_DISP_RAM);
    oled_cmd(SH1106_DISP_ON);

    oled_clear();
    return 0;
}

void oled_clear(void)
{
    memset(vbuf, 0, sizeof(vbuf));
    for (uint8_t p=0;p<OLED_PAGES;p++){
        oled_cmd(SH1106_SET_PAGE + p);
        oled_cmd(SH1106_SET_COL_LO); oled_cmd(SH1106_SET_COL_HI | 0x02);
        for (uint8_t c=0;c<OLED_W;c++) oled_data(vbuf[p*OLED_W + c]);
    }
    cur_col = 0; cur_row = 0;
}

void oled_power(bool on) { oled_cmd(on ? SH1106_DISP_ON : SH1106_DISP_OFF); }
void oled_invert(bool on){ oled_cmd(on ? SH1106_INVERT_DISP : SH1106_NORM_DISP); }
void oled_flip(bool on)  { oled_cmd(on ? SH1106_SEG_REMAP1 : SH1106_SEG_REMAP0);
                           oled_cmd(on ? SH1106_COM_REMAP  : SH1106_COM_NORM); }

void oled_set_cursor(uint8_t col, uint8_t row){ cur_col = col; cur_row = row; }

void oled_putc(char c)
{
    if (c < 0x20 || c > 0x7E) c = ' ';
    const uint8_t *g = font6x8[c - 0x20];
    for (uint8_t i=0;i<6;i++){
        uint8_t page = cur_row;
        uint8_t col  = cur_col + i + 2;
        if (col >= OLED_W) { col = 2 + i; cur_row = (cur_row + 1) % OLED_PAGES; page = cur_row; }
        vbuf[page*OLED_W + col] = g[i];
        oled_cmd(SH1106_SET_PAGE + page);
        oled_cmd(SH1106_SET_COL_LO | (col & 0x0F));
        oled_cmd(SH1106_SET_COL_HI | ((col >> 4) & 0x0F));
        oled_data(g[i]);
    }
    cur_col += 6;
    if (cur_col + 6 > OLED_W) { cur_col = 0; cur_row = (cur_row + 1) % OLED_PAGES; }
}

void oled_puts(const char *s)
{
    while (*s) oled_putc(*s++);
}

void oled_draw_hbar(uint8_t col, uint8_t row, uint8_t width, uint8_t fill_pct)
{
    uint8_t fill = (uint8_t)((uint16_t)width * fill_pct / 100);
    for (uint8_t i=0;i<width;i++){
        uint8_t v = (i < fill) ? 0xFF : 0x00;
        vbuf[row*OLED_W + col + i] = v;
        oled_cmd(SH1106_SET_PAGE + row);
        oled_cmd(SH1106_SET_COL_LO | ((col+i) & 0x0F));
        oled_cmd(SH1106_SET_COL_HI | (((col+i) >> 4) & 0x0F));
        oled_data(v);
    }
}

static char *itoa_simple(int val, char *buf, uint8_t width)
{
    char tmp[12]; uint8_t i=0;
    bool neg = val < 0;
    if (neg) val = -val;
    if (val == 0) tmp[i++] = '0';
    while (val) { tmp[i++] = '0' + (val % 10); val /= 10; }
    uint8_t pad = (i < width) ? (uint8_t)(width - i) : 0;
    uint8_t k=0;
    if (neg) buf[k++] = '-';
    while (pad--) buf[k++] = ' ';
    while (i) buf[k++] = tmp[--i];
    buf[k] = 0;
    return buf;
}

void oled_status_channel(uint8_t ch, int8_t rssi)
{
    char b[16];
    oled_set_cursor(0, 0);
    oled_puts("CH:");
    itoa_simple(ch, b, 2);
    oled_puts(b);
    oled_puts(" RSSI:");
    itoa_simple(rssi, b, 2);
    oled_puts(b);
    oled_puts("dBm");
}

static const char *mode_names[] = {"SNIFF", "KEYHUNT", "ROGUE", "JAM", "RELAY"};

void oled_status_mode(uint8_t mode)
{
    oled_set_cursor(0, 1);
    oled_puts("MODE: ");
    oled_puts(mode_names[mode % MODE_NUM]);
}

void oled_status_counters(uint32_t frames, uint32_t keys, uint32_t injected)
{
    char b[16];
    oled_set_cursor(0, 2);
    oled_puts("FRM:");
    itoa_simple((int)frames, b, 6);
    oled_puts(b);
    oled_set_cursor(0, 3);
    oled_puts("KEY:");
    itoa_simple((int)keys, b, 4);
    oled_puts(b);
    oled_puts(" TX:");
    itoa_simple((int)injected, b, 4);
    oled_puts(b);
}