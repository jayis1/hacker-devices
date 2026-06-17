/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Capture Buffer Manager — Implementation
 *
 * QSPI flash (W25Q128JV) and PSRAM (IS66WVH8M8DBLI) driver.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "capture_buffer.h"
#include <string.h>

/* =========================================================================
 * State
 * ========================================================================= */

capture_ring_t g_capture_ring = {0};
static int g_qspi_initialized = 0;
static int g_psram_initialized = 0;

/* =========================================================================
 * W25Q128JV Command Set
 * ========================================================================= */
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_VOLATILE_SR_WREN   0x50
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_READ_STATUS_1      0x05
#define W25Q_CMD_READ_STATUS_2      0x35
#define W25Q_CMD_READ_STATUS_3      0x15
#define W25Q_CMD_WRITE_STATUS_1     0x01
#define W25Q_CMD_READ_DATA          0x03      /* 1-line read (legacy) */
#define W25Q_CMD_FAST_READ          0x0B      /* 1-line read with dummy */
#define W25Q_CMD_QUAD_READ          0x6B      /* Quad I/O read */
#define W25Q_CMD_PAGE_PROGRAM       0x02      /* 1-line page program */
#define W25Q_CMD_QUAD_PAGE_PROGRAM  0x32      /* Quad page program */
#define W25Q_CMD_SECTOR_ERASE_4K    0x20      /* 4 KB sector erase */
#define W25Q_CMD_BLOCK_ERASE_32K    0x52      /* 32 KB block erase */
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8      /* 64 KB block erase */
#define W25Q_CMD_CHIP_ERASE         0xC7      /* Chip erase */
#define W25Q_CMD_POWER_DOWN         0xB9
#define W25Q_CMD_RELEASE_POWER_DOWN 0xAB
#define W25Q_CMD_ENABLE_QPI         0x38      /* Enable Quad I/O mode */
#define W25Q_CMD_DISABLE_QPI        0xFF
#define W25Q_CMD_READ_ID            0x9F      /* Read JEDEC ID */
#define W25Q_CMD_READ_UNIQUE_ID     0x4B

/* Status register bits */
#define W25Q_SR1_BUSY               (1U << 0)
#define W25Q_SR1_WEL                (1U << 1)
#define W25Q_SR2_QE                 (1U << 1)  /* Quad enable */

/* =========================================================================
 * QSPI Helper: Send command via indirect mode
 * ========================================================================= */

static void qspi_send_cmd(uint8_t cmd)
{
    uint32_t ccr = QSPI_CCR_IMODE_1LINE   /* Instruction on 1 line */
                 | QSPI_CCR_FMODE_INDIR;  /* Indirect write mode */
    mmio_write32(QUADSPI_BASE + QSPI_CCR, ccr | cmd);
    
    /* Wait for TC flag */
    while (!(mmio_read32(QUADSPI_BASE + QSPI_SR) & QSPI_SR_TCF))
        ;
    mmio_write32(QUADSPI_BASE + QSPI_FCR, QSPI_SR_TCF);
}

static void qspi_send_cmd_addr(uint8_t cmd, uint32_t addr)
{
    uint32_t ccr = QSPI_CCR_IMODE_1LINE
                 | QSPI_CCR_ADMODE_1LINE
                 | QSPI_CCR_ADSIZE_24    /* 3-byte address */
                 | QSPI_CCR_FMODE_INDIR;
    
    mmio_write32(QUADSPI_BASE + QSPI_CCR, ccr | cmd);
    mmio_write32(QUADSPI_BASE + QSPI_AR, addr);
    
    while (!(mmio_read32(QUADSPI_BASE + QSPI_SR) & QSPI_SR_TCF))
        ;
    mmio_write32(QUADSPI_BASE + QSPI_FCR, QSPI_SR_TCF);
}

static void qspi_send_cmd_addr_data(uint8_t cmd, uint32_t addr,
                                     const uint8_t *data, uint32_t len)
{
    uint32_t ccr = QSPI_CCR_IMODE_1LINE
                 | QSPI_CCR_ADMODE_1LINE
                 | QSPI_CCR_ADSIZE_24
                 | QSPI_CCR_DMODE_1LINE
                 | QSPI_CCR_FMODE_INDIR;

    mmio_write32(QUADSPI_BASE + QSPI_DLR, len - 1);
    mmio_write32(QUADSPI_BASE + QSPI_CCR, ccr | cmd);
    mmio_write32(QUADSPI_BASE + QSPI_AR, addr);

    /* Write data to FIFO */
    for (uint32_t i = 0; i < len; i++) {
        /* Wait for FIFO not full */
        while ((mmio_read32(QUADSPI_BASE + QSPI_SR) & QSPI_SR_FLEVEL) >= 16)
            ;
        mmio_write32(QUADSPI_BASE + QSPI_DR, data[i]);
    }

    while (!(mmio_read32(QUADSPI_BASE + QSPI_SR) & QSPI_SR_TCF))
        ;
    mmio_write32(QUADSPI_BASE + QSPI_FCR, QSPI_SR_TCF);
}

