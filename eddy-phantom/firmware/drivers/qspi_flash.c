/*
 * eddy-phantom — qspi_flash.c
 * QSPI flash driver for W25Q128JVSIQ (16 MB).
 * Provides read, write, and erase operations via the QSPI peripheral.
 * Model weights and keyboard profiles are stored here.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

#define W25Q128_PAGE_SIZE       256
#define W25Q128_SECTOR_SIZE     4096
#define W25Q128_BLOCK_SIZE      65536
#define W25Q128_TOTAL_SIZE      (16 * 1024 * 1024)

/* Flash commands */
#define CMD_READ_DATA           0x03
#define CMD_FAST_READ           0x0B
#define CMD_FAST_READ_QUAD      0xEB
#define CMD_PAGE_PROGRAM        0x02
#define CMD_QUAD_PAGE_PROGRAM   0x32
#define CMD_SECTOR_ERASE        0x20
#define CMD_BLOCK_ERASE         0xD8
#define CMD_CHIP_ERASE          0xC7
#define CMD_READ_STATUS         0x05
#define CMD_READ_STATUS2        0x35
#define CMD_WRITE_ENABLE        0x06
#define CMD_WRITE_DISABLE       0x04
#define CMD_READ_ID             0x9F
#define CMD_ENABLE_QSPI         0x38
#define CMD_RESET_ENABLE        0x66
#define CMD_RESET               0x99

/* ── Wait for flash idle (WIP bit clear) ──────────────────────── */
static int qspi_wait_idle(uint32_t timeout_ms)
{
    uint32_t start = 0;  /* would use board_millis() but keep self-contained */
    while (QSPI->SR & QSPI_SR_BUSY) {
        if (start++ > timeout_ms * 1000)
            return -1;
    }
    return 0;
}

/* ── Send indirect-mode command ───────────────────────────────── */
static void qspi_send_cmd(uint8_t cmd, uint32_t addr, int addr_mode)
{
    /* Configure CCR for indirect mode */
    QSPI->CCR = (0U << 26)           /* FMODE = indirect */
              | (1U << 24)           /* DMODE = no data */
              | (addr_mode ? (1U << 16) : 0)  /* ADSIZE = 24-bit if addr */
              | (addr_mode ? (1U << 12) : 0)  /* ADMODE = 1-line */
              | (1U << 18)           /* IMODE = 1-line */
              | cmd;                 /* instruction */

    if (addr_mode) {
        QSPI->AR = addr;
    }

    while (QSPI->SR & QSPI_SR_BUSY);
}

/* ── Read from QSPI flash (memory-mapped) ─────────────────────── */
int qspi_read(uint32_t addr, void *buf, uint32_t len)
{
    if (addr + len > W25Q128_TOTAL_SIZE)
        return -1;

    /* Memory-mapped read: QSPI is configured in memory-mapped mode
     * during board_init. We just copy from the memory-mapped region. */
    volatile uint8_t *src = (volatile uint8_t *)(QSPI_MM_BASE + addr);
    uint8_t *dst = (uint8_t *)buf;

    for (uint32_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }

    return 0;
}

/* ── Write enable ─────────────────────────────────────────────── */
static int qspi_write_enable(void)
{
    qspi_send_cmd(CMD_WRITE_ENABLE, 0, 0);
    while (QSPI->SR & QSPI_SR_BUSY);
    return 0;
}

/* ── Wait for write completion (poll status register) ─────────── */
static int qspi_wait_write_complete(uint32_t timeout)
{
    /* Read status register via indirect mode */
    uint8_t status;
    uint32_t count = 0;

    do {
        QSPI->CCR = (0U << 26) | (1U << 18) | CMD_READ_STATUS;
        QSPI->DLR = 0;  /* 1 byte */
        while (QSPI->SR & QSPI_SR_BUSY);
        status = (uint8_t)(QSPI->DR);
        count++;
        if (count > timeout) return -1;
    } while (status & 0x01);  /* WIP bit */

    return 0;
}

