/*
 * ddc.h - EDID Phantom DDC/EDID subsystem
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_DDC_H
#define EDID_PHANTOM_DDC_H

#include "../board.h"

void ddc_init(void);
void ddc_set_profile(const ep_profile_t *profile);
void ddc_capture_cycle(uint32_t now_ms, ep_status_t *status);
void ddc_build_report(char *buffer, size_t length);
const ep_edid_image_t *ddc_active_edid(void);
const ep_ddc_txn_t *ddc_transaction(size_t index);
size_t ddc_transaction_count(void);
void ddc_force_mutation(uint16_t seed, ep_status_t *status);

#endif
