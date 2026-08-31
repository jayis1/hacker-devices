/*
 * drivers/safety.h - MDIO Wraith safety controls
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MDIO_WRAITH_SAFETY_H
#define MDIO_WRAITH_SAFETY_H

#include "../board.h"

void mw_safety_init(mw_system_t *sys, const mw_profile_t *profile);
int mw_safety_can_arm(const mw_system_t *sys, const mw_profile_t *profile, char *reason, size_t length);
int mw_safety_can_write(mw_system_t *sys, const mw_profile_t *profile, uint16_t reg, uint16_t value, char *reason, size_t length);
void mw_safety_tick(mw_system_t *sys);
void mw_safety_force_bypass(mw_system_t *sys, const char *reason);

#endif
