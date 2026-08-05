/*
 * sdcard.h — SD card logging driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef SDCARD_H
#define SDCARD_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

void sdcard_init(void);
void sdcard_log_shot(const glitch_params_t *params, const glitch_result_t *result);
void sdcard_log_sweep_start(const sweep_config_t *config);
void sdcard_log_sweep_end(const sweep_status_t *status);
uint32_t sdcard_get_log_count(void);
bool sdcard_read_log_entry(uint32_t index, char *buf, uint32_t buf_len);

#endif /* SDCARD_H */