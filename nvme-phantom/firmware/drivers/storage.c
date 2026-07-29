/*
 * drivers/storage.c — W25Q128 SPI NOR payload store
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * The W25Q128 (16 MB SPI NOR) holds:
 *   - The ECP5 FPGA bitstream (offset 0x000000, length stored in 4B header)
 *   - The Identify Controller spoof presets (offset 0x100000)
 *   - The Opal session capture buffer (offset 0x180000)
 *   - The rule-script library (offset 0x1C0000)
 *
 * This driver provides read/erase/page-program helpers over SPI3.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* External SPI3 helpers (defined in fpga_spi.c) */
extern void nor_cs_low(void);
extern void nor_cs_high(void);
extern uint8_t nor_xfer(uint8_t tx);
extern void spi3_init(void);

/* ---- W25Q128 commands -------------------------------------------------- */

#define W25Q_CMD_READ4B    0x13
#define W25Q_CMD_READ      0x03
#define W25Q_CMD_RDSR1     0x05
#define W25Q_CMD_WREN      0x06
#define W25Q_CMD_SE4B      0x21   /* sector erase 4 KB */
#define W25Q_CMD_PP4B      0x12   /* page program 4 KB */
#define W25Q_CMD_RDID      0x9F
#define W25Q_CMD_RESET     0x66
#define W25Q_CMD_RSTEN     0x99

/* ---- Helpers ----------------------------------------------------------- */

static void nor_wait_busy(void)
{
    do {
        nor_cs_low();
        nor_xfer(W25Q_CMD_RDSR1);
    } while (nor_xfer(0x00) & 0x01);
    nor_cs_high();
}

static void nor_write_enable(void)
{
    nor_cs_low();
    nor_xfer(W25Q_CMD_WREN);
    nor_cs_high();
}

/* ---- Public API -------------------------------------------------------- */

int storage_init(void)
{
    spi3_init();
    /* Read JEDEC ID: should be 0xEF 0x40 0x18 (Winbond W25Q128) */
    nor_cs_low();
    nor_xfer(W25Q_CMD_RDID);
    uint8_t mfr = nor_xfer(0xFF);
    uint8_t mem_type = nor_xfer(0xFF);
    uint8_t cap = nor_xfer(0xFF);
    nor_cs_high();
    if (mfr != 0xEF || mem_type != 0x40 || cap != 0x18) return -1;
    return 0;
}

int storage_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    nor_wait_busy();
    nor_cs_low();
    nor_xfer(W25Q_CMD_READ4B);
    nor_xfer((addr >> 24) & 0xFF);
    nor_xfer((addr >> 16) & 0xFF);
    nor_xfer((addr >> 8)  & 0xFF);
    nor_xfer( addr        & 0xFF);
    for (uint32_t i = 0; i < len; i++) buf[i] = nor_xfer(0xFF);
    nor_cs_high();
    return 0;
}

int storage_erase_sector(uint32_t addr)
{
    nor_write_enable();
    nor_cs_low();
    nor_xfer(W25Q_CMD_SE4B);
    nor_xfer((addr >> 24) & 0xFF);
    nor_xfer((addr >> 16) & 0xFF);
    nor_xfer((addr >> 8)  & 0xFF);
    nor_xfer( addr        & 0xFF);
    nor_cs_high();
    nor_wait_busy();
    return 0;
}

int storage_write_page(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (len > 256) len = 256;                   /* page size = 256 B */
    nor_write_enable();
    nor_cs_low();
    nor_xfer(W25Q_CMD_PP4B);
    nor_xfer((addr >> 24) & 0xFF);
    nor_xfer((addr >> 16) & 0xFF);
    nor_xfer((addr >> 8)  & 0xFF);
    nor_xfer( addr        & 0xFF);
    for (uint16_t i = 0; i < len; i++) nor_xfer(data[i]);
    nor_cs_high();
    nor_wait_busy();
    return 0;
}

/* Write a buffer of arbitrary length (handles page boundaries). */
int storage_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint32_t off = 0;
    while (off < len) {
        uint32_t page_remain = 256 - (addr & 0xFF);
        uint32_t n = (len - off < page_remain) ? (len - off) : page_remain;
        if (storage_write_page(addr, data + off, (uint16_t)n) != 0) return -1;
        addr += n;
        off  += n;
    }
    return 0;
}

/* ---- Payload layout helpers -------------------------------------------- */

#define NOR_OFFSET_BITSTREAM   0x000000UL
#define NOR_OFFSET_SPOOF       0x100000UL
#define NOR_OFFSET_OPAL_CAP    0x180000UL
#define NOR_OFFSET_RULES       0x1C0000UL

int storage_load_bitstream_offset(uint32_t *out_offset, uint32_t *out_len)
{
    uint8_t hdr[4];
    if (storage_read(NOR_OFFSET_BITSTREAM, hdr, 4) != 0) return -1;
    *out_offset = NOR_OFFSET_BITSTREAM;
    *out_len = ((uint32_t)hdr[0] << 24) | ((uint32_t)hdr[1] << 16) |
               ((uint32_t)hdr[2] << 8) | (uint32_t)hdr[3];
    return (*out_len > 0 && *out_len < W25Q128_SIZE_BYTES) ? 0 : -1;
}

int storage_save_spoof_preset(uint8_t index, const uint8_t ident[4096])
{
    uint32_t addr = NOR_OFFSET_SPOOF + (uint32_t)index * 4096;
    storage_erase_sector(addr);
    /* 4096 B = 16 pages of 256 B */
    for (int p = 0; p < 16; p++) {
        storage_write_page(addr + p * 256, ident + p * 256, 256);
    }
    return 0;
}

int storage_load_spoof_preset(uint8_t index, uint8_t ident[4096])
{
    return storage_read(NOR_OFFSET_SPOOF + (uint32_t)index * 4096, ident, 4096);
}

int storage_save_opal_capture(uint32_t offset, const uint8_t *data, uint32_t len)
{
    return storage_write(NOR_OFFSET_OPAL_CAP + offset, data, len);
}

int storage_save_rule(uint8_t slot, const uint8_t rule[32])
{
    return storage_write(NOR_OFFSET_RULES + (uint32_t)slot * 32, rule, 32);
}