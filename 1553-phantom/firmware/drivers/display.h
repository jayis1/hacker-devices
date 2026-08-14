/*
 * display.h — SSD1306 OLED driver
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef DISPLAY_H
#define DISPLAY_H

void display_init(void);
void display_clear(void);
void display_text(int col, int row, const char *s);
void display_bar(int x, int y, int w, int h, int fill_pct);
void display_flush(void);

#endif /* DISPLAY_H */