static uint8_t qspi_read_status(void)
{
    uint32_t ccr = QSPI_CCR_IMODE_1LINE
                 | QSPI_CCR_DMODE_1LINE
                 | QSPI_CCR_FMODE_INDIR;

    mmio_write32(QUADSPI_BASE + QSPI_DLR, 0); /* 1 byte */
    mmio_write32(QUADSPI_BASE + QSPI_CCR, ccr | W25Q_CMD_READ_STATUS_1);

    while (!(mmio_read32(QUADSPI_BASE + QSPI_SR) & QSPI_SR_TCF))
        ;
    
    uint8_t sr = (uint8_t)(mmio_read32(QUADSPI_BASE + QSPI_DR) & 0xFF);
    mmio_write32(QUADSPI_BASE + QSPI_FCR, QSPI_SR_TCF);
    
    return sr;
}

static void qspi_wait_ready(void)
{
    while (qspi_read_status() & W25Q_SR1_BUSY)
        ;
}

static void qspi_write_enable(void)
{
    qspi_send_cmd(W25Q_CMD_WRITE_ENABLE);
}

/* =========================================================================
 * QSPI Initialization
 * ========================================================================= */

int qspi_init(void)
{
    if (g_qspi_initialized)
        return ERR_OK;

    /* Enable QSPI clock */
    mmio_set_bits(RCC_AHB3ENR, RCC_AHB3ENR_QSPI);

    /* Configure QSPI GPIO pins as AF9 */
    uint32_t moder, afrl, afrh;

    /* PB0 = IO1 (AF9) */
    moder  = mmio_read32(GPIOB_BASE + GPIO_MODER);
    afrl   = mmio_read32(GPIOB_BASE + GPIO_AFRL);

    moder  &= ~(3U << (0 * 2));
    moder  |= (GPIO_MODE_AF << (0 * 2));
    afrl   &= ~(0xFUL << (0 * 4));
    afrl   |= (AF_QSPI << (0 * 4));

    /* PB1 = IO0 (AF9) */
    moder  &= ~(3U << (1 * 2));
    moder  |= (GPIO_MODE_AF << (1 * 2));
    afrl   &= ~(0xFUL << (1 * 4));
    afrl   |= (AF_QSPI << (1 * 4));

    /* PB2 = CLK (AF9) */
    moder  &= ~(3U << (2 * 2));
    moder  |= (GPIO_MODE_AF << (2 * 2));
    afrl   &= ~(0xFUL << (2 * 4));
    afrl   |= (AF_QSPI << (2 * 4));

    mmio_write32(GPIOB_BASE + GPIO_MODER, moder);
    mmio_write32(GPIOB_BASE + GPIO_AFRL, afrl);

    /* PC6 = IO3, PC7 = IO2, PC11 = NCS (all AF9) */
    moder  = mmio_read32(GPIOC_BASE + GPIO_MODER);
    afrl   = mmio_read32(GPIOC_BASE + GPIO_AFRL);
    afrh   = mmio_read32(GPIOC_BASE + GPIO_AFRH);

    /* PC6 */
    moder  &= ~(3U << (6 * 2));
    moder  |= (GPIO_MODE_AF << (6 * 2));
    afrl   &= ~(0xFUL << (6 * 4));
    afrl   |= (AF_QSPI << (6 * 4));

    /* PC7 */
    moder  &= ~(3U << (7 * 2));
    moder  |= (GPIO_MODE_AF << (7 * 2));
    afrl   &= ~(0xFUL << (7 * 4));
    afrl   |= (AF_QSPI << (7 * 4));

    /* PC11 */
    moder  &= ~(3U << (11 * 2));
    moder  |= (GPIO_MODE_AF << (11 * 2));
    afrh   &= ~(0xFUL << ((11 - 8) * 4));
    afrh   |= (AF_QSPI << ((11 - 8) * 4));

    mmio_write32(GPIOC_BASE + GPIO_MODER, moder);
    mmio_write32(GPIOC_BASE + GPIO_AFRL, afrl);
    mmio_write32(GPIOC_BASE + GPIO_AFRH, afrh);

    /* Set high speed */
    mmio_write32(GPIOB_BASE + GPIO_OSPEEDR, 0xFFFFFFFF);
    mmio_write32(GPIOC_BASE + GPIO_OSPEEDR, 0xFFFFFFFF);

    /* Configure QSPI peripheral */
    /* CR: prescaler = 2 (100 MHz / 2 = 50 MHz), DMA disabled, FIFO threshold = 4 */
    mmio_write32(QUADSPI_BASE + QSPI_CR, (2U << 24) | (4U << 8)); /* PRESCALER=2, FTHRES=4 */
    
    /* DCR: Flash size = 24-bit (16 MB), chip select high time = 2 clk */
    mmio_write32(QUADSPI_BASE + QSPI_DCR, (23U << 16) | (2U << 6)); /* FSIZE=23, CSHT=2 */
    
    /* Enable QSPI */
    mmio_set_bits(QUADSPI_BASE + QSPI_CR, QSPI_CR_EN);

    /* Wait for flash ready */
    qspi_wait_ready();

    /* Read JEDEC ID to verify communication */
    uint32_t ccr = QSPI_CCR_IMODE_1LINE
                 | QSPI_CCR_DMODE_1LINE
                 | QSPI_CCR_FMODE_INDIR;
    mmio_write32(QUADSPI_BASE + QSPI_DLR, 2); /* 3 bytes */
    mmio_write32(QUADSPI_BASE + QSPI_CCR, ccr | W25Q_CMD_READ_ID);
    
    while (!(mmio_read32(QUADSPI_BASE + QSPI_SR) & QSPI_SR_TCF))
        ;
    /* uint8_t id0 = (uint8_t)(mmio_read32(QUADSPI_BASE + QSPI_DR) & 0xFF); Manufacturer = 0xEF (Winbond) */
    /* uint8_t id1 = (uint8_t)((mmio_read32(QUADSPI_BASE + QSPI_DR) >> 8) & 0xFF); 0x70 for W25Q128JV */
    /* uint8_t id2 = (uint8_t)((mmio_read32(QUADSPI_BASE + QSPI_DR) >> 16) & 0xFF); 0x18 for 128M */
    mmio_write32(QUADSPI_BASE + QSPI_FCR, QSPI_SR_TCF);

    g_qspi_initialized = 1;
    return ERR_OK;
}

