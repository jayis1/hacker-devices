/*
 * sd_driver.c — MicroSD card driver and PCAP file writer
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Implements SPI-mode SD card communication and PCAP file management.
 * Uses SPI1 on the RP2040 for SD card access. Writes PCAP files with
 * proper libpcap headers and packet records.
 */

#include "sd_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ---- SD card SPI commands (SPI mode) ---- */
#define SD_CMD0   0   /* GO_IDLE_STATE */
#define SD_CMD8   8   /* SEND_IF_COND (SD v2) */
#define SD_CMD9   9   /* SEND_CSD */
#define SD_CMD10  10  /* SEND_CID */
#define SD_CMD12  12  /* STOP_TRANSMISSION */
#define SD_CMD13  13  /* SEND_STATUS */
#define SD_CMD16  16  /* SET_BLOCKLEN */
#define SD_CMD17  17  /* READ_SINGLE_BLOCK */
#define SD_CMD24  24  /* WRITE_BLOCK */
#define SD_CMD25  25  /* WRITE_MULTIPLE_BLOCK */
#define SD_CMD41  41  /* APP_SEND_OP_COND (ACMD) */
#define SD_CMD55  55  /* APP_CMD (prefix for ACMD) */
#define SD_CMD58  58  /* READ_OCR */

#define SD_R1_IDLE        0x01
#define SD_R1_ILLEGAL     0x04
#define SD_DATA_TOKEN     0xFE
#define SD_DATA_TOKEN_WRITE 0xFC
#define SD_STOP_TOKEN     0xFD

/* ---- Private state ---- */
static uint8_t sd_initialized = 0;
static uint8_t sd_version2 = 0;  /* 1 if SD v2 (supports high capacity) */
static uint8_t sd_block_size = 512;
static uint32_t sd_capacity_mb = 0;

/* PCAP file state */
static uint32_t pcap_file_offset = 0;
static uint32_t pcap_file_seq = 0;
static uint32_t pcap_file_size = 0;
static uint8_t pcap_block_buf[PCAP_BLOCK_SIZE];
static uint32_t pcap_block_pos = 0;
static uint8_t pcap_open = 0;

/* ---- Low-level SPI helpers ---- */

#define SD_CS_LOW()   (SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_SD_CS_N))
#define SD_CS_HIGH()  (SIO_GPIO_OUT_SET = GPIO_BIT(PIN_SD_CS_N))

static void delay_us(uint32_t us)
{
    volatile uint32_t count = us * 44;
    while (count--) __asm__ volatile ("nop");
}

static void delay_ms(uint32_t ms)
{
    while (ms--) delay_us(1000);
}

static void gpio_init_simple(uint8_t pin, uint8_t output, uint8_t func)
{
    IO_BANK0_GPIO_CTRL(pin) = func;
    uint32_t pad = PAD_IE | PAD_DRIVE_4MA;
    if (!output) pad |= PAD_OD;
    PADS_BANK0_GPIO(pin) = pad;
    if (output)
        SIO_GPIO_OE_SET = GPIO_BIT(pin);
    else
        SIO_GPIO_OE_CLR = GPIO_BIT(pin);
}

