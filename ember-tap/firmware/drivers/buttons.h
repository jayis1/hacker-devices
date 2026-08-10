/*
 * buttons.h — debounced tactile button handler
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EMBER_BUTTONS_H
#define EMBER_BUTTONS_H

#include <stdint.h>

void buttons_init(void);

/* Bitmask of pressed buttons (1=UP, 2=DN, 4=SEL). Returns edges only. */
uint8_t buttons_read(void);

#endif /* EMBER_BUTTONS_H */
/* end of file — author: jayis1 */