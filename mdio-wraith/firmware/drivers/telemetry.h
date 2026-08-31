/*
 * drivers/telemetry.h - MDIO Wraith telemetry and reporting
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MDIO_WRAITH_TELEMETRY_H
#define MDIO_WRAITH_TELEMETRY_H

#include "../board.h"

void mw_telemetry_seed(mw_system_t *sys);
void mw_telemetry_update(mw_system_t *sys, uint32_t step);
void mw_telemetry_report(const mw_system_t *sys, char *buffer, size_t length);
void mw_telemetry_render_json(const mw_system_t *sys, char *buffer, size_t length);

#endif
