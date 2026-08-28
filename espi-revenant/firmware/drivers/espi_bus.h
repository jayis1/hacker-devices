/*
 * espi_bus.h - channel model for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ESPI_REVENANT_ESPI_BUS_H
#define ESPI_REVENANT_ESPI_BUS_H

#include "../board.h"

typedef struct {
    uint32_t tick_ms;
    uint8_t vwire_state;
    uint32_t flash_delay_ns;
    uint8_t flash_window_open;
    uint8_t target_awake;
    uint8_t maintenance_hint;
    uint8_t profile_step;
    er_profile_t profile;
} er_bus_t;

void er_bus_init(er_bus_t *bus);
void er_bus_load_profile(er_bus_t *bus, const er_profile_t *profile);
void er_bus_step(er_bus_t *bus, er_trace_frame_t *frame_out);
void er_bus_force_resume_window(er_bus_t *bus);
void er_bus_apply_vwire_pulse(er_bus_t *bus, uint8_t mask);
void er_bus_set_flash_delay(er_bus_t *bus, uint32_t delay_ns);
const char *er_bus_channel_name(er_channel_t channel);
void er_bus_describe_frame(const er_trace_frame_t *frame, char *buffer, size_t buffer_len);

#endif