static void spi1_init_hw(void)
{
    /* Configure SPI1 pins */
    IO_BANK0_GPIO_CTRL(PIN_SD_SCK)  = GPIO_FUNC_SPI;
    IO_BANK0_GPIO_CTRL(PIN_SD_MOSI) = GPIO_FUNC_SPI;
    IO_BANK0_GPIO_CTRL(PIN_SD_MISO) = GPIO_FUNC_SPI;

    gpio_init_simple(PIN_SD_CS_N, 1, GPIO_FUNC_SIO);
    SD_CS_HIGH();

    /* SD detect pin as input with pull-up */
    gpio_init_simple(PIN_SD_DETECT, 0, GPIO_FUNC_SIO);
    PADS_BANK0_GPIO(PIN_SD_DETECT) |= PAD_PUE;

    PADS_BANK0_GPIO(PIN_SD_SCK)  = PAD_IE | PAD_DRIVE_8MA | PAD_SLEWFAST;
    PADS_BANK0_GPIO(PIN_SD_MOSI) = PAD_IE | PAD_DRIVE_8MA | PAD_SLEWFAST;
    PADS_BANK0_GPIO(PIN_SD_MISO) = PAD_IE | PAD_DRIVE_4MA;

    /* Reset SPI1 */
    PSM_FRCE_OFF |= PSM_SPI1;
    delay_us(10);
    PSM_FRCE_OFF &= ~PSM_SPI1;
    delay_us(100);

    /* Configure SPI1: Master, 8-bit, Mode 0, slow initial clock (~400kHz) */
    SPI1->cpsr = 250;  /* 125MHz / 250 = 500kHz (initial slow clock) */
    SPI1->cr0 = SPI_CR0_DSS_8BIT | SPI_CR0_FRF_MOTOROLA;
    SPI1->cr1 = 0x00;
    SPI1->cr1 |= (1 << 1);  /* Enable */
}

static uint8_t spi1_transfer(uint8_t tx)
{
    while (!(SPI1->sr & SPI_SR_TNF)) { }
    SPI1->dr = tx;
    while (!(SPI1->sr & SPI_SR_RNE)) { }
    return (uint8_t)(SPI1->dr & 0xFF);
}

static void spi1_set_clock(uint32_t hz)
{
    /* Disable SPI, set new clock, re-enable */
    SPI1->cr1 &= ~(1 << 1);  /* Disable */
    uint32_t divisor = (PERI_CLK_HZ + hz - 1) / hz;
    if (divisor < 2) divisor = 2;
    SPI1->cpsr = divisor;  /* Even prescaler */
    if (divisor > 254) divisor = 254;  /* Max prescaler */
    SPI1->cr1 |= (1 << 1);  /* Enable */
}

/* ---- SD card command helpers ---- */

static uint8_t sd_wait_ready(uint32_t timeout_ms)
{
    uint8_t res;
    uint32_t t = timeout_ms;
    while (t--) {
        res = spi1_transfer(0xFF);
        if (res == 0xFF) return 0;  /* Ready */
        delay_us(1000);
    }
    return 0xFF;  /* Timeout */
}

static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t response;
    uint8_t crc = 0;  /* CRC only needed for CMD0 and CMD8 in SPI mode */

    /* For CMD0: CRC = 0x95, for CMD8: CRC = 0x87 */
    if (cmd == SD_CMD0) crc = 0x95;
    else if (cmd == SD_CMD8) crc = 0x87;

    spi1_transfer(0x40 | cmd);          /* Command byte (bit 6 = 1) */
    spi1_transfer((arg >> 24) & 0xFF);  /* Argument MSB */
    spi1_transfer((arg >> 16) & 0xFF);
    spi1_transfer((arg >> 8) & 0xFF);
    spi1_transfer(arg & 0xFF);           /* Argument LSB */
    spi1_transfer(crc);

    /* Wait for response (Ncr: 1-8 bytes for SD, 1-64 for MMC) */
    uint32_t n = 10;
    do {
        response = spi1_transfer(0xFF);
    } while ((response & 0x80) && --n);

    return response;
}

static int sd_read_block(uint32_t block_addr, uint8_t *buf)
{
    if (!sd_initialized) return -1;

    SD_CS_LOW();
    uint8_t r1 = sd_send_cmd(SD_CMD17, block_addr);
    if (r1 != 0x00) {
        SD_CS_HIGH();
        return -2;  /* Command rejected */
    }

    /* Wait for data token */
    uint32_t timeout = 200;
    uint8_t token;
    do {
        token = spi1_transfer(0xFF);
        if (token == SD_DATA_TOKEN) break;
        delay_us(1000);
    } while (--timeout);

    if (token != SD_DATA_TOKEN) {
        SD_CS_HIGH();
        return -3;  /* Timeout waiting for data */
    }

    /* Read 512 bytes of data */
    for (int i = 0; i < 512; i++) {
        buf[i] = spi1_transfer(0xFF);
    }

    /* Read and discard CRC (2 bytes) */
    spi1_transfer(0xFF);
    spi1_transfer(0xFF);

    SD_CS_HIGH();
    return 0;
}

