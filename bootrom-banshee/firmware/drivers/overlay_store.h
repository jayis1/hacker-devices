/*
 * BootROM Banshee overlay store
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef BOOTROM_BANSHEE_OVERLAY_STORE_H
#define BOOTROM_BANSHEE_OVERLAY_STORE_H

#include "../board.h"

void overlay_store_init(void);
const bb_overlay_t *overlay_store_get_by_name(const char *name);
const bb_overlay_t *overlay_store_get_by_index(uint32_t index);
uint32_t overlay_store_count(void);
bool overlay_store_apply(const char *name, bb_spi_frame_t *frame);

#endif
