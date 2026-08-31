/*
 * drivers/profile.h - MDIO Wraith scenario profiles
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MDIO_WRAITH_PROFILE_H
#define MDIO_WRAITH_PROFILE_H

#include "../board.h"

size_t mw_profile_count(void);
const mw_profile_t *mw_profile_get(size_t index);
const mw_profile_t *mw_profile_find(const char *name);
void mw_profile_describe(const mw_profile_t *profile, char *buffer, size_t length);

#endif
