/**
 * drivers/gpio.h — GPIO abstraction for PhotonStrike
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_GPIO_H
#define PHOTONSTRIKE_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

void gpio_init_all(void);
void gpio_config_pin(volatile uint32_t *port, uint8_t pin,
                     uint8_t mode, uint8_t pupd, uint8_t speed);
void gpio_set_pin(volatile uint32_t *port, uint8_t pin);
void gpio_clr_pin(volatile uint32_t *port, uint8_t pin);
bool gpio_read_pin(volatile uint32_t *port, uint8_t pin);
void gpio_af_pin(volatile uint32_t *port, uint8_t pin, uint8_t af);

#endif