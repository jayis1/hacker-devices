/*
 * display.h — SSD1306 OLED driver + UI renderer
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EMBER_DISPLAY_H
#define EMBER_DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_clear(void);
void display_flush(void);

/* Text drawing (6×8 font, col/row based). */
void display_text(int col, int row, const char *s);
void display_text_inv(int col, int row, const char *s);  /* inverted bg */

/* Draw a horizontal progress bar (0..255). */
void display_bar(int x, int y, int w, int h, uint8_t pct);

/* High-level status screen — called from main scheduler. */
void display_render_status(uint8_t mode, uint32_t vbus_mv, uint32_t vbus_ma,
                           int8_t temp_c, uint32_t fuzz_sent,
                           uint32_t fuzz_crash);

#endif /* EMBER_DISPLAY_H */
/* end of file — author: jayis1 */