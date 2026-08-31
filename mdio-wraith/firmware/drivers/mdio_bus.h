/*
 * drivers/mdio_bus.h - MDIO bus model and capture helpers
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MDIO_WRAITH_MDIO_BUS_H
#define MDIO_WRAITH_MDIO_BUS_H

#include "../board.h"

void mw_mdio_init(mw_system_t *sys);
void mw_mdio_scan(mw_system_t *sys);
uint16_t mw_mdio_read(mw_system_t *sys, uint8_t phy, uint8_t devad, uint16_t reg, uint8_t clause45);
int mw_mdio_write(mw_system_t *sys, const mw_profile_t *profile, uint8_t phy, uint8_t devad, uint16_t reg, uint16_t value, uint8_t clause45);
void mw_mdio_capture_summary(const mw_system_t *sys, char *buffer, size_t length);
const mw_phy_t *mw_mdio_lookup_phy(const mw_system_t *sys, uint8_t phy);
void mw_mdio_snapshot_vendor(mw_system_t *sys, uint8_t phy);

#endif