/* =========================================================================
 * QSPI Read/Write/Erase
 * ========================================================================= */

void qspi_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    /* Try memory-mapped mode first — alias base at 0x90000000 */
    /* The QSPI region is mapped from 0x90000000 */
    uint8_t *src = (uint8_t *)(QSPI_ALIAS_BASE + addr);
    for (uint32_t i = 0; i < len; i++)
        data[i] = src[i];
}

int qspi_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (!g_qspi_initialized) return ERR_NOT_INIT;
    if (len > 256) return ERR_INVALID_PARAM; /* Max page program size */
    
    qspi_wait_ready();
    qspi_write_enable();
    qspi_send_cmd_addr_data(W25Q_CMD_PAGE_PROGRAM, addr, data, len);
    qspi_wait_ready();
    
    return ERR_OK;
}

int qspi_erase_sector(uint32_t addr)
{
    if (!g_qspi_initialized) return ERR_NOT_INIT;
    
    qspi_wait_ready();
    qspi_write_enable();
    qspi_send_cmd_addr(W25Q_CMD_SECTOR_ERASE_4K, addr);
    qspi_wait_ready();
    
    return ERR_OK;
}

int qspi_erase_block_64k(uint32_t addr)
{
    if (!g_qspi_initialized) return ERR_NOT_INIT;
    
    qspi_wait_ready();
    qspi_write_enable();
    qspi_send_cmd_addr(W25Q_CMD_BLOCK_ERASE_64K, addr);
    qspi_wait_ready();
    
    return ERR_OK;
}

int qspi_erase_chip(void)
{
    if (!g_qspi_initialized) return ERR_NOT_INIT;
    
    qspi_wait_ready();
    qspi_write_enable();
    qspi_send_cmd(W25Q_CMD_CHIP_ERASE);
    qspi_wait_ready();
    
    return ERR_OK;
}

/* =========================================================================
 * PSRAM Initialization (FMC — NAND/PC Card controller in SRAM mode)
 * ========================================================================= */

