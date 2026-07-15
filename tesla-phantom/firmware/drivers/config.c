/*
 * tesla-phantom — drivers/config.c
 * Persistent configuration storage in QSPI flash.
 * Saves/loads device configuration (EMFI params, SCA params, trigger,
 * BLE PSK) with CRC32 validation.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* Configuration is stored in QSPI flash at a fixed offset.
 * We use a simple two-slot scheme (A/B) with CRC32 for wear-leveling
 * and atomic updates. The slot with the highest version number and
 * valid CRC is the active one. */

#define CONFIG_FLASH_OFFSET_A   0x00100000UL  /* 1 MB offset (slot A) */
#define CONFIG_FLASH_OFFSET_B   0x00110000UL  /* 1 MB + 64 KB (slot B) */
#define CONFIG_SECTOR_SIZE      0x00010000UL  /* 64 KB sector */

/* QSPI flash commands (W25Q128) */
#define QSPI_CMD_READ           0x03
#define QSPI_CMD_PAGE_PROGRAM   0x02
#define QSPI_CMD_SECTOR_ERASE   0x20
#define QSPI_CMD_READ_STATUS    0x05
#define QSPI_CMD_WRITE_ENABLE   0x06

/* ── QSPI helpers (simplified indirect mode) ─────────────────────── */
static int qspi_wait_ready(uint32_t timeout_ms) {
    /* In a real implementation, we'd read the flash status register
     * via QSPI and check the busy bit. Here we use a delay. */
    delay_ms(timeout_ms);
    return 0;
}

static int qspi_erase_sector(uint32_t addr) {
    /* Send write enable, then sector erase command via QSPI indirect mode.
     * This is a simplified implementation. */
    (void)addr;
    qspi_wait_ready(50);  /* typical sector erase time: 50 ms */
    return 0;
}

static int qspi_write_page(uint32_t addr, const void *buf, uint32_t len) {
    /* Page program: up to 256 bytes per page.
     * In a real implementation, we'd configure the QSPI peripheral
     * for indirect write mode and DMA the data. */
    (void)addr;
    (void)buf;
    (void)len;
    qspi_wait_ready(3);  /* typical page program time: 3 ms */
    return 0;
}

static int qspi_read(uint32_t addr, void *buf, uint32_t len) {
    /* Read data from QSPI flash in indirect mode.
     * In a real implementation, this uses the QSPI peripheral. */
    (void)addr;
    (void)buf;
    (void)len;
    return 0;
}

/* ── Slot management ──────────────────────────────────────────────── */
static int config_read_slot(uint32_t offset, config_t *cfg) {
    int rc = qspi_read(offset, cfg, sizeof(config_t));
    if (rc != 0) return -1;

    /* Validate magic */
    if (cfg->magic != CONFIG_MAGIC) return -1;

    /* Validate CRC */
    uint32_t crc = crc32(cfg, offsetof(config_t, crc32));
    if (crc != cfg->crc32) return -1;

    return 0;
}

static int config_write_slot(uint32_t offset, const config_t *cfg) {
    /* Erase sector first */
    int rc = qspi_erase_sector(offset);
    if (rc != 0) return -1;

    /* Write data */
    rc = qspi_write_page(offset, cfg, sizeof(config_t));
    if (rc != 0) return -1;

    return 0;
}

/* ── Public API ───────────────────────────────────────────────────── */

int config_load(void) {
    config_t cfg_a, cfg_b;
    int rc_a = config_read_slot(CONFIG_FLASH_OFFSET_A, &cfg_a);
    int rc_b = config_read_slot(CONFIG_FLASH_OFFSET_B, &cfg_b);

    if (rc_a == 0 && rc_b == 0) {
        /* Both valid — pick highest version */
        if (cfg_b.version > cfg_a.version)
            g_config = cfg_b;
        else
            g_config = cfg_a;
    } else if (rc_a == 0) {
        g_config = cfg_a;
    } else if (rc_b == 0) {
        g_config = cfg_b;
    } else {
        /* No valid config — use defaults */
        config_set_defaults();
    }

    return 0;
}

int config_save(void) {
    /* Compute CRC */
    g_config.magic = CONFIG_MAGIC;
    g_config.version++;
    g_config.crc32 = crc32(&g_config, offsetof(config_t, crc32));

    /* Write to the older slot (A/B alternation) */
    static uint8_t active_slot = 0;  /* 0=A, 1=B */
    uint32_t offset;

    if (active_slot == 0) {
        offset = CONFIG_FLASH_OFFSET_B;
        active_slot = 1;
    } else {
        offset = CONFIG_FLASH_OFFSET_A;
        active_slot = 0;
    }

    return config_write_slot(offset, &g_config);
}

int config_set_defaults(void) {
    g_config.magic = CONFIG_MAGIC;
    g_config.version = 0;
    g_config.safety_interlock = 1;
    g_config.auto_discharge = 1;

    /* EMFI defaults */
    g_config.emfi.voltage = 200;
    g_config.emfi.width_ns = 50;
    g_config.emfi.delay_ns = 1000;
    g_config.emfi.count = 1;
    g_config.emfi.interval_us = 0;

    /* SCA defaults */
    g_config.sca.samples = 16384;
    g_config.sca.rate_khz = 800;
    g_config.sca.gain_db = 24;
    g_config.sca.filter_cutoff = 50000;
    g_config.sca.range_5v = 1;

    /* Trigger default */
    g_config.trig_src = TRIG_EXTERNAL;

    /* BLE PSK — default to all zeros (must be set by user) */
    for (int i = 0; i < 16; i++)
        g_config.ble_psk[i] = 0;

    g_config.crc32 = crc32(&g_config, offsetof(config_t, crc32));

    return 0;
}

int config_set_emfi(const emfi_params_t *params) {
    if (params == NULL) return -1;
    if (params->voltage < 50 || params->voltage > 400) return -1;
    if (params->width_ns < 5 || params->width_ns > 200) return -1;
    g_config.emfi = *params;
    return config_save();
}

int config_set_sca(const sca_params_t *params) {
    if (params == NULL) return -1;
    if (params->rate_khz == 0 || params->rate_khz > ADC_MAX_RATE_KSPS) return -1;
    if (params->gain_db > 48) return -1;
    g_config.sca = *params;
    return config_save();
}