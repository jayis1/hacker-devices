/**
 * drivers/mems.h — 2-axis MEMS mirror + piezo Z control
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_MEMS_H
#define PHOTONSTRIKE_MEMS_H

#include <stdint.h>

void mems_init(void);
void mems_goto(uint16_t x_um, uint16_t y_um);
void mems_set_focus(uint16_t z_nm);
void mems_home(void);

#endif