/*
 * flash.h — W25Q128 external flash interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int    flash_init(void);
int    flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
int    flash_write(uint32_t addr, const uint8_t *buf, uint32_t len);
int    flash_erase_sector(uint32_t addr);
int    flash_read_pib(uint8_t *buf, uint32_t maxlen);
void   flash_wipe_keys(void);
int16_t flash_read_temp(void);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_H */