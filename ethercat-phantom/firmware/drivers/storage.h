/*
 * Simulated persistent storage and capture logging
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_STORAGE_H
#define ETHERCAT_PHANTOM_STORAGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

void eph_storage_init(void);
bool eph_storage_store_config(const eph_runtime_config_t *config);
bool eph_storage_load_config(eph_runtime_config_t *config);
size_t eph_storage_append_captures(const eph_capture_t *captures, size_t count);
size_t eph_storage_log_count(void);
void eph_storage_dump_recent(size_t limit);

#endif
