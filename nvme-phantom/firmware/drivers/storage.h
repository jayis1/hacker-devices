/*
 * drivers/storage.h — W25Q128 SPI NOR payload store header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_STORAGE_H
#define NVME_PHANTOM_STORAGE_H

#include <stdint.h>

int storage_init(void);
int storage_read(uint32_t addr, uint8_t *buf, uint32_t len);
int storage_erase_sector(uint32_t addr);
int storage_write_page(uint32_t addr, const uint8_t *data, uint16_t len);
int storage_write(uint32_t addr, const uint8_t *data, uint32_t len);

int storage_load_bitstream_offset(uint32_t *out_offset, uint32_t *out_len);
int storage_save_spoof_preset(uint8_t index, const uint8_t ident[4096]);
int storage_load_spoof_preset(uint8_t index, uint8_t ident[4096]);
int storage_save_opal_capture(uint32_t offset, const uint8_t *data, uint32_t len);
int storage_save_rule(uint8_t slot, const uint8_t rule[32]);

#endif /* NVME_PHANTOM_STORAGE_H */