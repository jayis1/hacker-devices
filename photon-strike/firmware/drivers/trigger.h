/**
 * drivers/trigger.h — trigger arbiter interface
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_TRIGGER_H
#define PHOTONSTRIKE_TRIGGER_H

#include <stdint.h>
#include "board.h"

void trigger_init(void);
void trigger_configure(const ps_scan_desc_t *desc);
void trigger_arm(void);
void trigger_disarm(void);
bool trigger_fired(uint32_t timeout_us);
void trigger_set_source(uint8_t src);

#endif