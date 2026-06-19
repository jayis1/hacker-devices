/*
 * flash.c — W25Q128 external QSPI flash driver (16 MB)
 *
 * Stores: the QCA7420 PIB image at offset 0x00000 (32 KB), key backup at
 * 0x100000 (4 KB, encrypted), and a captured-frame ring-buffer overflow
 * region at 0x200000.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "flash.h"
#include "../board.h"
#include "../registers.h"

#define QSPI_CR   (*(volatile uint32_t *)(QUADSPI_BASE + 0x00))
#define QSPI_DCR  (*(volatile uint32_t *)(QUADSPI_BASE + 0x04))
#define QSPI_SR   (*(volatile uint32_t *)(QUADSPI_BASE + 0x08))
#define QSPI_FCR  (*(volatile uint32_t *)(QUADSPI_BASE + 0x0C))
#define QSPI_DLR  (*(volatile uint32_t *)(QUADSPI_BASE + 0x10))
#define QSPI_CCR  (*(volatile uint32_t *)(QUADSPI_BASE + 0x14))
#define QSPI_AR   (*(volatile uint32_t *)(QUADSPI_BASE + 0x18))
#define QSPI_DR   (*(volatile uint32_t *)(QUADSPI_BASE + 0x20))

#define QSPI_CCR_FMODE_IND_READ  (0U << 4)
#define QSPI_CCR_FMODE_IND_WRITE (1U << 4)
#define QSPI_CCR_DMODE_4LINE     (3U << 24)
#define QSPI_CCR_IMODE_1LINE     (1U << 8)
#define QSPI_CCR_ADMODE_4LINE    (3U << 12)
#define QSPI_CCR_ADSIZE_24       (2U << 20)

static int g_qspi_init = 0;

static void qspi_wait_busy(void) {
    while (QSPI_SR & (1U << 5)) { } /* BUSY */
}

int flash_init(void) {
    /* Configure QSPI: prescaler /2 = 120 MHz, FSEL=1 (high), DFM=0 */
    QSPI_CR = (1U << 0) | (1U << 1) | (2U << 24); /* EN + DMAEN + prescaler */
    QSPI_DCR = (1U << 0) | (3U << 1) | (1U << 8) | (1U << 17) | (1U << 18);
    /* Flash size: W25Q128 = 128 Mbit = 16 MB. CSHT = 4 cycles. */
    QSPI_DCR |= (22U << 3); /* FSIZE = log2(16MB) = 23, but mask */
    g_qspi_init = 1;
    return 0;
}

int flash_read(uint32_t addr, uint8_t *buf, uint32_t len) {
    if (!g_qspi_init) return -1;
    QSPI_DLR = len - 1;
    QSPI_CCR = QSPI_CCR_FMODE_IND_READ | QSPI_CCR_DMODE_4LINE
             | QSPI_CCR_ADMODE_4LINE | QSPI_CCR_ADSIZE_24 | QSPI_CCR_IMODE_1LINE
             | 0x6B; /* quad read opcode */
    QSPI_AR = addr;
    qspi_wait_busy();
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = *(volatile uint8_t *)&QSPI_DR;
    }
    return (int)len;
}

int flash_write(uint32_t addr, const uint8_t *buf, uint32_t len) {
    if (!g_qspi_init) return -1;
    /* Page program (256-byte pages); assumes WEL set + sector erased */
    QSPI_DLR = len - 1;
    QSPI_CCR = QSPI_CCR_FMODE_IND_WRITE | QSPI_CCR_DMODE_4LINE
             | QSPI_CCR_ADMODE_4LINE | QSPI_CCR_ADSIZE_24 | QSPI_CCR_IMODE_1LINE
             | 0x32; /* quad page program */
    QSPI_AR = addr;
    qspi_wait_busy();
    for (uint32_t i = 0; i < len; i++) {
        *(volatile uint8_t *)&QSPI_DR = buf[i];
    }
    return (int)len;
}

int flash_erase_sector(uint32_t addr) {
    if (!g_qspi_init) return -1;
    QSPI_DLR = 0;
    QSPI_CCR = QSPI_CCR_FMODE_IND_WRITE
             | QSPI_CCR_ADMODE_4LINE | QSPI_CCR_ADSIZE_24 | QSPI_CCR_IMODE_1LINE
             | 0x20; /* sector erase 4KB */
    QSPI_AR = addr;
    qspi_wait_busy();
    return 0;
}

int flash_read_pib(uint8_t *buf, uint32_t maxlen) {
    static uint32_t off = 0;
    if (off >= 32 * 1024) return 0;
    uint32_t n = (maxlen < (32 * 1024 - off)) ? maxlen : (32 * 1024 - off);
    if (flash_read(off, buf, n) != (int)n) return -1;
    off += n;
    return (int)n;
}

void flash_wipe_keys(void) {
    uint8_t zero[256];
    memset(zero, 0, sizeof(zero));
    for (uint32_t a = 0x100000; a < 0x101000; a += 256) {
        flash_erase_sector(a);
        flash_write(a, zero, 256);
    }
}

int16_t flash_read_temp(void) {
    /* Read DS18B20 1-wire temperature (simplified: return fixed 25 C).
     * Real impl bit-bangs PC8 1-wire and parses the scratchpad.
     */
    return 25;
}