/* ── Write a page (up to 256 bytes) ───────────────────────────── */
int qspi_write_page(uint32_t addr, const void *buf, uint32_t len)
{
    if (len > W25Q128_PAGE_SIZE) len = W25Q128_PAGE_SIZE;
    if (addr + len > W25Q128_TOTAL_SIZE) return -1;

    /* Ensure addr is within a single page (page boundary = 256 bytes) */
    uint32_t page_remain = W25Q128_PAGE_SIZE - (addr % W25Q128_PAGE_SIZE);
    if (len > page_remain) len = page_remain;

    qspi_write_enable();

    /* Configure for indirect write with data */
    QSPI->CCR = (0U << 26)           /* FMODE = indirect write */
              | (3U << 24)           /* DMODE = 1-line data */
              | (2U << 16)           /* ADSIZE = 24-bit */
              | (1U << 12)           /* ADMODE = 1-line */
              | (1U << 18)           /* IMODE = 1-line */
              | CMD_PAGE_PROGRAM;

    QSPI->AR = addr;
    QSPI->DLR = len - 1;

    /* Write data to DR register */
    const uint8_t *src = (const uint8_t *)buf;
    for (uint32_t i = 0; i < len; i++) {
        QSPI->DR = src[i];
    }

    /* Wait for write to complete */
    if (qspi_wait_write_complete(100000) != 0)
        return -1;

    return (int)len;
}

/* ── Erase a 4 KB sector ──────────────────────────────────────── */
int qspi_erase_sector(uint32_t addr)
{
    if (addr >= W25Q128_TOTAL_SIZE) return -1;
    addr &= ~(W25Q128_SECTOR_SIZE - 1);  /* align to sector boundary */

    qspi_write_enable();

    QSPI->CCR = (0U << 26)
              | (2U << 16)    /* ADSIZE = 24-bit */
              | (1U << 12)    /* ADMODE = 1-line */
              | (1U << 18)    /* IMODE = 1-line */
              | CMD_SECTOR_ERASE;
    QSPI->AR = addr;

    /* Sector erase takes 20-400 ms (typical 45 ms) */
    if (qspi_wait_write_complete(500000) != 0)
        return -1;

    return 0;
}

/* ── Initialize QSPI flash ────────────────────────────────────── */
int qspi_init(void)
{
    /* QSPI peripheral is initialized in board_init_qspi().
     * Here we verify the flash chip is responding by reading its ID. */

    /* Read JEDEC ID: should be 0xEF4018 for W25Q128 */
    QSPI->CCR = (0U << 26)           /* FMODE = indirect */
              | (3U << 24)           /* DMODE = 1-line data */
              | (1U << 18)           /* IMODE = 1-line */
              | CMD_READ_ID;
    QSPI->DLR = 2;  /* 3 bytes (0-indexed) */

    while (QSPI->SR & QSPI_SR_BUSY);

    uint8_t id[3];
    for (int i = 0; i < 3; i++) {
        id[i] = (uint8_t)QSPI->DR;
    }

    /* Verify manufacturer ID (0xEF = Winbond) */
    if (id[0] != 0xEF) {
        return -1;  /* wrong or no flash chip */
    }

    /* Re-enable memory-mapped mode for read operations */
    QSPI->CCR = QSPI_CCR_FMODE_MEM
              | QSPI_CCR_DMODE_4L
              | QSPI_CCR_ADMODE_4L
              | (3U << 16)
              | QSPI_CCR_IMODE_4L
              | CMD_FAST_READ_QUAD;

    while (QSPI->SR & QSPI_SR_BUSY);

    return 0;
}

/* ── Write arbitrary data (handles page crossing) ─────────────── */
int qspi_write(uint32_t addr, const void *buf, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)buf;
    uint32_t offset = 0;

    while (offset < len) {
        uint32_t page_remain = W25Q128_PAGE_SIZE - (addr % W25Q128_PAGE_SIZE);
        uint32_t chunk = (len - offset < page_remain) ?
                         (len - offset) : page_remain;

        if (qspi_write_page(addr, src + offset, chunk) != (int)chunk)
            return -1;

        addr += chunk;
        offset += chunk;
    }

    return (int)len;
}