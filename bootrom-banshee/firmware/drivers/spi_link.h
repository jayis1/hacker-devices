/*
 * BootROM Banshee SPI link model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef BOOTROM_BANSHEE_SPI_LINK_H
#define BOOTROM_BANSHEE_SPI_LINK_H

#include "../board.h"

void spi_link_init(void);
void spi_link_set_mutation_enabled(bool enabled);
void spi_link_set_bus_mode(bb_bus_mode_t mode);
void spi_link_force_jedec_id(uint32_t value);
void spi_link_set_truncation(uint32_t len);
void spi_link_set_overlay_name(const char *name);
void spi_link_set_delay_us(uint32_t delay_us);
bool spi_link_poll_frame(uint32_t now_ms, bb_spi_frame_t *frame);
const char *spi_link_bus_mode_name(bb_bus_mode_t mode);

#endif
