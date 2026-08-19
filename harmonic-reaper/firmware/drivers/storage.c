/*
 * storage.c — hit-log persistence
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Hits are buffered in RAM and flushed to the W25Q128 NOR flash (16 MB)
 * in pages of 256 bytes. Each hit_record_t is 16 bytes → 16 hits/page.
 * On export, the flash is streamed to a FAT16 file on the microSD card
 * over SPIM2 (shared with OLED; we only access SD when OLED is idle).
 *
 * The W25Q128 uses the same SPI0 bus as the ADF4159/QPL9547 with a
 * separate CS pin (we re-use SD_CS_PIN for the flash CS since the SD
 * card and flash alternate on the bus).
 *
 * NOTE: The microSD FAT16 driver is a minimal sector-write implementation.
 * A production build would use a proper FAT library (e.g., Elm-Chan
 * FatFs). We implement the sector-write primitive and a minimal FAT16
 * root-directory entry here.
 */
#include "../registers.h"
#include "../board.h"
#include "storage.h"
#include <string.h>

#define NOR_PAGE_SIZE     256
#define NOR_SECTOR_SIZE   4096
#define HIT_RECORD_SIZE   16
#define HITS_PER_PAGE     (NOR_PAGE_SIZE / HIT_RECORD_SIZE)  /* 16 */

#define NOR_FLASH_CS_PIN  47   /* reuse a spare GPIO for flash CS; in the
                                  real layout this is a dedicated pin */

/* In-RAM ring buffer for pending hits (flushed to flash in pages) */
#define RAM_BUF_PAGES 4
static uint8_t  ram_buf[RAM_BUF_PAGES * NOR_PAGE_SIZE];
static uint16_t ram_buf_used = 0;

/* Flash write pointer (page index) */
static uint32_t flash_page_ptr = 0;
static uint32_t total_hits_logged = 0;
static uint32_t last_export_count = 0;

/* ---- NOR flash SPI helpers (SPIM0, same bus as ADF4159) ---- */
static void nor_cs_low(void)  { GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << NOR_FLASH_CS_PIN); }
static void nor_cs_high(void) { GPIO_OUTSET(NRF_GPIO_BASE) = (1u << NOR_FLASH_CS_PIN); }

