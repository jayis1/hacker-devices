/*
 * config.c — Persistent configuration in SPI NOR for CoilGrip
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Stores key/value pairs (BLE pairing key, default mode, glitch presets,
 * fingerprint DB pointer) in the W25Q128 SPI NOR. We use a tiny
 * flat-table layout: 256 slots of (16-byte key + 48-byte value), with a
 * 1-byte valid flag. The first sector (4 KB) is the config region.
 */

#include "board.h"
#include "registers.h"

#define CONFIG_BASE_ADDR     0x000000U   /* SPI NOR offset */
#define CONFIG_MAX_ENTRIES   256U
#define CONFIG_KEY_LEN       16U
#define CONFIG_VAL_LEN       48U
#define CONFIG_SLOT_LEN      (CONFIG_KEY_LEN + CONFIG_VAL_LEN + 1U)  /* +valid flag */

static bool g_loaded = false;
static uint8_t g_cache[CONFIG_MAX_ENTRIES * CONFIG_SLOT_LEN];

/* ---- SPI NOR (QSPI) skeleton ------------------------------------------ */
/* The W25Q128 is on the QSPI peripheral; we use indirect mode. */
#define QUADSPI_BASE          0x48005000U
#define QUADSPI               ((volatile uint32_t *)QUADSPI_BASE)

#define QSPI_CR                0x00
#define QSPI_DCR               0x04
#define QSPI_SR                 0x08
#define QSPI_FCR               0x0C
#define QSPI_DLR                0x10
#define QSPI_CCR                0x14
#define QSPI_AR                 0x18
#define QSPI_DR                 0x20

static void qspi_wait_busy(void)
{
    while (QUADSPI[QSPI_SR / 4] & BIT(5)) { }  /* BUSY */
}

static int qspi_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    QUADSPI[QSPI_DLR / 4] = len - 1;
    QUADSPI[QSPI_CCR / 4] = (1U << 31) | (0x03U << 26) | (3U << 18) | (3U << 24) | 0x6B;
    QUADSPI[QSPI_AR / 4] = addr;
    qspi_wait_busy();
    for (uint16_t i = 0; i < len; i++)
        buf[i] = (uint8_t)QUADSPI[QSPI_DR / 4];
    return 0;
}

static int qspi_write_enable(void)
{
    QUADSPI[QSPI_CCR / 4] = (0x06U << 24);
    qspi_wait_busy();
    return 0;
}

static int qspi_program(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    qspi_write_enable();
    QUADSPI[QSPI_DLR / 4] = len - 1;
    QUADSPI[QSPI_CCR / 4] = (1U << 31) | (0x02U << 24) | (3U << 18);
    QUADSPI[QSPI_AR / 4] = addr;
    for (uint16_t i = 0; i < len; i++)
        QUADSPI[QSPI_DR / 4] = buf[i];
    qspi_wait_busy();
    return 0;
}

static int qspi_erase_sector(uint32_t addr)
{
    qspi_write_enable();
    QUADSPI[QSPI_DLR / 4] = 0;
    QUADSPI[QSPI_CCR / 4] = (0x20U << 24);
    QUADSPI[QSPI_AR / 4] = addr;
    qspi_wait_busy();
    return 0;
}

/* ---- Public API -------------------------------------------------------- */

int config_load(void)
{
    /* Read the whole config region into RAM cache (8 KB). */
    int rc = qspi_read(CONFIG_BASE_ADDR, g_cache, sizeof(g_cache));
    g_loaded = (rc == 0);
    return rc;
}

int config_save(void)
{
    if (!g_loaded) return -1;
    qspi_erase_sector(CONFIG_BASE_ADDR);
    /* Program in 256-byte pages */
    for (uint16_t off = 0; off < sizeof(g_cache); off += 256)
        qspi_program(CONFIG_BASE_ADDR + off, &g_cache[off], 256);
    return 0;
}

/* Small local strlen to avoid pulling in libc */
static uint8_t strlen_local(const char *s)
{
    uint8_t n = 0;
    while (s && s[n] && n < CONFIG_KEY_LEN) n++;
    return n;
}

static int find_slot(const char *key)
{
    if (!g_loaded) return -1;
    for (uint16_t i = 0; i < CONFIG_MAX_ENTRIES; i++) {
        uint16_t base = i * CONFIG_SLOT_LEN;
        if (g_cache[base + CONFIG_KEY_LEN + CONFIG_VAL_LEN] != 0xFF) continue;  /* not valid */
        /* Compare key (zero-padded) */
        bool match = true;
        for (uint8_t k = 0; k < CONFIG_KEY_LEN; k++) {
            uint8_t a = g_cache[base + k];
            uint8_t b = (k < (uint8_t)strlen_local(key)) ? (uint8_t)key[k] : 0;
            if (a != b) { match = false; break; }
        }
        if (match) return (int)i;
    }
    return -1;
}

int config_get(const char *key, char *val_out, uint16_t val_sz)
{
    int slot = find_slot(key);
    if (slot < 0) return -1;
    uint16_t base = (uint16_t)slot * CONFIG_SLOT_LEN + CONFIG_KEY_LEN;
    for (uint16_t i = 0; i < CONFIG_VAL_LEN && i < val_sz; i++) {
        val_out[i] = (char)g_cache[base + i];
        if (g_cache[base + i] == 0) break;
    }
    val_out[val_sz - 1] = 0;
    return 0;
}

int config_set(const char *key, const char *val)
{
    if (!g_loaded) return -1;
    int slot = find_slot(key);
    if (slot < 0) {
        /* Find free slot */
        for (uint16_t i = 0; i < CONFIG_MAX_ENTRIES; i++) {
            if (g_cache[i * CONFIG_SLOT_LEN + CONFIG_KEY_LEN + CONFIG_VAL_LEN] != 0xFF)
                { slot = (int)i; break; }
        }
        if (slot < 0) return -1;  /* table full */
    }
    uint16_t base = (uint16_t)slot * CONFIG_SLOT_LEN;
    /* Clear key */
    for (uint8_t i = 0; i < CONFIG_KEY_LEN; i++) g_cache[base + i] = 0;
    /* Copy key */
    uint8_t kl = strlen_local(key);
    for (uint8_t i = 0; i < kl; i++) g_cache[base + i] = (uint8_t)key[i];
    /* Clear + copy value */
    for (uint8_t i = 0; i < CONFIG_VAL_LEN; i++)
        g_cache[base + CONFIG_KEY_LEN + i] = (i < (uint8_t)strlen_local(val)) ? (uint8_t)val[i] : 0;
    /* Mark valid */
    g_cache[base + CONFIG_KEY_LEN + CONFIG_VAL_LEN] = 0xFF;
    return 0;
}

uint16_t config_get_count(void)
{
    uint16_t n = 0;
    for (uint16_t i = 0; i < CONFIG_MAX_ENTRIES; i++)
        if (g_cache[i * CONFIG_SLOT_LEN + CONFIG_KEY_LEN + CONFIG_VAL_LEN] == 0xFF) n++;
    return n;
}