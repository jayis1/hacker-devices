/*
 * drivers/joystick.h — 5-way analog joystick driver
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef ZIGBEE_PHANTOM_JOYSTICK_H
#define ZIGBEE_PHANTOM_JOYSTICK_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    JOY_NONE = 0, JOY_UP, JOY_DOWN, JOY_LEFT, JOY_RIGHT, JOY_CENTER
} joy_dir_t;

void joystick_init(void);
joy_dir_t joystick_read(void);          /* non-blocking; returns last press */
joy_dir_t joystick_wait(uint32_t timeout_ms);  /* blocking with timeout */
void joystick_clear(void);

#endif