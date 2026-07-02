/*
 * optics_driver.h — LC bandpass + lens/fiber input select
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_OPTICS_DRIVER_H
#define AURORA_OPTICS_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

int  optics_init(void);
void optics_set_wavelength(uint16_t nm);   /* 450..950 */
uint16_t optics_get_wavelength(void);
void optics_select_fiber_input(bool on);   /* true = 5mm fiber pigtail */

#endif