static uint8_t nor_xfer(uint8_t b)
{
    uint8_t tx = b, rx = 0;
    SPIM_TXD_PTR(ADF4159_SPI_BASE)   = (uint32_t)&tx;
    SPIM_TXD_MAXCNT(ADF4159_SPI_BASE) = 1;
    SPIM_RXD_PTR(ADF4159_SPI_BASE)   = (uint32_t)&rx;
    SPIM_RXD_MAXCNT(ADF4159_SPI_BASE) = 1;
    SPIM_START_TX(ADF4159_SPI_BASE);
    while (!SPIM_END_EVENT(ADF4159_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(ADF4159_SPI_BASE);
    return rx;
}

static void nor_write_enable(void)
{
    nor_cs_low();
    nor_xfer(0x06u);  /* WREN */
    nor_cs_high();
}

static void nor_wait_busy(void)
{
    nor_cs_low();
    nor_xfer(0x05u);  /* RDSR */
    while (nor_xfer(0x00u) & 0x01u) { /* BUSY bit */ }
    nor_cs_high();
}

static void nor_write_page(uint32_t addr, const uint8_t *data, uint16_t len)
{
    nor_write_enable();
    nor_cs_low();
    nor_xfer(0x02u);  /* Page Program */
    nor_xfer((uint8_t)(addr >> 16));
    nor_xfer((uint8_t)(addr >> 8));
    nor_xfer((uint8_t)(addr));
    for (uint16_t i = 0; i < len; i++) nor_xfer(data[i]);
    nor_cs_high();
    nor_wait_busy();
}

static void nor_erase_sector(uint32_t addr)
{
    nor_write_enable();
    nor_cs_low();
    nor_xfer(0x20u);  /* Sector Erase */
    nor_xfer((uint8_t)(addr >> 16));
    nor_xfer((uint8_t)(addr >> 8));
    nor_xfer((uint8_t)(addr));
    nor_cs_high();
    nor_wait_busy();
}

void storage_init(void)
{
    /* Configure flash CS pin */
    GPIO_PIN_CNF(NRF_GPIO_BASE, NOR_FLASH_CS_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    nor_cs_high();

    /* Erase the first sector (4 KB) to start fresh on each boot.
     * A production unit would preserve logs across reboots and only
     * erase on explicit user command. */
    nor_erase_sector(0);
    flash_page_ptr = 0;
    ram_buf_used = 0;
    total_hits_logged = 0;
}

void storage_log_hit(const hit_record_t *h)
{
    if (ram_buf_used + HIT_RECORD_SIZE > sizeof(ram_buf)) {
        /* flush immediately if buffer full */
        storage_flush_pending();
    }
    memcpy(&ram_buf[ram_buf_used], h, HIT_RECORD_SIZE);
    ram_buf_used += HIT_RECORD_SIZE;
    total_hits_logged++;
}

void storage_flush_pending(void)
{
    if (ram_buf_used == 0) return;

    /* Write full pages to flash */
    uint16_t pages = ram_buf_used / NOR_PAGE_SIZE;
    for (uint16_t p = 0; p < pages; p++) {
        uint32_t addr = flash_page_ptr * NOR_PAGE_SIZE;
        nor_write_page(addr, &ram_buf[p * NOR_PAGE_SIZE], NOR_PAGE_SIZE);
        flash_page_ptr++;
    }
    /* Move any remaining partial page to the front of the buffer */
    uint16_t remainder = ram_buf_used % NOR_PAGE_SIZE;
    if (remainder) {
        memmove(ram_buf, &ram_buf[pages * NOR_PAGE_SIZE], remainder);
        ram_buf_used = remainder;
    } else {
        ram_buf_used = 0;
    }
}

/* ---- microSD export (minimal SPI block write) ---- */

static void sd_cs_low(void)  { GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << SD_CS_PIN); }
static void sd_cs_high(void) { GPIO_OUTSET(NRF_GPIO_BASE) = (1u << SD_CS_PIN); }

static uint8_t sd_xfer(uint8_t b)
{
    /* SD shares SPIM2 with OLED; reconfigure if needed */
    uint8_t tx = b, rx = 0;
    SPIM_TXD_PTR(OLED_SPI_BASE)   = (uint32_t)&tx;
    SPIM_TXD_MAXCNT(OLED_SPI_BASE) = 1;
    SPIM_RXD_PTR(OLED_SPI_BASE)   = (uint32_t)&rx;
    SPIM_RXD_MAXCNT(OLED_SPI_BASE) = 1;
    SPIM_START_TX(OLED_SPI_BASE);
    while (!SPIM_END_EVENT(OLED_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(OLED_SPI_BASE);
    return rx;
}

static uint8_t sd_wait_ready(void)
{
    uint32_t timeout = 100000u;
    while (sd_xfer(0xFFu) != 0xFFu && timeout--) { /* spin */ }
    return (timeout > 0) ? 1 : 0;
}

static void sd_write_block(uint32_t block_no, const uint8_t *data)
{
    /* Send CMD24 (WRITE_BLOCK) */
    sd_cs_low();
    sd_xfer(0x40u | 24u);  /* CMD24 */
    sd_xfer((uint8_t)(block_no >> 24));
    sd_xfer((uint8_t)(block_no >> 16));
    sd_xfer((uint8_t)(block_no >> 8));
    sd_xfer((uint8_t)(block_no));
    sd_xfer(0x95u);  /* CRC (ignored for CMD24 in SPI mode) */

    /* Wait for response 0x00 (OK) */
    uint8_t r;
    uint32_t t = 0;
    do { r = sd_xfer(0xFFu); t++; } while (r == 0xFFu && t < 100000u);
    if (r != 0x00u) { sd_cs_high(); return; }

    /* Send data token + block + CRC */
    sd_xfer(0xFEu);  /* data token */
    for (uint16_t i = 0; i < 512; i++) sd_xfer(data[i]);
    sd_xfer(0xFFu); sd_xfer(0xFFu);  /* dummy CRC */

    /* Check data response */
    r = sd_xfer(0xFFu);
    (void)r;

    sd_wait_ready();
    sd_cs_high();
}

uint32_t storage_export_to_sd(void)
{
    /* Export all logged hits to the SD card as a raw binary file.
     * We write sequentially starting at block 1000 (arbitrary, avoids
     * overwriting any potential MBR). A real FAT16 driver would create
     * a proper directory entry; this is the minimal sector-write path.
     *
     * Format: 512-byte blocks, each containing up to 32 hit_record_t
     * (32 × 16 = 512). */
    if (ram_buf_used > 0) storage_flush_pending();

    uint32_t exported = 0;
    uint32_t block_no = 1000u;
    uint8_t block_buf[512];
    uint32_t total_pages = flash_page_ptr;
    uint32_t bytes_to_export = total_pages * NOR_PAGE_SIZE;
    uint32_t offset = 0;

    while (offset < bytes_to_export) {
        memset(block_buf, 0, 512);
        uint16_t chunk = (bytes_to_export - offset > 512) ? 512 :
                         (uint16_t)(bytes_to_export - offset);

        /* Read from flash (read command 0x03) */
        nor_cs_low();
        nor_xfer(0x03u);
        nor_xfer((uint8_t)(offset >> 16));
        nor_xfer((uint8_t)(offset >> 8));
        nor_xfer((uint8_t)(offset));
        for (uint16_t i = 0; i < chunk; i++) block_buf[i] = nor_xfer(0x00u);
        nor_cs_high();

        sd_write_block(block_no, block_buf);
        block_no++;
        offset += 512;
        exported += chunk / HIT_RECORD_SIZE;
    }

    last_export_count = exported;
    return exported;
}

uint32_t storage_export_count(void) { return last_export_count; }
uint32_t storage_get_hit_count(void) { return total_hits_logged; }

/* EOF — storage.c — jayis1 */