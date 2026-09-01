/*
 * BootROM Banshee radio/control abstraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef BOOTROM_BANSHEE_RADIO_H
#define BOOTROM_BANSHEE_RADIO_H

#include "../board.h"

void radio_init(void);
void radio_queue_demo_command(bb_radio_command_kind_t kind, uint32_t arg0, uint32_t arg1, const char *text);
bool radio_poll_command(bb_radio_command_t *cmd);
void radio_publish_status(const bb_runtime_t *runtime);

#endif
