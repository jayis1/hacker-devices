/**
 * drivers/laser.h — 1064 nm laser driver control
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_LASER_H
#define PHOTONSTRIKE_LASER_H

#include <stdint.h>
#include <stdbool.h>

void    laser_init(void);
void    laser_enable(void);
void    laser_disable(void);
void    laser_set_current(uint16_t dac_counts);  /* 0..4095 */
uint16_t laser_read_temp(void);                  /* degrees C */
bool    laser_driver_fault(void);

#endif