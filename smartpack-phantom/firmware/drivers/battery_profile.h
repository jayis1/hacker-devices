/*
 * SmartPack Phantom battery profile interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef SMARTPACK_BATTERY_PROFILE_H
#define SMARTPACK_BATTERY_PROFILE_H

#include <stddef.h>
#include <stdint.h>
#include "../board.h"

void battery_profile_init(sp_battery_profile_t *profile, sp_profile_kind_t kind);
const char *battery_profile_name(sp_profile_kind_t kind);
uint8_t battery_profile_relative_soc(const sp_battery_profile_t *profile);
uint16_t battery_profile_runtime_to_empty(const sp_battery_profile_t *profile);
uint16_t battery_profile_average_time_to_empty(const sp_battery_profile_t *profile);
uint16_t battery_profile_manufacturer_date(const sp_battery_profile_t *profile);
uint16_t battery_profile_command_word(const sp_battery_profile_t *profile, uint8_t command);
size_t battery_profile_command_block(const sp_battery_profile_t *profile, uint8_t command, uint8_t *out, size_t max_len);
void battery_profile_apply_status(sp_battery_profile_t *profile, uint16_t status_word);
void battery_profile_set_soc_percent(sp_battery_profile_t *profile, uint8_t soc_percent);
void battery_profile_set_temperature(sp_battery_profile_t *profile, int16_t temp_c);
void battery_profile_set_charge_limit(sp_battery_profile_t *profile, uint16_t ma);
void battery_profile_apply_named_overlay(sp_battery_profile_t *profile, const char *name);

#endif
