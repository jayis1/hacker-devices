/*
 * drivers/sdcard.c — micro-SD SPI mode: scenario load & log write
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal SD card driver (SPI mode, CMD0/8/55/41/CMD17/CMD24) with a
 * tiny flat-file convention for storing scenarios and logs. We avoid a
 * full FAT filesystem to keep code size down; scenarios occupy fixed
 * block ranges (slot N -> block 1+N) and logs grow from block 32 onward.
 */
#include "../board.h"
#include "../registers.h"

/* ---- SD SPI low-level --------------------------------------------- */
static void sd_cs_low(void)  { spi_cs_low(SD_CS_PORT, SD_CS_PIN); }
static void sd_cs_high(void) { spi_cs_high(SD_CS_PORT, SD_CS_PIN); }

static uint8_t sd_xfer(uint8_t d)
{
    return spi_xfer(SPI4_BASE, d);
}

static void sd_wait_ready(void)
{
    uint16_t n = 0xFFFF;
    uint8_t r;
    do { r = sd_xfer(0xFF); } while (r != 0xFF && --n);
}

static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r, n = 8;
    sd_cs_low();
    sd_xfer(0x40 | cmd);
    sd_xfer((arg >> 24) & 0xFF);
    sd_xfer((arg >> 16) & 0xFF);
    sd_xfer((arg >> 8)  & 0xFF);
    sd_xfer(arg & 0xFF);
    sd_xfer(crc | 0x01);
    do { r = sd_xfer(0xFF); } while (r & 0x80 && --n);
    return r;
}

static uint8_t sd_read_block(uint8_t *buf, uint32_t lba)
{
    uint8_t r = sd_cmd(17, lba, 0);
    if (r != 0x00) { sd_cs_high(); return 0; }
    /* wait for data token 0xFE */
    uint16_t n = 0xFFFF;
    uint8_t t;
    do { t = sd_xfer(0xFF); } while (t != 0xFE && --n);
    if (t != 0xFE) { sd_cs_high(); return 0; }
    for (uint16_t i = 0; i < SD_BLOCK_SZ; i++) buf[i] = sd_xfer(0xFF);
    /* discard CRC */
    sd_xfer(0xFF); sd_xfer(0xFF);
    sd_cs_high();
    return 1;
}

static uint8_t sd_write_block(const uint8_t *buf, uint32_t lba)
{
    uint8_t r = sd_cmd(24, lba, 0);
    if (r != 0x00) { sd_cs_high(); return 0; }
    sd_xfer(0xFF);
    sd_xfer(0xFE);   /* data token */
    for (uint16_t i = 0; i < SD_BLOCK_SZ; i++) sd_xfer(buf[i]);
    sd_xfer(0xFF); sd_xfer(0xFF);   /* dummy CRC */
    uint8_t t = sd_xfer(0xFF) & 0x1F;
    if (t != 0x05) { sd_cs_high(); return 0; }
    sd_wait_ready();
    sd_cs_high();
    return 1;
}

/* ---- Init --------------------------------------------------------- */
uint8_t sd_init(void)
{
    sd_cs_high();
    /* send 80 dummy clocks to enter SPI mode */
    for (uint8_t i = 0; i < 10; i++) sd_xfer(0xFF);
    /* CMD0: reset (idle) */
    uint8_t r = sd_cmd(0, 0, 0x4A);
    if (r != 0x01) return 0;
    /* CMD8: check voltage range */
    r = sd_cmd(8, 0x000001AA, 0x43);
    (void)r;
    /* CMD55 + ACMD41: initialize */
    uint16_t n = 0;
    do {
        sd_cmd(55, 0, 0);
        r = sd_cmd(41, 0x40000000, 0);
        if (++n > 1000) return 0;
    } while (r != 0x00);
    return 1;
}

uint8_t sd_present(void)
{
    return (GPIO_IDR(SD_CD_PORT) & GPIO_PIN(SD_CD_PIN)) ? 0 : 1;
}

/* ---- Scenario slots ----------------------------------------------- */
#define SCN_BASE_BLOCK   1
#define LOG_BASE_BLOCK   64
#define LOG_MAX_BLOCKS   1024

uint8_t sd_load_scenario(uint8_t slot, uint8_t *buf, uint16_t *len)
{
    if (slot >= RP_SCN_MAX_SLOTS) return 0;
    static uint8_t block[SD_BLOCK_SZ];
    if (!sd_read_block(block, SCN_BASE_BLOCK + slot)) return 0;
    /* first 2 bytes = length, then bytecode */
    uint16_t l = (uint16_t)block[0] | ((uint16_t)block[1] << 8);
    if (l == 0 || l > RP_SCN_MAX_OPS) return 0;
    for (uint16_t i = 0; i < l && i < SD_BLOCK_SZ - 2; i++)
        buf[i] = block[2 + i];
    *len = l;
    return 1;
}

uint8_t sd_save_scenario(uint8_t slot, const uint8_t *buf, uint16_t len)
{
    if (slot >= RP_SCN_MAX_SLOTS || len > RP_SCN_MAX_OPS) return 0;
    static uint8_t block[SD_BLOCK_SZ];
    block[0] = (uint8_t)(len & 0xFF);
    block[1] = (uint8_t)(len >> 8);
    for (uint16_t i = 0; i < len; i++) block[2 + i] = buf[i];
    for (uint16_t i = 2 + len; i < SD_BLOCK_SZ; i++) block[i] = 0;
    return sd_write_block(block, SCN_BASE_BLOCK + slot);
}

/* ---- Logging ------------------------------------------------------ */
static uint32_t log_next_block;
static uint16_t log_entry_count;

void sd_log_reset(void)
{
    log_next_block = LOG_BASE_BLOCK;
    log_entry_count = 0;
}

void sd_log_open(void)
{
    log_next_block = LOG_BASE_BLOCK;
    log_entry_count = 0;
}

void sd_log_append(const uint8_t *entry, uint8_t len)
{
    /* simple: each entry occupies one 512-byte block (wasteful but simple) */
    if (len > SD_BLOCK_SZ - 4) len = SD_BLOCK_SZ - 4;
    static uint8_t block[SD_BLOCK_SZ];
    block[0] = RP_LOG_MAGIC & 0xFF;
    block[1] = (RP_LOG_MAGIC >> 8) & 0xFF;
    block[2] = len;
    block[3] = 0;
    for (uint8_t i = 0; i < len; i++) block[4 + i] = entry[i];
    for (uint16_t i = 4 + len; i < SD_BLOCK_SZ; i++) block[i] = 0;
    sd_write_block(block, log_next_block);
    log_next_block++;
    log_entry_count++;
    if (log_entry_count > RP_LOG_MAX_ENTRIES) {
        log_next_block = LOG_BASE_BLOCK;
        log_entry_count = 0;
    }
}

uint16_t sd_log_count(void) { return log_entry_count; }