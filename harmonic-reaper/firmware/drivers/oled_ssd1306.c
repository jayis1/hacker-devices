/*
 * oled_ssd1306.c — OLED UI driver (SSD1306, 128×64, SPI)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * We use a simple framebuffer (1024 bytes = 128×64/8) and draw text via
 * a 5×7 font. The display is on SPIM2 (shared with SD card — we only
 * access one at a time, guarded by the scheduler's serialization).
 */
#include "../registers.h"
#include "../board.h"
#include "oled_ssd1306.h"
#include <string.h>

/* 128×64 / 8 = 1024 bytes */
#define OLED_WIDTH   128
#define OLED_HEIGHT   64
#define OLED_BUF_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)

static uint8_t fb[OLED_BUF_SIZE];

/* SSD1306 command/control bytes */
#define OLED_CMD    0
#define OLED_DATA   1

static void oled_cs_low(void)  { GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << OLED_CS_PIN); }
static void oled_cs_high(void) { GPIO_OUTSET(NRF_GPIO_BASE) = (1u << OLED_CS_PIN); }
static void oled_dc_cmd(void)  { GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << OLED_DC_PIN); }
static void oled_dc_data(void) { GPIO_OUTSET(NRF_GPIO_BASE) = (1u << OLED_DC_PIN); }

static void oled_spi_write(const uint8_t *data, uint16_t len)
{
    SPIM_TXD_PTR(OLED_SPI_BASE)   = (uint32_t)data;
    SPIM_TXD_MAXCNT(OLED_SPI_BASE) = len;
    SPIM_RXD_MAXCNT(OLED_SPI_BASE) = 0;
    SPIM_START_TX(OLED_SPI_BASE);
    while (!SPIM_END_EVENT(OLED_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(OLED_SPI_BASE);
}

static void oled_cmd(uint8_t c)
{
    oled_dc_cmd();
    oled_cs_low();
    oled_spi_write(&c, 1);
    oled_cs_high();
}

static void oled_flush(void)
{
    /* Set column & page address window, then dump framebuffer */
    oled_cmd(0x21); oled_cmd(0);   oled_cmd(127);  /* col 0..127 */
    oled_cmd(0x22); oled_cmd(0);   oled_cmd(7);    /* page 0..7  */

    oled_dc_data();
    oled_cs_low();
    oled_spi_write(fb, OLED_BUF_SIZE);
    oled_cs_high();
}

/* ---- minimal 5×7 font (printable ASCII 0x20..0x7E) ---- */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */
    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* * */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41}, /* < */
    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x41,0x22,0x14,0x08,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x01,0x01}, /* F */
    {0x3E,0x41,0x41,0x51,0x32}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x03,0x04,0x78,0x04,0x03}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x00,0x00,0x00,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */
    {0x00,0x00,0x00,0x00,0x00}, /* ] */
    {0x00,0x02,0x01,0x02,0x00}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
};

static void oled_draw_char(uint8_t x, uint8_t y, char c)
{
    if (c < 0x20 || c > 0x5F) c = 0x20;
    uint8_t idx = (uint8_t)(c - 0x20);
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = font5x7[idx][col];
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1u << row)) {
                uint16_t px = x + col;
                uint16_t py = y + row;
                if (px < OLED_WIDTH && py < OLED_HEIGHT) {
                    fb[py / 8 * OLED_WIDTH + px] |= (1u << (py & 7));
                }
            }
        }
    }
}

static void oled_draw_str(uint8_t x, uint8_t y, const char *s)
{
    while (*s) {
        oled_draw_char(x, y, *s);
        x += 6;
        s++;
    }
}

static void oled_clear_fb(void)
{
    memset(fb, 0, OLED_BUF_SIZE);
}

/* Draw a horizontal bar — value 0..100 at (x,y), width w */
static void oled_draw_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t val)
{
    if (val > 100) val = 100;
    uint8_t fill = (uint8_t)((val * w) / 100);
    for (uint8_t i = 0; i < w; i++) {
        uint8_t col = (i < fill) ? 0x7F : 0x41;
        for (uint8_t r = 0; r < 7; r++) {
            if (col & (1u << r)) {
                fb[(y + r) / 8 * OLED_WIDTH + (x + i)] |= (1u << ((y + r) & 7));
            }
        }
    }
}

