/*
 * BootROM Banshee glitch engine
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef BOOTROM_BANSHEE_GLITCH_ENGINE_H
#define BOOTROM_BANSHEE_GLITCH_ENGINE_H

#include "../board.h"

void glitch_engine_init(void);
void glitch_engine_set_delay_us(uint32_t delay_us);
void glitch_engine_set_truncation(uint32_t len);
void glitch_engine_set_strap_mask(uint32_t mask);
uint32_t glitch_engine_delay_us(void);
uint32_t glitch_engine_truncation(void);
uint32_t glitch_engine_strap_mask(void);
void glitch_engine_process(bb_runtime_t *runtime, bb_spi_frame_t *frame, uint32_t now_ms, char *status_text, uint32_t status_len);
const char *glitch_engine_strap_summary(void);

#endif
