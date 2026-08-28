/*
 * interposer.h - safety and manipulation engine for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ESPI_REVENANT_INTERPOSER_H
#define ESPI_REVENANT_INTERPOSER_H

#include "../board.h"
#include "espi_bus.h"

typedef struct {
    uint8_t armed;
    uint8_t bypass_enabled;
    uint8_t watchdog_ok;
    uint8_t vwire_pulses_used;
    uint8_t periph_ops_used;
    uint32_t watchdog_ms;
    uint32_t last_action_ms;
} er_interposer_t;

void er_interposer_init(er_interposer_t *ctx);
void er_interposer_arm(er_interposer_t *ctx, uint8_t armed);
void er_interposer_tick(er_interposer_t *ctx, const er_bus_t *bus, er_status_t *status);
uint8_t er_interposer_can_inject_vwire(const er_interposer_t *ctx, const er_profile_t *profile);
uint8_t er_interposer_can_delay_flash(const er_interposer_t *ctx, const er_profile_t *profile);
uint8_t er_interposer_can_replay_peripheral(const er_interposer_t *ctx, const er_profile_t *profile);
void er_interposer_note_action(er_interposer_t *ctx, uint32_t now_ms);
void er_interposer_force_bypass(er_interposer_t *ctx, er_status_t *status, const char *reason, er_event_t *event_out);

#endif