void oled_init(void)
{
    /* Configure SPIM2 for OLED */
    SPIM_PSEL_SCK(OLED_SPI_BASE)  = OLED_SCK_PIN;
    SPIM_PSEL_MOSI(OLED_SPI_BASE) = OLED_MOSI_PIN;
    SPIM_FREQUENCY(OLED_SPI_BASE) = SPIM_FREQ_8M;
    SPIM_CONFIG(OLED_SPI_BASE)   = SPIM_MODE3;
    SPIM_ENABLE(OLED_SPI_BASE)   = 1u;

    /* GPIO for DC, CS, RST */
    GPIO_PIN_CNF(NRF_GPIO_BASE, OLED_DC_PIN)  =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_PIN_CNF(NRF_GPIO_BASE, OLED_CS_PIN)  =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_PIN_CNF(NRF_GPIO_BASE, OLED_RST_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);

    /* Reset */
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << OLED_RST_PIN);
    nrf_delay_ms(10);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << OLED_RST_PIN);
    nrf_delay_ms(50);

    /* Init sequence (SSD1306 datasheet §9.4) */
    oled_cmd(0xAE);              /* display off */
    oled_cmd(0xD5); oled_cmd(0x80);  /* clock divide */
    oled_cmd(0xA8); oled_cmd(0x3F);  /* multiplex = 63 */
    oled_cmd(0xD3); oled_cmd(0x00);  /* display offset = 0 */
    oled_cmd(0x40);              /* start line = 0 */
    oled_cmd(0x8D); oled_cmd(0x14);  /* charge pump on */
    oled_cmd(0x20); oled_cmd(0x00);  /* horizontal addressing */
    oled_cmd(0xA1);              /* segment remap */
    oled_cmd(0xC8);              /* COM scan direction */
    oled_cmd(0xDA); oled_cmd(0x12);  /* COM pins */
    oled_cmd(0x81); oled_cmd(0xCF);  /* contrast */
    oled_cmd(0xD9); oled_cmd(0xF1);  /* pre-charge period */
    oled_cmd(0xDB); oled_cmd(0x40);  /* VCOMH deselect */
    oled_cmd(0xA4);              /* display = RAM content */
    oled_cmd(0xA6);              /* normal (not inverted) */
    oled_cmd(0xAF);              /* display ON */

    oled_clear_fb();
    oled_flush();
}

void oled_show_boot(void)
{
    oled_clear_fb();
    oled_draw_str(8, 0, "HARMONIC REAPER");
    oled_draw_str(16, 16, "NLJD TSCM");
    oled_draw_str(20, 32, "by jayis1");
    oled_draw_str(4, 52, "Booting...");
    oled_flush();
}

void oled_set_mode(sweep_mode_t m)
{
    oled_clear_fb();
    const char *label = "IDLE";
    switch (m) {
        case MODE_IDLE:        label = "IDLE";      break;
        case MODE_QUIET_RX:    label = "QUIET RX";  break;
        case MODE_SWEEP_CW:    label = "CW SWEEP";  break;
        case MODE_SWEEP_PULSED:label = "PULSED";    break;
        case MODE_CALIBRATE:   label = "CALIBRATE"; break;
        case MODE_FAULT:       label = "FAULT";     break;
    }
    oled_draw_str(0, 0, label);
    oled_flush();
}

