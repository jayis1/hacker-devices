/*
 * oled.h — SSD1306 128x64 OLED driver + UI primitives
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_OLED_H
#define PULSEREAPER_OLED_H

#include <stdint.h>

void oled_init(void);

/* Show a two-line status (line1, line2). Either may be NULL. */
void oled_show_status(const char *line1, const char *line2);

/* Show capture statistics (frames, bytes). */
void oled_show_capture_stats(uint32_t frames, uint32_t bytes);

/* Clear the display. */
void oled_clear(void);

#endif