static int sd_write_block(uint32_t block_addr, const uint8_t *buf)
{
    if (!sd_initialized) return -1;

    SD_CS_LOW();
    uint8_t r1 = sd_send_cmd(SD_CMD24, block_addr);
    if (r1 != 0x00) {
        SD_CS_HIGH();
        return -2;
    }

    /* Send write data token */
    spi1_transfer(SD_DATA_TOKEN_WRITE);

    /* Write 512 bytes */
    for (int i = 0; i < 512; i++) {
        spi1_transfer(buf[i]);
    }

    /* Send dummy CRC */
    spi1_transfer(0xFF);
    spi1_transfer(0xFF);

    /* Read data response */
    uint8_t resp = spi1_transfer(0xFF);
    if ((resp & 0x1F) != 0x05) {
        SD_CS_HIGH();
        return -3;  /* Write rejected */
    }

    /* Wait for write to complete (busy: returns 0x00) */
    if (sd_wait_ready(500)) {
        SD_CS_HIGH();
        return -4;  /* Write timeout */
    }

    SD_CS_HIGH();
    return 0;
}

/* ============================================================
 * Public API
 * ============================================================ */

int sd_init(void)
{
    spi1_init_hw();

    if (!sd_is_present()) {
        return -1;  /* No card */
    }

    /* Send 80 dummy clocks with CS high to enter SPI mode */
    SD_CS_HIGH();
    for (int i = 0; i < 80; i++) {
        spi1_transfer(0xFF);
    }

    /* CMD0: Reset to idle state */
    SD_CS_LOW();
    uint8_t r1 = sd_send_cmd(SD_CMD0, 0);
    if (r1 != SD_R1_IDLE) {
        SD_CS_HIGH();
        return -2;  /* No SD card or not in SPI mode */
    }

    /* CMD8: Check SD version */
    r1 = sd_send_cmd(SD_CMD8, 0x000001AA);
    if (r1 == SD_R1_IDLE) {
        /* SD v2: read 4-byte OCR */
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) ocr[i] = spi1_transfer(0xFF);
        if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
            sd_version2 = 1;
        }
    }
    SD_CS_HIGH();

    /* CMD55 + ACMD41: Initialize card */
    uint32_t timeout = 2000;
    do {
        SD_CS_LOW();
        sd_send_cmd(SD_CMD55, 0);
        r1 = sd_send_cmd(SD_CMD41, sd_version2 ? 0x40000000 : 0);
        SD_CS_HIGH();
        if (r1 == 0x00) break;  /* Initialization complete */
        delay_ms(1);
    } while (--timeout);

    if (r1 != 0x00) {
        return -3;  /* Initialization failed */
    }

    /* Read CSD to get capacity */
    SD_CS_LOW();
    sd_send_cmd(SD_CMD9, 0);
    /* Wait for data token */
    uint8_t token;
    uint32_t t = 200;
    do {
        token = spi1_transfer(0xFF);
        if (token == SD_DATA_TOKEN) break;
        delay_us(1000);
    } while (--t);

    if (token == SD_DATA_TOKEN) {
        uint8_t csd[16];
        for (int i = 0; i < 16; i++) csd[i] = spi1_transfer(0xFF);
        spi1_transfer(0xFF);  /* CRC */
        spi1_transfer(0xFF);

        /* Parse CSD: capacity = (C_SIZE + 1) * 512K bytes (for SDHC) */
        if (csd[0] & 0xC0) {
            /* CSD version 2.0 (SDHC/SDXC) */
            uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) |
                              ((uint32_t)csd[8] << 8) | csd[9];
            sd_capacity_mb = (c_size + 1) / 2;  /* Convert to MB */
        }
    }
    SD_CS_HIGH();

    /* Set block size to 512 (should already be, but be explicit) */
    SD_CS_LOW();
    sd_send_cmd(SD_CMD16, 512);
    SD_CS_HIGH();

    /* Increase SPI clock to 24 MHz for data transfer */
    spi1_set_clock(SD_SPI_HZ);

    sd_initialized = 1;
    return 0;
}

