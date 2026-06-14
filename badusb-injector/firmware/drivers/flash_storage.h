/*
 * PHANTOM — SPI Flash Storage Driver
 * W25Q128JVSIQ 16MB SPI NOR Flash for Payload Storage
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 */

#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Flash header magic */
#define FLASH_HEADER_MAGIC      0x504E5400  /* "PNT\0" */
#define FLASH_HEADER_VERSION    0x00010000  /* v1.0 */

/* Flash header structure */
typedef struct {
    uint32_t magic;             /* Must be FLASH_HEADER_MAGIC */
    uint32_t version;          /* Header version */
    uint32_t profile_count;   /* Number of stored profiles */
    uint32_t total_size;      /* Total payload data size */
    uint32_t crc32;            /* Header CRC32 */
    uint32_t reserved[4];     /* Reserved for future use */
} flash_header_t;

/* Profile index entry */
typedef struct {
    uint32_t offset;          /* Start offset in flash data area */
    uint32_t size;            /* Payload size in bytes */
    uint32_t flags;           /* Profile flags */
    char name[16];            /* Null-terminated profile name */
    uint32_t crc32;           /* Payload CRC32 */
} profile_entry_t;

/* Function prototypes */
int flash_storage_init(void);
int flash_read_jedec_id(uint8_t *manufacturer, uint16_t *device_id);
int flash_read_payload(uint32_t offset, uint8_t *buf, uint32_t len);
int flash_write_payload(uint32_t offset, const uint8_t *buf, uint32_t len);
int flash_erase_sector(uint32_t sector_addr);
int flash_erase_block_64k(uint32_t block_addr);
int flash_erase_chip(void);
int flash_load_profiles(profile_entry_t *profiles, uint8_t max_count);
int flash_save_profile(uint8_t index, const profile_entry_t *entry, const uint8_t *data);
int flash_delete_profile(uint8_t index);
int flash_read_settings(device_settings_t *settings);
int flash_write_settings(const device_settings_t *settings);
int flash_read_geofence(geofence_config_t *config);
int flash_write_geofence(const geofence_config_t *config);

/* Low-level SPI flash operations */
static void flash_select(void);
static void flash_deselect(void);
static uint8_t flash_transfer(uint8_t data);
static void flash_wait_busy(void);
static void flash_write_enable(void);

#endif /* FLASH_STORAGE_H */