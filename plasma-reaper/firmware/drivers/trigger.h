/*
 * trigger.h — Trigger subsystem driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TRIGGER_H
#define TRIGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

void trigger_init(void);
void trigger_configure(const trigger_config_t *config);
void trigger_arm(void);
void trigger_disarm(void);
bool trigger_fired(void);

/* UART trigger word matching */
void trigger_set_uart_word(const uint8_t *word, uint8_t len);
bool trigger_check_uart_word(uint8_t byte);

/* Power envelope comparator threshold */
void trigger_set_power_threshold(uint16_t threshold_mv);

#endif /* TRIGGER_H */