/*
 * PHANTOM — SPI Flash Storage Driver Implementation
 * W25Q128JVSIQ 16MB SPI NOR Flash for Payload Storage
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 */

#include "flash_storage.h"
#include "registers.h"
#include "board.h"

/* ============================================================
 * SPI Flash Low-Level Operations
 * ============================================================ */

static void flash_select(void) {
    SIO_GPIO_OUT_CLR = (1 << PIN_FLASH_CS);  /* CS low */
    busy_wait_us(1);  /* CS setup time */
}

static void flash_deselect(void) {
    SIO_GPIO_OUT_SET = (1 << PIN_FLASH_CS);  /* CS high */
    busy_wait_us(1);  /* CS hold time */
}

static uint8_t flash_transfer(uint8_t data) {
    /* Wait until TX FIFO has space */
    while (SPI0_SSPSR & SPI_SSPSR_TNF == 0);
    SPI0_SSPDR = data;

    /* Wait until RX FIFO has data */
    while (SPI0_SSPSR & SPI_SSPSR_RNE == 0);
    return (uint8_t)SPI0_SSPDR;
}

static void flash_wait_busy(void) {
    flash_select();
    flash_transfer(FLASH_CMD_READ_STATUS1);
    while (flash_transfer(0xFF) & 0x01) {
        /* Bit 0 = busy */
    }
    flash_deselect();
}

static void flash_write_enable(void) {
    flash_select();
    flash_transfer(FLASH_CMD_WRITE_ENABLE);
    flash_deselect();
}

/* ============================================================
 * SPI Flash Initialization
 * ============================================================ */

int flash_storage_init(void) {
    /* Configure SPI0 for payload flash */
    /* SPI0 is already out of reset (gpio_init did this) */

    /* Configure SPI0: CPOL=0, CPHA=0, 8-bit, prescaler for 50MHz */
    SPI0_SSPCR0 = SPI_SSPCR0_DSS_8BIT | SPI_SSPCR0_FRF_SPI;
    SPI0_SSPCPSR = 2;  /* Clock prescale divisor */
    /* With 133MHz peripheral clock and CPSR=2, SCR=0: SCK = 133MHz / (2*(1+0)) = 66.5MHz */
    /* For W25Q128 max 133MHz read, 80MHz write — this is fine */

    /* Enable SPI0 */
    SPI0_SSPCR1 = SPI_SSPCR1_SSE | SPI_SSPCR1_MS;  /* Enable, master mode */

    /* Deassert CS */
    flash_deselect();

    /* Verify flash JEDEC ID */
    uint8_t mfr;
    uint16_t dev_id;
    if (flash_read_jedec_id(&mfr, &dev_id) != 0) {
        return -1;  /* Flash not found */
    }

    if (mfr != FLASH_JEDEC_MANUFACTURER ||
        (dev_id >> 8) != FLASH_JEDEC_DEVICE_HI ||
        (dev_id & 0xFF) != FLASH_JEDEC_DEVICE_LO) {
        return -2;  /* Wrong flash device */
    }

    return 0;
}

/* ============================================================
 * JEDEC ID Read
 * ============================================================ */

int flash_read_jedec_id(uint8_t *manufacturer, uint16_t *device_id) {
    flash_select();
    flash_transfer(FLASH_CMD_READ_JEDEC_ID);

    *manufacturer = flash_transfer(0xFF);
    *device_id = (flash_transfer(0xFF) << 8) | flash_transfer(0xFF);

    flash_deselect();
    return 0;
}

/* ============================================================
 * Flash Read
 * ============================================================ */

int flash_read_payload(uint32_t offset, uint8_t *buf, uint32_t len) {
    if (offset + len > FLASH_TOTAL_SIZE) return -1;

    uint32_t addr = FLASH_DATA_OFFSET + offset;

    flash_select();
    flash_transfer(FLASH_CMD_READ_DATA);
    flash_transfer((addr >> 16) & 0xFF);
    flash_transfer((addr >> 8) & 0xFF);
    flash_transfer(addr & 0xFF);

    for (uint32_t i = 0; i < len; i++) {
        buf[i] = flash_transfer(0xFF);
    }

    flash_deselect();
    return 0;
}