void oled_update_live(int16_t p2, int16_t p3, int8_t ratio,
                      uint8_t classify, uint8_t batt, uint8_t charging,
                      sweep_mode_t mode, uint32_t hits,
                      int16_t pitch, int16_t yaw)
{
    oled_clear_fb();

    /* Row 0: mode + battery */
    const char *ml = (mode == MODE_SWEEP_PULSED) ? "PULSED" :
                     (mode == MODE_SWEEP_CW)    ? "CW"    :
                     (mode == MODE_QUIET_RX)    ? "QRX"   :
                     (mode == MODE_CALIBRATE)   ? "CAL"   : "IDLE";
    oled_draw_str(0, 0, ml);
    char bbuf[8];
    /* simple int-to-string for battery % */
    int bp = batt;
    bbuf[0] = '0' + (bp / 100) % 10;
    bbuf[1] = '0' + (bp / 10) % 10;
    bbuf[2] = '0' + bp % 10;
    bbuf[3] = '%';
    bbuf[4] = charging ? '+' : ' ';
    bbuf[5] = 0;
    oled_draw_str(100, 0, bbuf);

    /* Row 1: 2f0 bar graph */
    oled_draw_str(0, 10, "2f");
    uint8_t p2_pct = (uint8_t)((p2 + 120) * 100 / 120);  /* map −120..0 dBFS */
    if (p2_pct > 100) p2_pct = 100;
    oled_draw_bar(18, 10, 100, p2_pct);

    /* Row 2: 3f0 bar graph */
    oled_draw_str(0, 22, "3f");
    uint8_t p3_pct = (uint8_t)((p3 + 120) * 100 / 120);
    if (p3_pct > 100) p3_pct = 100;
    oled_draw_bar(18, 22, 100, p3_pct);

    /* Row 3: ratio + classification verdict */
    oled_draw_str(0, 36, "R:");
    char rbuf[8];
    /* signed int to string */
    int r = ratio;
    int ri = 0;
    if (r < 0) { rbuf[ri++] = '-'; r = -r; }
    rbuf[ri++] = '0' + (r / 10) % 10;
    rbuf[ri++] = '0' + r % 10;
    rbuf[ri] = 0;
    oled_draw_str(18, 36, rbuf);

    const char *verdict = "----";
    switch (classify) {
        case 1: verdict = "SEMI";   break;  /* semiconductor */
        case 2: verdict = "METAL";  break;  /* dissimilar metal */
        case 3: verdict = "????";   break;  /* ambiguous */
    }
    oled_draw_str(60, 36, verdict);

    /* Row 4: hit count */
    char hbuf[12];
    hbuf[0] = 'H'; hbuf[1] = ':';
    uint32_t h = hits;
    int hi = 2;
    if (h == 0) { hbuf[hi++] = '0'; }
    else {
        char tmp[12]; int ti = 0;
        while (h) { tmp[ti++] = '0' + (h % 10); h /= 10; }
        while (ti > 0) hbuf[hi++] = tmp[--ti];
    }
    hbuf[hi] = 0;
    oled_draw_str(0, 48, hbuf);

    /* Row 5: wand heading */
    oled_draw_str(64, 48, "HDG");
    (void)pitch; (void)yaw;

    oled_flush();
}

void oled_flash_hit(int16_t p2, int8_t ratio, uint32_t hit_num)
{
    /* Quick splash screen on a confirmed hit — stays for ~2 refreshes */
    oled_clear_fb();
    oled_draw_str(0, 0, "** HIT **");
    oled_draw_str(0, 16, "2f dBFS:");
    char buf[8];
    int p = p2; int pi = 0;
    if (p < 0) { buf[pi++] = '-'; p = -p; }
    buf[pi++] = '0' + (p / 10) % 10;
    buf[pi++] = '0' + p % 10;
    buf[pi] = 0;
    oled_draw_str(60, 16, buf);

    oled_draw_str(0, 32, "Ratio:");
    int r = ratio; int ri2 = 0;
    char rbuf2[8];
    if (r < 0) { rbuf2[ri2++] = '-'; r = -r; }
    rbuf2[ri2++] = '0' + (r / 10) % 10;
    rbuf2[ri2++] = '0' + r % 10;
    rbuf2[ri2] = 0;
    oled_draw_str(60, 32, rbuf2);
    oled_draw_str(90, 32, "SEMI");
    oled_flush();
}

void oled_flash_low_battery(uint8_t pct)
{
    oled_clear_fb();
    oled_draw_str(0, 0, "LOW BATTERY");
    oled_draw_str(0, 16, "RECHARGE");
    char buf[8];
    buf[0] = '0' + (pct / 10) % 10;
    buf[1] = '0' + pct % 10;
    buf[2] = '%';
    buf[3] = 0;
    oled_draw_str(0, 32, buf);
    oled_flush();
}

/* EOF — oled_ssd1306.c — jayis1 */