uint8_t sd_is_present(void)
{
    /* SD detect switch: active low (pressed = card present) */
    return !(SIO_GPIO_IN & GPIO_BIT(PIN_SD_DETECT)) ? 1 : 0;
}

uint32_t sd_get_capacity_mb(void)
{
    return sd_capacity_mb;
}

uint32_t sd_get_free_mb(void)
{
    /* Approximate: assume 90% of capacity is free initially.
     * A full FAT32 implementation would read the FSInfo sector.
     * For the reference firmware, we track used space via pcap_file_size. */
    uint32_t used = pcap_file_size / (1024 * 1024);
    if (used >= sd_capacity_mb) return 0;
    return sd_capacity_mb - used;
}

int sd_pcap_open(uint32_t link_type)
{
    if (!sd_initialized) return -1;

    /* Close any previously open file */
    if (pcap_open) sd_pcap_close();

    /* Generate PCAP file header */
    pcap_header_t hdr = {
        .magic = PCAP_MAGIC,
        .version_major = PCAP_VERSION_MAJOR,
        .version_minor = PCAP_VERSION_MINOR,
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = PCAP_SNAPLEN,
        .network = link_type,
    };

    /* Start writing to a new block-aligned region on SD.
     * In the reference firmware, we write to a fixed file offset
     * computed from the file sequence number. A real implementation
     * would use a FAT32 filesystem library. */
    pcap_file_seq++;
    pcap_file_offset = 0x10000 + (pcap_file_seq * PCAP_MAX_FILE_SIZE_MB * 1024 * 1024ULL);

    /* Write PCAP header to first block */
    uint8_t block[512];
    memset(block, 0xFF, sizeof(block));
    memcpy(block, &hdr, sizeof(hdr));

    int ret = sd_write_block(pcap_file_offset / 512, block);
    if (ret) return ret;

    pcap_block_pos = sizeof(pcap_header_t);
    pcap_file_size = sizeof(pcap_header_t);
    memset(pcap_block_buf, 0xFF, sizeof(pcap_block_buf));
    memcpy(pcap_block_buf, &hdr, sizeof(hdr));
    pcap_open = 1;

    return 0;
}

int sd_pcap_write_packet(const uint8_t *data, uint32_t len,
                         uint32_t ts_sec, uint32_t ts_usec,
                         uint32_t orig_len)
{
    if (!pcap_open || !sd_initialized) return -1;

    /* Build PCAP record header */
    pcap_record_header_t rec = {
        .ts_sec = ts_sec,
        .ts_usec = ts_usec,
        .incl_len = len,
        .orig_len = orig_len,
    };

    /* Write record header to block buffer */
    if (pcap_block_pos + sizeof(rec) > PCAP_BLOCK_SIZE) {
        /* Flush current block to SD */
        if (sd_write_block(pcap_file_offset / 512 + pcap_file_size / 512,
                           pcap_block_buf)) {
            return -2;
        }
        pcap_block_pos = 0;
        memset(pcap_block_buf, 0xFF, sizeof(pcap_block_buf));
    }

    memcpy(&pcap_block_buf[pcap_block_pos], &rec, sizeof(rec));
    pcap_block_pos += sizeof(rec);
    pcap_file_size += sizeof(rec);

    /* Write packet data (may span multiple blocks) */
    uint32_t data_remaining = len;
    uint32_t data_offset = 0;
    while (data_remaining > 0) {
        uint32_t space_in_block = PCAP_BLOCK_SIZE - pcap_block_pos;
        uint32_t to_copy = (data_remaining < space_in_block) ? data_remaining : space_in_block;

        memcpy(&pcap_block_buf[pcap_block_pos], &data[data_offset], to_copy);
        pcap_block_pos += to_copy;
        pcap_file_size += to_copy;
        data_offset += to_copy;
        data_remaining -= to_copy;

        if (pcap_block_pos >= PCAP_BLOCK_SIZE) {
            /* Flush block to SD */
            uint32_t block_num = pcap_file_offset / 512 + (pcap_file_size / 512);
            if (sd_write_block(block_num, pcap_block_buf)) {
                return -3;
            }
            pcap_block_pos = 0;
            memset(pcap_block_buf, 0xFF, sizeof(pcap_block_buf));
        }
    }

    return 0;
}

