/*
 * profile.h - EDID Phantom profile interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_PROFILE_H
#define EDID_PHANTOM_PROFILE_H

#include "../board.h"

void profile_init(void);
const ep_profile_t *profile_default(void);
const ep_profile_t *profile_find(const char *name);
size_t profile_count(void);
const ep_profile_t *profile_at(size_t index);

#endif
