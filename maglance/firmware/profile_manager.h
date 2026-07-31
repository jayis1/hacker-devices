/*
 * profile_manager.h — EEPROM profile storage for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PROFILE_MANAGER_H
#define PROFILE_MANAGER_H

#include "board.h"
#include <stdint.h>

/* Initialize EEPROM driver and verify connectivity */
int profile_manager_init(void);

/* Save a profile to the specified slot (0-15) */
int profile_manager_save(uint8_t slot, const profile_t *profile);

/* Load a profile from the specified slot */
int profile_manager_load(uint8_t slot, profile_t *profile);

/* Check if a slot is occupied (has valid data) */
int profile_manager_is_occupied(uint8_t slot);

/* Erase a profile slot */
int profile_manager_erase(uint8_t slot);

/* Erase all profile slots */
int profile_manager_erase_all(void);

/* Calculate simple XOR checksum for profile data */
uint8_t profile_manager_checksum(const profile_t *profile);

/* Validate a profile's checksum */
int profile_manager_validate(const profile_t *profile);

/* List all occupied slots (returns count, fills slots array) */
int profile_manager_list(uint8_t *slots, uint8_t max_slots);

/* Save coil tip calibration data */
int profile_manager_save_calibration(uint8_t tip_id,
                                      const uint8_t *cal_data,
                                      uint16_t cal_len);

/* Load coil tip calibration data */
int profile_manager_load_calibration(uint8_t tip_id,
                                      uint8_t *cal_data,
                                      uint16_t max_len);

#endif /* PROFILE_MANAGER_H */