int sd_pcap_close(void)
{
    if (!pcap_open) return -1;

    /* Flush remaining data in block buffer to SD */
    if (pcap_block_pos > 0) {
        uint32_t block_num = pcap_file_offset / 512 + (pcap_file_size / 512);
        sd_write_block(block_num, pcap_block_buf);
    }

    pcap_open = 0;
    return 0;
}

uint32_t sd_pcap_get_filesize(void)
{
    return pcap_file_size;
}

int sd_list_files(sd_file_callback_t callback)
{
    if (!sd_initialized || !callback) return -1;

    /* List capture files. In the reference firmware, files are at
     * fixed offsets based on sequence number. A real FAT32 implementation
     * would scan directory entries. */
    for (uint32_t seq = 1; seq <= pcap_file_seq; seq++) {
        char name[24];
        /* Format: FP_NNNNN.pcap */
        name[0] = 'F'; name[1] = 'P'; name[2] = '_';
        /* Simple number formatting */
        uint32_t n = seq;
        for (int i = 7; i >= 3; i--) {
            name[i] = '0' + (n % 10);
            n /= 10;
        }
        name[8] = '.'; name[9] = 'p'; name[10] = 'c'; name[11] = 'a';
        name[12] = 'p'; name[13] = '\0';

        uint32_t size = PCAP_MAX_FILE_SIZE_MB * 1024 * 1024;  /* Approximate */
        callback(name, size);
    }

    return 0;
}

int sd_delete_file(const char *filename)
{
    /* Reference: mark blocks as erased. Real FAT32 would update dir entry. */
    (void)filename;
    return 0;
}

int sd_read_file_chunk(const char *filename, uint32_t offset,
                       uint8_t *buf, uint32_t chunk_size, uint32_t *bytes_read)
{
    if (!sd_initialized) return -1;
    (void)filename;  /* Simplified: read from current pcap file */

    uint32_t block_num = pcap_file_offset / 512 + (offset / 512);
    uint32_t offset_in_block = offset % 512;
    uint32_t to_read = chunk_size;

    /* Read blocks */
    uint32_t buf_pos = 0;
    while (to_read > 0) {
        uint8_t block[512];
        if (sd_read_block(block_num, block)) return -2;

        uint32_t available = 512 - offset_in_block;
        uint32_t copy_len = (to_read < available) ? to_read : available;
        memcpy(&buf[buf_pos], &block[offset_in_block], copy_len);

        buf_pos += copy_len;
        to_read -= copy_len;
        block_num++;
        offset_in_block = 0;
    }

    if (bytes_read) *bytes_read = buf_pos;
    return 0;
}

int sd_health_check(void)
{
    if (!sd_is_present()) return -1;
    if (!sd_initialized) return -2;
    /* Try a simple status command */
    SD_CS_LOW();
    uint8_t r1 = sd_send_cmd(SD_CMD13, 0);
    SD_CS_HIGH();
    if (r1 != 0x00) return -3;
    return 0;
}