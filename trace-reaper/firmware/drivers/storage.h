/*
 * TRACE-REAPER — encrypted session storage driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include "../board.h"

void storage_init(const uint8_t passkey[16]);
uint8_t storage_count(void);
int storage_save(const session_result_t *res, const session_cfg_t *cfg,
                 uint32_t ts_now);
int storage_load(uint32_t idx, session_result_t *res, session_cfg_t *cfg,
                 char *label_out);
void storage_wipe_all(void);

#endif /* STORAGE_H */