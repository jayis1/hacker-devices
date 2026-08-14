/*
 * buttons.h — button + panic API
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

void buttons_init(void);
void buttons_tick(uint32_t now_ms);
void buttons_on_press(uint32_t now_ms);

/* Implemented by main.c */
void buttons_long_press(void);

#endif /* BUTTONS_H */