/* ============================================================
 * Flash Write (Page Program)
 * ============================================================ */

int flash_write_payload(uint32_t offset, const uint8_t *buf, uint32_t len) {
    if (offset + len > FLASH_TOTAL_SIZE) return -1;

    uint32_t addr = FLASH_DATA_OFFSET + offset;
    uint32_t written = 0;

    while (written < len) {
        /* Calculate bytes to write in this page */
        uint32_t page_offset = addr % FLASH_PAGE_SIZE;
        uint32_t chunk_len = FLASH_PAGE_SIZE - page_offset;
        if (chunk_len > (len - written)) chunk_len = len - written;

        /* Enable write */
        flash_write_enable();

        /* Page program */
        flash_select();
        flash_transfer(FLASH_CMD_PAGE_PROGRAM);
        flash_transfer((addr >> 16) & 0xFF);
        flash_transfer((addr >> 8) & 0xFF);
        flash_transfer(addr & 0xFF);

        for (uint32_t i = 0; i < chunk_len; i++) {
            flash_transfer(buf[written + i]);
        }

        flash_deselect();

        /* Wait for write completion */
        flash_wait_busy();

        addr += chunk_len;
        written += chunk_len;
    }

    return 0;
}

/* ============================================================
 * Flash Erase Operations
 * ============================================================ */

int flash_erase_sector(uint32_t sector_addr) {
    uint32_t addr = FLASH_DATA_OFFSET + sector_addr;

    flash_write_enable();

    flash_select();
    flash_transfer(FLASH_CMD_SECTOR_ERASE);
    flash_transfer((addr >> 16) & 0xFF);
    flash_transfer((addr >> 8) & 0xFF);
    flash_transfer(addr & 0xFF);
    flash_deselect();

    flash_wait_busy();
    return 0;
}

int flash_erase_block_64k(uint32_t block_addr) {
    uint32_t addr = FLASH_DATA_OFFSET + block_addr;

    flash_write_enable();

    flash_select();
    flash_transfer(FLASH_CMD_BLOCK_ERASE_64K);
    flash_transfer((addr >> 16) & 0xFF);
    flash_transfer((addr >> 8) & 0xFF);
    flash_transfer(addr & 0xFF);
    flash_deselect();

    flash_wait_busy();
    return 0;
}

int flash_erase_chip(void) {
    flash_write_enable();

    flash_select();
    flash_transfer(FLASH_CMD_CHIP_ERASE);
    flash_deselect();

    flash_wait_busy();  /* Can take up to 200 seconds */
    return 0;
}

/* ============================================================
 * Profile Management
 * ============================================================ */

int flash_load_profiles(profile_entry_t *profiles, uint8_t max_count) {
    /* Read header */
    flash_header_t header;
    flash_read_payload(FLASH_HEADER_OFFSET - FLASH_DATA_OFFSET,
                       (uint8_t *)&header, sizeof(header));

    if (header.magic != FLASH_HEADER_MAGIC) {
        /* Flash not initialized — return empty profile list */
        return 0;
    }

    if (header.version > FLASH_HEADER_VERSION) {
        /* Incompatible version */
        return -1;
    }

    /* Read profile index */
    uint8_t count = (header.profile_count < max_count) ? header.profile_count : max_count;
    uint32_t index_size = count * sizeof(profile_entry_t);

    /* Read directly from index offset in flash */
    flash_select();
    flash_transfer(FLASH_CMD_READ_DATA);
    flash_transfer((FLASH_INDEX_OFFSET >> 16) & 0xFF);
    flash_transfer((FLASH_INDEX_OFFSET >> 8) & 0xFF);
    flash_transfer(FLASH_INDEX_OFFSET & 0xFF);

    for (uint32_t i = 0; i < index_size; i++) {
        ((uint8_t *)profiles)[i] = flash_transfer(0xFF);
    }

    flash_deselect();
    return count;
}

