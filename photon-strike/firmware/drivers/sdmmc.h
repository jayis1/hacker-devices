/**
 * drivers/sdmmc.h — MicroSD logging on SDMMC1
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_SDMMC_H
#define PHOTONSTRIKE_SDMMC_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

bool sd_init(void);
bool sd_open_log(const char *name);
bool sd_log_shot(const ps_shot_t *shot);
bool sd_close_log(void);

#endif