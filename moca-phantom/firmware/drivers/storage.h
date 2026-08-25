/*
 * MoCA Phantom Storage Interface
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_STORAGE_H
#define MPH_STORAGE_H

#include "../board.h"

void mph_storage_init(void);
void mph_storage_store_config(const mph_runtime_config_t *config);
void mph_storage_append_captures(const mph_frame_t *frames, size_t count, mph_metrics_t *metrics);
size_t mph_storage_log_count(void);
void mph_storage_dump_recent(size_t count);

#endif