int flash_save_profile(uint8_t index, const profile_entry_t *entry, const uint8_t *data) {
    if (index >= DUCKY_MAX_PROFILES) return -1;

    /* Calculate sector-aligned offset for data */
    /* Erase necessary sectors and write data */
    uint32_t data_offset = entry->offset;
    uint32_t sectors_needed = (entry->size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;

    /* Erase sectors */
    for (uint32_t i = 0; i < sectors_needed; i++) {
        flash_erase_sector(data_offset + i * FLASH_SECTOR_SIZE);
    }

    /* Write payload data */
    flash_write_payload(data_offset, data, entry->size);

    /* Update profile index entry */
    uint32_t index_offset = FLASH_INDEX_OFFSET + index * sizeof(profile_entry_t);

    /* Erase index sector */
    flash_erase_sector(FLASH_INDEX_OFFSET);

    /* Write index entry */
    flash_write_payload(index_offset, (const uint8_t *)entry, sizeof(profile_entry_t));

    return 0;
}

int flash_delete_profile(uint8_t index) {
    if (index >= DUCKY_MAX_PROFILES) return -1;

    /* Create empty entry */
    profile_entry_t empty = {0};

    /* Erase and rewrite index entry */
    uint32_t index_offset = FLASH_INDEX_OFFSET + index * sizeof(profile_entry_t);
    flash_erase_sector(FLASH_INDEX_OFFSET);
    flash_write_payload(index_offset, (const uint8_t *)&empty, sizeof(profile_entry_t));

    return 0;
}

/* ============================================================
 * Device Settings
 * ============================================================ */

int flash_read_settings(device_settings_t *settings) {
    flash_select();
    flash_transfer(FLASH_CMD_READ_DATA);
    flash_transfer((FLASH_SETTINGS_OFFSET >> 16) & 0xFF);
    flash_transfer((FLASH_SETTINGS_OFFSET >> 8) & 0xFF);
    flash_transfer(FLASH_SETTINGS_OFFSET & 0xFF);

    for (uint32_t i = 0; i < sizeof(device_settings_t); i++) {
        ((uint8_t *)settings)[i] = flash_transfer(0xFF);
    }

    flash_deselect();

    if (settings->magic != SETTINGS_MAGIC) {
        /* Return defaults */
        memset(settings, 0, sizeof(device_settings_t));
        settings->magic = SETTINGS_MAGIC;
        settings->version = SETTINGS_VERSION;
        settings->default_profile = 0;
        settings->usb_mode = 0;  /* Start in stealth */
        settings->default_delay = DUCKY_DEFAULT_DELAY;
        settings->oled_brightness = 255;
        settings->led_brightness = 255;
        return 1;  /* Default settings */
    }

    return 0;
}

int flash_write_settings(const device_settings_t *settings) {
    /* Erase settings sector */
    flash_erase_sector(FLASH_SETTINGS_OFFSET);

    /* Write settings */
    flash_write_payload(FLASH_SETTINGS_OFFSET,
                        (const uint8_t *)settings,
                        sizeof(device_settings_t));

    return 0;
}

/* ============================================================
 * Geofence Configuration
 * ============================================================ */

int flash_read_geofence(geofence_config_t *config) {
    flash_select();
    flash_transfer(FLASH_CMD_READ_DATA);
    flash_transfer((FLASH_GEOFENCE_OFFSET >> 16) & 0xFF);
    flash_transfer((FLASH_GEOFENCE_OFFSET >> 8) & 0xFF);
    flash_transfer(FLASH_GEOFENCE_OFFSET & 0xFF);

    for (uint32_t i = 0; i < sizeof(geofence_config_t); i++) {
        ((uint8_t *)config)[i] = flash_transfer(0xFF);
    }

    flash_deselect();
    return 0;
}

int flash_write_geofence(const geofence_config_t *config) {
    /* Erase geofence sector */
    flash_erase_sector(FLASH_GEOFENCE_OFFSET);

    /* Write geofence config */
    flash_write_payload(FLASH_GEOFENCE_OFFSET,
                        (const uint8_t *)config,
                        sizeof(geofence_config_t));

    return 0;
}