int psram_init(void)
{
    if (g_psram_initialized)
        return ERR_OK;

    /* Enable FMC clock */
    mmio_set_bits(RCC_AHB3ENR, (1U << 4)); /* FMCEN */

    /* For the IS66WVH8M8DBLI (8 MB, 16-bit, synchronous PSRAM),
     * we configure FMC Bank 3 as SRAM/NOR mode. 
     * 
     * NOTE: A full PSRAM init would need timing configuration based on
     * the specific FMC controller. For this design, we assume the
     * memory is ready after clock and reset.
     * 
     * Simplified: FMC Bank 3 is at 0x68000000. We configure it as
     * 16-bit, no wait, asynchronous SRAM. */

    /* Configure FMC_BCR1 for Bank 3 */
    /* For STM32H7, FMC_BASE + 0x08 = BCR1 + 4 for bank 2, etc.
     * But the H7 FMC is different — it has PCR for NAND/PC Card.
     * Bank 3 is at FMC_PCR_BANK3 = 0x80 offset from FMC base. */

    uint32_t pcr = mmio_read32(FMC_BASE + FMC_PCR_BANK3);
    pcr &= ~(0x3FUL << 0);       /* Clear control bits */
    pcr |= FMC_PCR_PWAITEN;      /* Wait enable */
    pcr |= FMC_PCR_PWID_16;     /* 16-bit data bus */
    pcr |= (1U << FMC_PCR_TCLR_OFFSET);   /* TCLR = 1 HCLK */
    pcr |= (1U << FMC_PCR_TAR_OFFSET);    /* TAR = 1 HCLK */
    pcr |= FMC_PCR_PBKEN;        /* Enable bank */
    mmio_write32(FMC_BASE + FMC_PCR_BANK3, pcr);

    /* Set timing registers for async SRAM-like operation */
    /* PMEM: 10 HCLK cycles set/hold */
    mmio_write32(FMC_BASE + FMC_PMEM_BANK3, 0x0A0A0A0A);
    /* PATT: same */
    mmio_write32(FMC_BASE + FMC_PATT_BANK3, 0x0A0A0A0A);

    g_psram_initialized = 1;
    return ERR_OK;
}

/* =========================================================================
 * Capture Ring Buffer
 * ========================================================================= */

void capture_ring_start(void)
{
    g_capture_ring.write_index = 0;
    g_capture_ring.read_index = 0;
    g_capture_ring.bytes_filled = 0;
    g_capture_ring.overflow = 0;
    g_capture_ring.active = 1;
}

void capture_ring_stop(void)
{
    g_capture_ring.active = 0;
}

uint32_t capture_ring_get_size(void)
{
    return g_capture_ring.bytes_filled;
}

/* =========================================================================
 * Capture Save/Load
 * ========================================================================= */

int capture_save_to_flash(uint8_t mod_type, uint8_t quality)
{
    if (!g_qspi_initialized) return ERR_NOT_INIT;

    /* Find first free slot by scanning metadata area */
    uint8_t slot = 0;
    capture_metadata_t meta;
    
    for (slot = 0; slot < CAPTURE_MAX_SLOTS; slot++) {
        uint32_t meta_addr = CAPTURE_PARTITION_METADATA + (slot * CAPTURE_METADATA_ENTRY_SIZE);
        qspi_read(meta_addr, (uint8_t *)&meta, sizeof(meta));
        
        /* Check if slot is free (timestamp == 0xFFFFFFFF or length == 0) */
        if (meta.timestamp == 0xFFFFFFFF || meta.timestamp == 0 || meta.length == 0)
            break;
    }
    
    if (slot >= CAPTURE_MAX_SLOTS)
        return ERR_STORAGE_FULL;

    /* Prepare metadata */
    meta.timestamp       = g_capture_ring.bytes_filled ? g_capture_ring.bytes_filled : 1; /* Placeholder — real RTC would go here */
    meta.offset          = CAPTURE_PARTITION_DATA + (slot * 256 * 1024); /* 256 KB per slot */
    meta.length          = g_capture_ring.bytes_filled;
    meta.sample_rate     = I2S_RX_SAMPLE_RATE;
    meta.bit_depth       = I2S_RX_BIT_DEPTH;
    meta.modulation_type = mod_type;
    meta.power_level     = g_ble.status.tx_power;
    meta.signal_quality  = quality;

    /* Erase metadata sector */
    qspi_erase_sector(CAPTURE_PARTITION_METADATA + (slot * CAPTURE_METADATA_ENTRY_SIZE));
    
    /* Write metadata */
    uint32_t meta_addr = CAPTURE_PARTITION_METADATA + (slot * CAPTURE_METADATA_ENTRY_SIZE);
    int ret = qspi_write(meta_addr, (const uint8_t *)&meta, sizeof(meta));
    if (ret) return ret;

    /* Erase data blocks for this slot */
    uint32_t data_addr = meta.offset;
    uint32_t remaining = meta.length;
    while (remaining > 0) {
        qspi_erase_sector(data_addr);
        data_addr += CAPTURE_SECTOR_SIZE;
        remaining -= (remaining > CAPTURE_SECTOR_SIZE) ? CAPTURE_SECTOR_SIZE : remaining;
    }

    /* Write PSRAM capture data to flash in 256-byte pages */
    data_addr = meta.offset;
    uint32_t psram_idx = 0;
    uint8_t page_buf[256];
    
    while (psram_idx < meta.length) {
        uint32_t chunk = (meta.length - psram_idx > 256) ? 256 : (meta.length - psram_idx);
        for (uint32_t i = 0; i < chunk; i++)
            page_buf[i] = ((uint8_t *)PSRAM_CAPTURE_BASE)[psram_idx + i];
        
        ret = qspi_write(data_addr, page_buf, chunk);
        if (ret) return ret;
        
        psram_idx += chunk;
        data_addr += chunk;
    }

    return (int)slot;
}

int capture_load_from_flash(uint8_t slot, uint8_t *out_data, uint32_t *out_len)
{
    if (!g_qspi_initialized) return ERR_NOT_INIT;
    if (slot >= CAPTURE_MAX_SLOTS || !out_data || !out_len)
        return ERR_INVALID_PARAM;

    capture_metadata_t meta;
    uint32_t meta_addr = CAPTURE_PARTITION_METADATA + (slot * CAPTURE_METADATA_ENTRY_SIZE);
    qspi_read(meta_addr, (uint8_t *)&meta, sizeof(meta));

    if (meta.length == 0 || meta.length > QSPI_FLASH_SIZE)
        return ERR_NO_DATA;

    *out_len = meta.length;
    qspi_read(meta.offset, out_data, meta.length);

    return ERR_OK;
}

int capture_get_metadata(uint8_t slot, capture_metadata_t *meta)
{
    if (!g_qspi_initialized || !meta) return ERR_NOT_INIT;
    if (slot >= CAPTURE_MAX_SLOTS) return ERR_INVALID_PARAM;

    uint32_t meta_addr = CAPTURE_PARTITION_METADATA + (slot * CAPTURE_METADATA_ENTRY_SIZE);
    qspi_read(meta_addr, (uint8_t *)meta, sizeof(*meta));

    return (meta->length == 0 || meta->length > QSPI_FLASH_SIZE) ? ERR_NO_DATA : ERR_OK;
}

int capture_delete(uint8_t slot)
{
    if (!g_qspi_initialized) return ERR_NOT_INIT;
    if (slot >= CAPTURE_MAX_SLOTS) return ERR_INVALID_PARAM;

    /* Read metadata to get data offset */
    capture_metadata_t meta;
    uint32_t meta_addr = CAPTURE_PARTITION_METADATA + (slot * CAPTURE_METADATA_ENTRY_SIZE);
    qspi_read(meta_addr, (uint8_t *)&meta, sizeof(meta));

    if (meta.length > 0 && meta.length <= QSPI_FLASH_SIZE) {
        /* Erase data blocks */
        meta.length = (meta.length + CAPTURE_SECTOR_SIZE - 1) & ~(CAPTURE_SECTOR_SIZE - 1); /* Round up */
        for (uint32_t offset = 0; offset < meta.length; offset += CAPTURE_BLOCK_SIZE) {
            qspi_erase_block_64k(meta.offset + offset);
        }
    }

    /* Clear metadata by writing zeros */
    uint8_t zeros[CAPTURE_METADATA_ENTRY_SIZE];
    memset(zeros, 0, sizeof(zeros));
    qspi_erase_sector(meta_addr);
    return qspi_write(meta_addr, zeros, sizeof(zeros));
}

uint8_t capture_get_slot_count(void)
{
    if (!g_qspi_initialized) return 0;

    uint8_t count = 0;
    for (uint8_t slot = 0; slot < CAPTURE_MAX_SLOTS; slot++) {
        capture_metadata_t meta;
        uint32_t meta_addr = CAPTURE_PARTITION_METADATA + (slot * CAPTURE_METADATA_ENTRY_SIZE);
        qspi_read(meta_addr, (uint8_t *)&meta, sizeof(meta));
        if (meta.length > 0 && meta.length <= QSPI_FLASH_SIZE)
            count++;
    }
    return count;
}